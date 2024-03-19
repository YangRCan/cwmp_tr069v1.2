/**
 * @Copyright : Yangrongcan
 */
#if !defined(_CWMP_CWMP_)
#define _CWMP_CWMP_

#include <list>
#include <string>

#include "tinyxml2.h"

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

enum EVENT_BACKUP_SAVE {
	EVENT_NO_BACKUP = 0,
	EVENT_BACKUP
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
	std::string code;
	int type;
	int remove_policy;
};

struct event {
	int code;
	std::string key;
	int method_id;
	tinyxml2::XMLElement *backup_node; //该事件对应的备份节点
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
	std::string parameter;
	std::string value;
	std::string type;
};

struct deviceInfo {
	std::string manufacturer;
	std::string oui;
	std::string product_class;
	std::string serial_number;
};

class cwmpInfo
{
private:
    /* data */
    std::list<event> events;
    std::list<notification> notifications;
    std::list<download> downloads;
    std::list<upload> uploads;
    deviceInfo deviceInfo;
    int retry_count;
    int download_count;
    int upload_count;
    int end_session;
    int method_id;
    bool get_rpc_methods;
    bool hold_requests;
    int netlink_sock[2];
	bool retry_inform;
public:
    cwmpInfo();
    ~cwmpInfo();
	void set_get_rpc_methods(bool flag);
	void cwmp_init_deviceid(void);

	event* cwmp_add_event(int code, const char *key, int method_id, int backup);

	void cwmp_add_inform_timer(void);
	void cwmp_do_inform(void);
	void cwmp_do_inform_retry(int delaySeconds);

	int cwmp_inform(void);

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
