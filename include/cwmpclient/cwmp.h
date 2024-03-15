/**
 * @Copyright : Yangrongcan
 */
#if !defined(_CWMP_CWMP_)
#define _CWMP_CWMP_

#include <list>
#include <string>

using namespace std;

constexpr int MAX_DOWNLOAD = 10;
constexpr int MAX_UPLOAD = 10;
constexpr int FAULT_ACS_8005 = 8005;

enum END_SESSION {
	ENDS_REBOOT = 0x01,
	ENDS_FACTORY_RESET = 0x02,
	ENDS_RELOAD_CONFIG = 0x04,
};

enum EVENT_TYPE {
	EVENT_SINGLE,
	EVENT_MULTIPLE
};

enum EVENT_REMOVE_POLICY {
	EVENT_REMOVE_AFTER_INFORM = 0x1,
	EVENT_REMOVE_AFTER_TRANSFER_COMPLETE = 0x2,
	EVENT_REMOVE_NO_RETRY = 0x4
};

enum {
	EVENT_BOOTSTRAP = 0,
	EVENT_BOOT,
	EVENT_PERIODIC,
	EVENT_SCHEDULED,
	EVENT_VALUE_CHANGE,
	EVENT_KICKED,
	EVENT_CONNECTION_REQUEST,
	EVENT_TRANSFER_COMPLETE,
	EVENT_DIAGNOSTICS_COMPLETE,
	EVENT_REQUEST_DOWNLOAD,
	EVENT_AUTONOMOUS_TRANSFER_COMPLETE,
	EVENT_M_REBOOT,
	EVENT_M_SCHEDULEINFORM,
	EVENT_M_DOWNLOAD,
	EVENT_M_UPLOAD,
	__EVENT_MAX
};

struct event_code
{
	string code;
	int type;
	int remove_policy;
};

struct event {
	int code;
	string key;
	int method_id;
};

struct download {
	char *key;
	char *download_url;
	char *file_size;
	char *file_type;
	char *username;
	char *password;
	time_t time_execute;
	// mxml_node_t *backup_node;
};

struct upload {
	char *key;
	char *upload_url;
	char *file_type;
	char *username;
	char *password;
	time_t time_execute;
	// mxml_node_t *backup_node;
};

struct notification {
	string parameter;
	string value;
	string type;
};

struct deviceInfo {
	string manufacturer;
	string oui;
	string product_class;
	string serial_number;
};

class cwmpInfo
{
private:
    /* data */
    list<event> events;
    list<notification> notifications;
    list<download> downloads;
    list<upload> uploads;
    deviceInfo deviceInfo;
    int retry_count;
    int download_count;
    int upload_count;
    int end_session;
    int method_id;
    bool get_rpc_methods;
    bool hold_requests;
    int netlink_sock[2];
public:
    cwmpInfo();
    ~cwmpInfo();
	void add_scheduled_inform(char *key, int delay);
	// void add_download(char *key, int delay, char *file_size, char *download_url, char *file_type, char *username, char *password, mxml_node_t *node);
	// void add_upload(char *key, int delay, char *upload_url, char *file_type, char *username, char *password, mxml_node_t *node);
	// void download_launch(struct uloop_timeout *timeout);
	// void upload_launch(struct uloop_timeout *timeout);
	void init(void);
	void connection_request(int code);
	void remove_event(int remove_policy, int method_id);
	void clear_event_list(void);
	void add_notification(char *parameter, char *value, char *type, char *notification);
	void clear_notifications(void);
	// void scheduled_inform(struct uloop_timeout *timeout);
	void add_handler_end_session(int handler);

	int inform(void);
	int handle_messages(void);
	int set_parameter_write_handler(char *name, char *value);
	int get_int_event_code(char *code);

	event *add_event(int code, char *key, int method_id, int backup);
	long int periodic_inform_time(void);
	void update_value_change(void);
	void add_inform_timer();
	void clean(void);
	void periodic_inform_init(void);
	int init_deviceid(void);
	void free_deviceid(void);
};

extern cwmpInfo *cwmp;
extern event_code event_code_array[__EVENT_MAX];

#endif // _CWMP_CWMP_
