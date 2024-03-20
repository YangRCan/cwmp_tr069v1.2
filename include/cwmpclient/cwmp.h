/**
 * @Copyright : Yangrongcan
 */
#if !defined(_CWMP_CWMP_)
#define _CWMP_CWMP_

#include <list>
#include <string>
#include <atomic>

#include "tinyxml2.h"

constexpr int MAX_DOWNLOAD = 10;
constexpr int MAX_UPLOAD = 10;
constexpr int FAULT_ACS_8005 = 8005;

enum END_SESSION
{
	ENDS_REBOOT = 0x01,
	ENDS_FACTORY_RESET = 0x02,
	ENDS_RELOAD_CONFIG = 0x04,
};

enum EVENT_TYPE
{
	EVENT_SINGLE,
	EVENT_MULTIPLE
};

enum EVENT_BACKUP_SAVE
{
	EVENT_NO_BACKUP = 0,
	EVENT_BACKUP
};

enum EVENT_REMOVE_POLICY
{
	EVENT_REMOVE_AFTER_INFORM = 0x1,
	EVENT_REMOVE_AFTER_TRANSFER_COMPLETE = 0x2,
	EVENT_REMOVE_NO_RETRY = 0x4
};

enum
{
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

struct event
{
	int code;
	std::string key;
	int method_id;
	tinyxml2::XMLElement *backup_node; // 该事件对应的备份节点
};

struct download
{
	std::atomic<bool> cancelFlag;
	std::string key;
	std::string download_url;
	std::string file_size;
	std::string file_type;
	std::string username;
	std::string password;
	time_t time_execute;
	tinyxml2::XMLElement *backup_node;
};

struct upload
{
	std::atomic<bool> cancelFlag;
	std::string key;
	std::string upload_url;
	std::string file_type;
	std::string username;
	std::string password;
	time_t time_execute;
	tinyxml2::XMLElement *backup_node;
};

struct notification
{
	std::string parameter;
	std::string value;
	std::string type;
};

struct deviceInfo
{
	std::string manufacturer;
	std::string oui;
	std::string product_class;
	std::string serial_number;
};

class cwmpInfo
{
private:
	/* data */
	std::list<event *> events;
	std::list<notification *> notifications;
	std::list<download *> downloads;
	std::list<upload *> uploads;
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
	std::atomic<int> inform_num;//用于保证不重复上报，新建定期上报线程就得+1，值不大于100

	void cwmp_periodic_inform(int inform_num_copy, long interval);
public:
	cwmpInfo();
	~cwmpInfo();
	void cwmp_clean(void);
	void cwmp_clear_event_list(void);
	void cwmp_clear_notifications(void);

	void set_get_rpc_methods(bool flag);
	void cwmp_init_deviceid(void);

	event *cwmp_add_event(int code, std::string key, int method_id, int backup);

	void cwmp_add_inform_timer(void);
	void cwmp_do_inform(void);
	void cwmp_do_inform_retry(int delaySeconds);
	void cwmp_periodic_inform_init(void);

	int cwmp_inform(void);

	int cwmp_periodic_inform_time(void);
};

extern cwmpInfo *cwmp;
extern event_code event_code_array[__EVENT_MAX];

#endif // _CWMP_CWMP_
