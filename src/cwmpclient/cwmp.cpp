/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include <thread>
#include <chrono>

#include "cwmp.h"
#include "cwmpclient.h"
#include "log.h"
#include "http.h"
#include "backup.h"
#include "tinyxml2.h"

namespace tx = tinyxml2;

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
	this->retry_inform = false;
}

cwmpInfo::~cwmpInfo()
{
}

/**
 * 设置成员变量get_rpc_methods的值
 */
void cwmpInfo::set_get_rpc_methods(bool flag)
{
	this->get_rpc_methods = flag;
}

/**
 * 读取设备信息到cwmp的deviceInfo成员中
 */
void cwmpInfo::cwmp_init_deviceid(void)
{
}

/**
 * 向events链表中添加事件
 * return: 返回该新创建的event对象
 */
event *cwmpInfo::cwmp_add_event(int code, const char *key, int method_id, int backup)
{
	event *e = NULL;
	int type = event_code_array[code].type;
	Log(NAME, L_NOTICE, "add event '%s'\n", event_code_array[code].code);
	if (type == EVENT_SINGLE)
	{
		// 迭代事件链表,若事件已存在，返回NULL
		for (auto it = this->events.begin(); it != this->events.end(); ++it)
		{
			if (it->code == code)
			{
				return NULL;
			}
		}
	}

	e = new event;
	if (!e)
		return NULL; // 内存分配失败
	this->events.push_back(*e);
	e->code = code;
	e->key = key;
	e->method_id = method_id;
	if (backup == EVENT_BACKUP)
		e->backup_node = backup_add_event(code, key, method_id);
	return e;
}

// /**
//  * 设置一个 Inform 定时器，每隔 10 毫秒触发一次
//  */
// void cwmpInfo::cwmp_add_inform_timer(void)
// {
// 	this->retry_inform = false;
// 	thread timerThread(this->cwmp_do_inform);
// }

// /**
//  * 延迟5毫秒执行Inform
//  */
// void cwmpInfo::cwmp_do_inform(void)
// {
// 	// 等待5毫秒
// 	std::this_thread::sleep_for(std::chrono::milliseconds(5));
// 	cwmp_inform(); // 到点执行该函数
// }

// /**
//  * 重试，延迟一段时间后执行Inform
//  */
// void cwmpInfo::cwmp_do_inform_retry(int delaySeconds)
// {
// 	// 等待指定秒数
// 	std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));
// 	cwmp_inform();
// }

// /**
//  * 发送Inform消息的函数
//  */
// int cwmpInfo::cwmp_inform(void)
// {
// 	// tx::XMLElement *node;
// 	// int method_id;

// 	// auto handle_error = [this](const string &message)
// 	// {
// 	// 	Log(NAME, L_DEBUG, message.c_str());
// 	// 	http_client_exit();
// 	// 	xml_exit();
// 	// 	this->cwmp_handle_end_session();
// 	// 	Log(NAME, L_NOTICE, "end session failed\n");
// 	// 	this->cwmp_retry_session();
// 	// 	return -1;
// 	// }

// 	// Log(NAME, L_NOTICE, "start session\n");
// 	// if (http_client_init()) // 返回 1 表示初始化失败
// 	// {
// 	// 	return handle_error("initializing http client failed\n");
// 	// }
// 	// if (this->rpc_inform()) // 发送 Inform 消息，并在接收到响应后解析消息内容
// 	// {
// 	// 	return handle_error("sending Inform failed\n");
// 	// }
// 	// Log(NAME, L_NOTICE, "receive InformResponse from the ACS\n");

// 	// this->cwmp_remove_event(EVENT_REMOVE_AFTER_INFORM, 0); // 移除符合特定条件(参数)的事件节点
// 	// this->cwmp_clear_notifications();					   // 清空cwmp->notifications链表

// 	// do
// 	// {
// 	// 	// 循环发送http完成请求给ACS，直到拿到响应，或出错
// 	// 	// backup_check_transfer_complete: 在 backup_tree 中查找名为 "transfer_complete" 的节点, 并返回
// 	// 	while ((node = backup_check_transfer_complete()) && !this->hold_requests)
// 	// 	{													   // 在rpc_inform（）中的解析响应数据时获取了cwmp->hold_requests的值
// 	// 		if (this->rpc_transfer_complete(node, &method_id)) // 发送传输完成http给ACS，并查看响应是否有出错
// 	// 		{
// 	// 			return handle_error("sending TransferComplete failed\n");
// 	// 		}
// 	// 		Log(NAME, L_NOTICE, "receive TransferCompleteResponse from the ACS\n");

// 	// 		backup_remove_transfer_complete(node);									  // 释放传入节点的内存空间
// 	// 		this->cwmp_remove_event(EVENT_REMOVE_AFTER_TRANSFER_COMPLETE, method_id); // 移除符合特定条件(参数)的事件节点
// 	// 		if (!backup_check_transfer_complete())									  // 找不到名为 "transfer_complete" 的节点, 进入
// 	// 			this->cwmp_remove_event(EVENT_REMOVE_AFTER_TRANSFER_COMPLETE, 0);	  // 移除符合特定条件(参数)的事件节点
// 	// 	}
// 	// 	// 如果要获取RPC方法，且现在还未拿到ACS的响应
// 	// 	if (this->get_rpc_methods && !this->hold_requests)
// 	// 	{
// 	// 		if (this->rpc_get_rpc_methods()) // 发送getRPCMethods请求
// 	// 		{
// 	// 			return handle_error("sending GetRPCMethods failed\n");
// 	// 		}
// 	// 		Log(NAME, L_NOTICE, "receive GetRPCMethodsResponse from the ACS\n");

// 	// 		this->get_rpc_methods = false; // 修改为false，因为已经请求过了
// 	// 	}

// 	// 	if (this->cwmp_handle_messages())
// 	// 	{
// 	// 		return handle_error("handling xml message failed\n");
// 	// 	}
// 	// 	this->hold_requests = false;
// 	// } while (this->get_rpc_methods || backup_check_transfer_complete());

// 	// http_client_exit();
// 	// xml_exit();
// 	// this->cwmp_handle_end_session();
// 	// this->retry_count = 0;
// 	// Log(NAME, L_NOTICE, "end session success\n");
// 	// return 0;
// }