/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>

#include "cwmp.h"
#include "cwmpclient.h"
#include "log.h"
#include "http.h"
#include "backup.h"
#include "config.h"
#include "time_tool.h"
#include "parameter.h"
#include "faultCode.h"
#include "tr181.h"
#include "tinyxml2.h"
#include "xml.h"

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
	this->isInfroming = false;
	this->inform_num = 0;
}

cwmpInfo::~cwmpInfo()
{
	Log(NAME, L_DEBUG, "cwmp object destroyed.\n");
}

/**
 * 返回成员 retry_count
 */
int cwmpInfo::get_retry_count(void)
{
	return this->retry_count;
}

/**
 * 返回成员 events
 */
std::list<event *> cwmpInfo::get_event_list(void)
{
	return this->events;
}

/**
 * 返回成员 notifications
 */
std::list<notification *> cwmpInfo::get_notifications(void)
{
	return this->notifications;
}

/**
 * 返回成员 deviceInfo
 */
deviceInfo cwmpInfo::get_device_info(void)
{
	return this->deviceInfo;
}

/**
 * 设置成员变量get_rpc_methods的值
 */
void cwmpInfo::set_get_rpc_methods(bool flag)
{
	this->get_rpc_methods = flag;
}

/**
 * 设置成员deviceid的成员值
 */
void cwmpInfo::set_deviceid(std::string manufacturer, std::string oui, std::string product_class, std::string serial_number)
{
	this->deviceInfo.manufacturer = manufacturer;
	this->deviceInfo.oui = oui;
	this->deviceInfo.product_class = product_class;
	this->deviceInfo.serial_number = serial_number;
}

/**
 * 从配置文件读取设备信息到cwmp的deviceInfo成员中
 */
void cwmpInfo::cwmp_init_deviceInfo(void)
{
	read_config_to_cwmp_deviceid();

	init_root();
	init_dataModel();
	std::string root(ROOT);
	std::string parameterPath;
	setParameter((root + parameterPath.assign(".DeviceInfo.Manufacturer")).c_str(), cwmp->deviceInfo.manufacturer.c_str(), NOVERIFY);
	setParameter((root + parameterPath.assign(".DeviceInfo.ManufacturerOUI")).c_str(), cwmp->deviceInfo.oui.c_str(), NOVERIFY);
	setParameter((root + parameterPath.assign(".DeviceInfo.ProductClass")).c_str(), cwmp->deviceInfo.product_class.c_str(), NOVERIFY);
	setParameter((root + parameterPath.assign(".DeviceInfo.SerialNumber")).c_str(), cwmp->deviceInfo.serial_number.c_str(), NOVERIFY);
}

/**
 * 清除链表内容，变量赋为初始值
 */
void cwmpInfo::cwmp_clean(void)
{
	download *d;
	upload *u;
	cwmp_clear_event_list();
	cwmp_clear_notifications();
	for (auto it = this->downloads.begin(); it != this->downloads.end(); ++it)
	{
		(*it)->cancelFlag = true; // 指向的空间不能释放，由线程自己去释放
	}
	this->downloads.clear();
	for (auto it = this->uploads.begin(); it != this->uploads.end(); ++it)
	{
		(*it)->cancelFlag = true;
	}
	this->uploads.clear();
	this->download_count = 0;
	this->upload_count = 0;
	this->end_session = 0;
	this->retry_count = 0;
	this->hold_requests = false;
	this->get_rpc_methods = false;
}

/**
 * 释放对象中的 events 列表
 */
void cwmpInfo::cwmp_clear_event_list(void)
{
	for (auto it = this->events.begin(); it != this->events.end(); ++it)
	{
		delete *it;
	}
	this->events.clear();
}

/**
 * 释放对象中的 notifications 列表
 */
void cwmpInfo::cwmp_clear_notifications(void)
{
	for (auto it = this->notifications.begin(); it != this->notifications.end(); ++it)
	{
		delete *it;
	}
	this->notifications.clear();
}

/**
 * 向events链表中添加事件
 * return: 返回该新创建的event对象
 */
event *cwmpInfo::cwmp_add_event(int code, std::string key, int method_id, int backup)
{
	event *e = NULL;
	int type = event_code_array[code].type;
	Log(NAME, L_NOTICE, "add event '%s'\n", event_code_array[code].code.c_str());
	if (type == EVENT_SINGLE)
	{
		// 迭代事件链表,若事件已存在，返回NULL
		for (auto it = this->events.begin(); it != this->events.end(); ++it)
		{
			if ((*it)->code == code)
			{
				return NULL;
			}
		}
	}

	e = new event;
	if (!e)
		return NULL; // 内存分配失败
	this->events.push_back(e);
	e->code = code;
	e->key = key;
	e->method_id = method_id;
	if (backup == EVENT_BACKUP)
		e->backup_node = backup_add_event(code, key, method_id);
	return e;
}

/**
 * 移除符合特定条件(参数)的事件节点
 */
void cwmpInfo::cwmp_remove_event(int remove_policy, int method_id)
{
	this->events.remove_if([remove_policy, method_id](event *e)
						   { return (event_code_array[e->code].remove_policy & remove_policy) && e->method_id == method_id; });
}

/**
 * 向cwmp中的downloads列表添加下载任务
 */
void cwmpInfo::cwmp_add_download(std::string key, int delay, std::string file_size, std::string download_url, std::string file_type, std::string username, std::string password, tx::XMLElement *node)
{
	download *d = new download;
	if (!d)
		return;
	cwmp->download_count++;
	d->key = key;
	d->file_size = file_size;
	d->download_url = download_url;
	d->file_type = file_type;
	d->username = username;
	d->password = password;
	d->cancelFlag = false;
	d->backup_node = node;
	d->time_execute = time(NULL) + delay;
	d->state = UD_NO_START;
	this->downloads.push_back(d);
	Log(NAME, L_NOTICE, "add download: delay = %d sec, url = %s, FileType = '%s', CommandKey = '%s'\n", delay, d->download_url.c_str(), d->file_type.c_str(), d->key.c_str());
	// 创建定时线程去执行cwmp_download_launch函数
	std::thread downloadThread(&cwmpInfo::cwmp_download_launch, this, d, 10);
	downloadThread.detach();
}

/**
 * 执行下载的函数
 */
void cwmpInfo::cwmp_download_launch(download *d, int delay)
{
	std::this_thread::sleep_for(std::chrono::seconds(delay));
	if (d->cancelFlag == true)
	{
		delete d; // 释放下载事件存储的空间
		return;
	}
	d->state = UD_EXECUTING;
	Log(NAME, L_NOTICE, "start download url = %s, FileType = '%s', CommandKey = '%s'\n", d->download_url.c_str(), d->file_type.c_str(), d->key.c_str());
	int code = FAULT_0;
	std::string start_time(get_time());
	downloadFile(d->download_url.c_str(), d->file_type.c_str(), d->file_size.c_str(), d->username.c_str(), d->password.c_str());
	backup_remove_node(d->backup_node); // 从备份文件中移除该节点
	this->downloads.remove(d);			// 从链表中删除该download节点
	this->download_count--;
	tx::XMLElement *node = backup_add_transfer_complete(d->key, code, start_time, ++this->method_id);
	if (!node)
	{
		delete d;
		return;
	}
	this->cwmp_add_event(EVENT_TRANSFER_COMPLETE, "", 0, EVENT_BACKUP);
	this->cwmp_add_event(EVENT_M_DOWNLOAD, d->key, this->method_id, EVENT_BACKUP);

	int status = 1, fault = FAULT_0;
	getExecutionStatus(&status, &fault); // 获取执行下载后的结果状态
	if (fault > 0)
	{ // 执行出错
		code = fault;
		Log(NAME, L_NOTICE, "download error: '%s'\n", fault_array[code].string);
		backup_update_fault_transfer_complete(node, code);
		this->cwmp_add_inform_timer(10);
		return;
	}
	else if (status != 1)
	{ // 命令不成功
		code = FAULT_9002;
		Log(NAME, L_NOTICE, "download error: '%s'\n", fault_array[code].string);
		backup_update_fault_transfer_complete(node, code);
		this->cwmp_add_inform_timer(10);
		return;
	}
	applyDownloadFile(d->file_type.c_str());
	getExecutionStatus(&status, &fault); // 获取执行下载后的结果状态
	if (fault > 0)
	{
		code = fault;
		Log(NAME, L_NOTICE, "download error: '%s'\n", fault_array[code].string);
		backup_update_fault_transfer_complete(node, code);
		this->cwmp_add_inform_timer(10);
		return;
	}
	backup_update_complete_time_transfer_complete(node);
	this->cwmp_add_inform_timer(10);
}

/**
 * 向cwmp中的uploads列表添加上传任务
 */
void cwmpInfo::cwmp_add_upload(std::string key, int delay, std::string upload_url, std::string file_type, std::string username, std::string password, tinyxml2::XMLElement *node)
{
	upload *ul = new upload;
	if (!ul)
		return;
	this->upload_count++;

	ul->key = key;
	ul->upload_url = upload_url;
	ul->file_type = file_type;
	ul->username = username;
	ul->password = password;
	ul->cancelFlag = false;
	ul->backup_node = node;
	ul->time_execute = time(NULL) + delay;
	this->uploads.push_back(ul);
	Log(NAME, L_NOTICE, "add upload: delay = %d sec, url = %s, FileType = '%s', CommandKey = '%s'\n", delay, ul->upload_url.c_str(), ul->file_type.c_str(), ul->key.c_str());
	// 创建定时线程去执行cwmp_upload_launch函数
	std::thread downloadThread(&cwmpInfo::cwmp_upload_launch, this, ul, 10);
	downloadThread.detach();
}

/**
 * 上传线程执行的函数
 */
void cwmpInfo::cwmp_upload_launch(upload *ul, int delay)
{
	std::this_thread::sleep_for(std::chrono::seconds(delay));
	if (ul->cancelFlag == true)
	{
		delete ul; // 释放下载事件存储的空间
		return;
	}
	Log(NAME, L_NOTICE, "start upload url = %s, FileType = '%s', CommandKey = '%s'\n", ul->upload_url.c_str(), ul->file_type.c_str(), ul->key.c_str());
	int code = FAULT_0;
	std::string start_time(get_time());
	uploadFile(ul->upload_url.c_str(), ul->file_type.c_str(), ul->username.c_str(), ul->password.c_str());
	backup_remove_node(ul->backup_node);
	this->uploads.remove(ul);
	this->upload_count--;
	tinyxml2::XMLElement *node = backup_add_transfer_complete(ul->key, code, start_time, ++this->method_id);
	if (!node)
	{
		delete ul;
		return;
	}
	this->cwmp_add_event(EVENT_TRANSFER_COMPLETE, "", 0, EVENT_BACKUP);
	this->cwmp_add_event(EVENT_M_UPLOAD, ul->key, this->method_id, EVENT_BACKUP);
	int status = 1, fault = FAULT_0;
	getExecutionStatus(&status, &fault);
	if (fault > 0)
	{ // 执行出错
		code = fault;
		Log(NAME, L_NOTICE, "upload error: '%s'\n", fault_array[code].string);
		backup_update_fault_transfer_complete(node, code);
		this->cwmp_add_inform_timer(10);
		return;
	}
	else if (status != 1)
	{ // 命令不成功
		code = FAULT_9002;
		Log(NAME, L_NOTICE, "upload error: '%s'\n", fault_array[code].string);
		backup_update_fault_transfer_complete(node, code);
		this->cwmp_add_inform_timer(10);
		return;
	}
	backup_update_complete_time_transfer_complete(node);
	this->cwmp_add_inform_timer(10);
}

/**
 * 设置一个 Inform 定时器，每隔 5 毫秒触发一次
 */
void cwmpInfo::cwmp_add_inform_timer(int64_t interval)
{
	std::thread timerThread(&cwmpInfo::cwmp_do_inform, this, interval);
	timerThread.detach(); // 将线程分离，使得线程可以自主运行
}

/**
 * 延迟interval毫秒执行Inform
 */
void cwmpInfo::cwmp_do_inform(int64_t interval)
{
	if (!this->inform_mtx.try_lock()) {
        // 获取锁失败，执行退出逻辑
		Log(NAME, L_NOTICE, "Failed to acquire lock. Exiting thread.\n");
        return;
    }
	// 等待5毫秒
	std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	this->retry_inform = false;
	cwmp_inform(); // 到点执行该函数
	this->inform_mtx.unlock();
}

/**
 * 重新启动会话需要等待的时间
 */
inline int cwmpInfo::cwmp_retry_count_interval(int retry_count)
{
	switch (retry_count)
	{
	case 0:
		return 0;
	case 1:
		return 7;
	case 2:
		return 15;
	case 3:
		return 30;
	case 4:
		return 60;
	case 5:
		return 120;
	case 6:
		return 240;
	case 7:
		return 480;
	case 8:
		return 960;
	case 9:
		return 1920;
	default:
		return 3840;
	}
}

/**
 * 重新尝试开启会话
 */
inline void cwmpInfo::cwmp_retry_session(void)
{
	int rp, retry = 1;
	for (auto it = this->events.begin(); it != this->events.end(); ++it)
	{
		rp = event_code_array[(*it)->code].remove_policy;
		if ((rp & EVENT_REMOVE_NO_RETRY))
		{
			retry = 0;
		}
		else
		{
			retry = 1;
			break;
		}
	}
	this->cwmp_remove_event(EVENT_REMOVE_NO_RETRY, 0);
	if (retry == 0 && this->retry_count == 0)
		return;
	this->retry_count++;
	int rtime = this->cwmp_retry_count_interval(this->retry_count);
	Log(NAME, L_NOTICE, "retry session in %d sec, RetryCount = %d\n", rtime, this->retry_count);
	std::thread inform_retry(&cwmpInfo::cwmp_do_inform_retry, this, rtime);
	inform_retry.detach();
}

/**
 * 重试，延迟一段时间后执行Inform
 */
void cwmpInfo::cwmp_do_inform_retry(int delaySeconds)
{
	this->retry_inform = true; // 在对象中标明正在尝试重启会话
	// 等待指定秒数
	std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));
	if (this->retry_inform)
		return; // 如果重试标志被取消了，则不重启会话
	this->retry_inform = false;
	cwmp_inform();
}

/**
 * 它用于发送 Inform 消息，并在接收到响应后解析消息内容
 */
inline int cwmpInfo::rpc_inform(void)
{
	std::string msg_in, msg_out; // 用于存储输入和输出的消息内容
	int error = 0, count = 0;	 // 用于计数循环次数

	if (xml_prepare_inform_message(msg_out))
	{ // 准备发送 Inform 消息，将消息存储在 msg_out 中。如果准备消息失败，则记录日志并返回 -1
		Log(NAME, L_DEBUG, "Inform xml message creating failed\n");
		return -1;
	}

	Log(NAME, L_NOTICE, "send Inform\n");

	do {
		// 发送 Inform 消息并接收响应
		if (http_send_message(msg_out, msg_in)) {//返回结果存在msg_in中(msg_in是一个引用参数)
			Log(NAME, L_DEBUG, "sending Inform http message failed\n");
			error = -1; break;
		}
		// 如果接收到空消息，则记录日志并设置错误标记
		if (msg_in.empty()) {
			Log(NAME, L_DEBUG, "parse Inform xml message from ACS: Empty message\n");
			error = -1; break;
		}
		// 解析响应消息，解析ACS的返回消息(xml格式)，返回成功0、错误码8005、解析错误-1
		// error = xml_parse_inform_response_message(msg_in);
		// if (error && (error != FAULT_ACS_8005)) {// 如果解析失败且错误码不是 FAULT_ACS_8005，则记录日志并设置错误标记
		// 	Log(NAME, L_DEBUG, "parse Inform xml message from ACS failed\n");
		// 	error = -1; break;
		// }
	} while(error && (count++)<10);//error=0表示成功，退出循环，当错误码存在且循环次数不超过10次时，重试发送和解析消息的过程
	//释放消息的内存，并返回错误码
	return error;
}

/**
 * 定时inform初始化
 */
void cwmpInfo::cwmp_periodic_inform_init(void)
{
	this->inform_num = (this->inform_num + 1) % 100; // 加一表示创建了一个新的inform循环定时器，原有的定时器会主动结束
	if (config->acs->periodic_enable && config->acs->periodic_interval)
	{ // 检查了是否启用了周期性通知以及周期性通知的间隔是否已经配置
		if (config->acs->periodic_time != -1)
		{ // 已设置了参考时间，记录日志并根据参考时间和间隔设置周期性通知定时器
			Log(NAME, L_NOTICE, "init periodic inform: reference time = %ld, interval = %d\n", config->acs->periodic_time, config->acs->periodic_interval);
			std::thread periodicInform(&cwmpInfo::cwmp_periodic_inform, this, (int)this->inform_num, this->cwmp_periodic_inform_time());
			periodicInform.detach(); // 将线程分离，使得线程可以自主运行
		}
		else
		{ // 如果未设置参考时间，记录日志并根据间隔设置周期性通知定时器
			Log(NAME, L_NOTICE, "init periodic inform: reference time = n/a, interval = %d\n", config->acs->periodic_interval);
			std::thread periodicInform(&cwmpInfo::cwmp_periodic_inform, this, (int)this->inform_num, config->acs->periodic_interval);
			periodicInform.detach(); // 将线程分离，使得线程可以自主运行
		}
	}
}

/**
 * 发送Inform消息的函数
 */
int cwmpInfo::cwmp_inform(void)
{
	// std::cout << "到点上报" << std::endl;
	tx::XMLElement *node;
	int method_id;

	auto handle_error = [this](const std::string &message)
	{
		Log(NAME, L_DEBUG, message.c_str());
		http_client_exit();
		// xml_exit();
		this->cwmp_handle_end_session();
		Log(NAME, L_NOTICE, "end session failed\n");
		this->cwmp_retry_session();
		return -1;
	};

	Log(NAME, L_NOTICE, "start session\n");
	if (http_client_init()) // 返回 1 表示初始化失败
	{
		return handle_error("initializing http client failed\n");
	}
	if (this->rpc_inform()) // 发送 Inform 消息，并在接收到响应后解析消息内容
	{
		return handle_error("sending Inform failed\n");
	}
	Log(NAME, L_NOTICE, "receive InformResponse from the ACS\n");

	// this->cwmp_remove_event(EVENT_REMOVE_AFTER_INFORM, 0); // 移除符合特定条件(参数)的事件节点
	// this->cwmp_clear_notifications();					   // 清空cwmp->notifications链表

	// do
	// {
	// 	// 循环发送http完成请求给ACS，直到拿到响应，或出错
	// 	// backup_check_transfer_complete: 在 backup_tree 中查找名为 "transfer_complete" 的节点, 并返回
	// 	while ((node = backup_check_transfer_complete()) && !this->hold_requests)
	// 	{													   // 在rpc_inform（）中的解析响应数据时获取了cwmp->hold_requests的值
	// 		if (this->rpc_transfer_complete(node, &method_id)) // 发送传输完成http给ACS，并查看响应是否有出错
	// 		{
	// 			return handle_error("sending TransferComplete failed\n");
	// 		}
	// 		Log(NAME, L_NOTICE, "receive TransferCompleteResponse from the ACS\n");

	// 		backup_remove_transfer_complete(node);									  // 释放传入节点的内存空间
	// 		this->cwmp_remove_event(EVENT_REMOVE_AFTER_TRANSFER_COMPLETE, method_id); // 移除符合特定条件(参数)的事件节点
	// 		if (!backup_check_transfer_complete())									  // 找不到名为 "transfer_complete" 的节点, 进入
	// 			this->cwmp_remove_event(EVENT_REMOVE_AFTER_TRANSFER_COMPLETE, 0);	  // 移除符合特定条件(参数)的事件节点
	// 	}
	// 	// 如果要获取RPC方法，且现在还未拿到ACS的响应
	// 	if (this->get_rpc_methods && !this->hold_requests)
	// 	{
	// 		if (this->rpc_get_rpc_methods()) // 发送getRPCMethods请求
	// 		{
	// 			return handle_error("sending GetRPCMethods failed\n");
	// 		}
	// 		Log(NAME, L_NOTICE, "receive GetRPCMethodsResponse from the ACS\n");

	// 		this->get_rpc_methods = false; // 修改为false，因为已经请求过了
	// 	}

	// 	if (this->cwmp_handle_messages())
	// 	{
	// 		return handle_error("handling xml message failed\n");
	// 	}
	// 	this->hold_requests = false;
	// } while (this->get_rpc_methods || backup_check_transfer_complete());

	// http_client_exit();
	// // xml_exit();
	// this->cwmp_handle_end_session();
	// this->retry_count = 0;
	// Log(NAME, L_NOTICE, "end session success\n");
	return 0;
}

/**
 * 从配置文件读取更新，数据文件的内容
 */
void cwmpInfo::cwmp_update_value_change(void)
{
	init_root(); // 必须先执行
	init_dataModel();
	std::string root(ROOT);
	std::string parameterPath;
	// device
	setParameter((root + parameterPath.assign(".DeviceInfo.SoftwareVersion")).c_str(), config->device->software_version.c_str(), NOVERIFY);

	// acs
	setParameter((root + parameterPath.assign(".ManagementServer.URL")).c_str(), config->acs->url.c_str(), NOVERIFY);
	setParameter((root + parameterPath.assign(".ManagementServer.Username")).c_str(), config->acs->username.c_str(), NOVERIFY);
	setParameter((root + parameterPath.assign(".ManagementServer.Password")).c_str(), config->acs->password.c_str(), NOVERIFY);
	setParameter((root + parameterPath.assign(".ManagementServer.PeriodicInformEnable")).c_str(), config->acs->periodic_enable ? "true" : "false", NOVERIFY);
	setParameter((root + parameterPath.assign(".ManagementServer.PeriodicInformInterval")).c_str(), std::to_string(config->acs->periodic_interval).c_str(), NOVERIFY);
	// setParameter((root + parameterPath.assign(".ManagementServer.PeriodicInformTime")).c_str(), config->acs->periodic_time, NOVERIFY);

	// local
	setParameter((root + parameterPath.assign(".DeviceInfo.IP")).c_str(), config->local->ip.c_str(), NOVERIFY);
}

/**
 * 定时调用自己，来实现定时上报
 */
void cwmpInfo::cwmp_periodic_inform(int inform_num_copy, long interval)
{
	// using std::chrono::operator""s;
	Log(NAME, L_DEBUG, "start thread %d\n", interval);
	if (config->acs->periodic_enable && config->acs->periodic_interval && this->inform_num == inform_num_copy)
	{
		std::this_thread::sleep_for(std::chrono::seconds(interval));
		std::thread periodicInform(&cwmpInfo::cwmp_periodic_inform, this, inform_num_copy, config->acs->periodic_interval);
		periodicInform.detach(); // 将线程分离，使得线程可以自主运行
		Log(NAME, L_DEBUG, "create a new thread\n");
	}
	if (config->acs->periodic_enable && this->inform_num == inform_num_copy)
	{
		this->cwmp_add_event(EVENT_PERIODIC, "", 0, EVENT_BACKUP);
		this->cwmp_add_inform_timer(10);
		Log(NAME, L_DEBUG, "extract the inform function\n");
	}
}

/**
 * 计算下一次周期性通知的时间间隔
 */
int cwmpInfo::cwmp_periodic_inform_time(void)
{
	int delta_time;	   // 用于存储当前时间与上次通知时间的差值
	int periodic_time; // 用于存储计算后的下一次通知的时间间隔

	delta_time = time(NULL) - config->acs->periodic_time; // 计算当前时间与上次通知时间的差值
	if (delta_time > 0)									  // 大于 0，表示当前时间晚于上次通知时间
		// 周期间隔 减去 当前时间与上次通知时间的模
		periodic_time = config->acs->periodic_interval - (delta_time % config->acs->periodic_interval);
	else // 表示当前时间早于或等于上次通知时间
		// 负的当前时间与周期间隔的模
		periodic_time = (-delta_time) % config->acs->periodic_interval;

	return periodic_time;
}

/**
 * 在会话结束时判断是否需要进行哪些操作
 */
void cwmpInfo::cwmp_handle_end_session(void)
{
	// external_action_simple_execute("apply", "service", NULL);
	if (this->end_session & ENDS_FACTORY_RESET)
	{
		Log(NAME, L_NOTICE, "end session: factory reset\n");
		// do_factory_reset();//恢复出厂
		exit(EXIT_SUCCESS);
	}
	if (this->end_session & ENDS_REBOOT)
	{
		Log(NAME, L_NOTICE, "end session: reboot\n");
		// do_reboot();//重启
		exit(EXIT_SUCCESS);
	}
	if (this->end_session & ENDS_RELOAD_CONFIG)
	{
		Log(NAME, L_NOTICE, "end session: configuration reload\n");
		config_load();
	}
	this->end_session = 0;
}
