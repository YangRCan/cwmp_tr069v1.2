/**
 * @Copyright : Yangrongcan
 */
#include <iostream>

#include "cwmp.h"
#include "cwmpclient.h"
#include "log.h"
#include "backup.h"

event_code event_code_array[__EVENT_MAX] = {
	[EVENT_BOOTSTRAP] = {"0 BOOTSTRAP", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_BOOT] = {"1 BOOT", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_PERIODIC] = {"2 PERIODIC", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_SCHEDULED] = {"3 SCHEDULED", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_VALUE_CHANGE] = {"4 VALUE CHANGE", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_KICKED] = {"5 KICKED", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_CONNECTION_REQUEST] = {"6 CONNECTION REQUEST", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM | EVENT_REMOVE_NO_RETRY},
	[EVENT_TRANSFER_COMPLETE] = {"7 TRANSFER COMPLETE", EVENT_SINGLE, EVENT_REMOVE_AFTER_TRANSFER_COMPLETE},
	[EVENT_DIAGNOSTICS_COMPLETE] = {"8 DIAGNOSTICS COMPLETE", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_REQUEST_DOWNLOAD] = {"9 REQUEST DOWNLOAD", EVENT_SINGLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_AUTONOMOUS_TRANSFER_COMPLETE] = {"10 AUTONOMOUS TRANSFER COMPLETE", EVENT_SINGLE, EVENT_REMOVE_AFTER_TRANSFER_COMPLETE},
	[EVENT_M_REBOOT] = {"M Reboot", EVENT_MULTIPLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_M_SCHEDULEINFORM] = {"M ScheduleInform", EVENT_MULTIPLE, EVENT_REMOVE_AFTER_INFORM},
	[EVENT_M_DOWNLOAD] = {"M Download", EVENT_MULTIPLE, EVENT_REMOVE_AFTER_TRANSFER_COMPLETE},
	[EVENT_M_UPLOAD] = {"M Upload", EVENT_MULTIPLE, EVENT_REMOVE_AFTER_TRANSFER_COMPLETE}};

cwmpInfo::cwmpInfo()
{
	this->retry_count = 0;
	this->download_count = 0;
	this->upload_count = 0;
	this->end_session = 0;
	this->method_id = 0;
	this->get_rpc_methods = false;
	this->hold_requests = false;
	this->netlink_sock[0] = 0;
	this->netlink_sock[1] = 0;
}

cwmpInfo::~cwmpInfo()
{
}

event* cwmpInfo::cwmp_add_event(int code, const char *key, int method_id, int backup) {
	event *e = NULL;
	int type = event_code_array[code].type;
	Log(NAME, L_NOTICE, "add event '%s'\n", event_code_array[code].code);
	if(type == EVENT_SINGLE) {
		// 迭代事件链表,若事件已存在，返回NULL
		for(auto it = this->events.begin(); it != this->events.end(); ++it) {
			if(it->code == code) {
				return NULL;
			}
		}
	}

	e = new event;
	if(!e) return NULL;//内存分配失败
	this->events.push_back(*e);
	e->code = code;
	e->key = key;
	e->method_id = method_id;
	if(backup == EVENT_BACKUP)
		e->backup_node = backup_add_event(code, key, method_id);
	return e;
}