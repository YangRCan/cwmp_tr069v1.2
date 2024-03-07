/**
 * @Copyright : Yangrongcan
*/
#ifndef _CWMP_FAULTCODE_
#define _CWMP_FAULTCODE_

#define UNKNOWN_TIME "0001-01-01T00:00:00Z"

/**
 * 失败类型的枚举
*/
enum notify {
	FAULT_0,	// 没错误
	FAULT_9000, // 方法不支持
	FAULT_9001, // 请求被拒绝
	FAULT_9002, // 内部错误
	FAULT_9003, // 无效参数
	FAULT_9004, // 超出了资源
	FAULT_9005, // 无效的参数名
	FAULT_9006, // 无效的参数类型
	FAULT_9007, // 无效的参数值
	FAULT_9008, // 企图修改不可写的参数
	FAULT_9009, // 通知请求被拒绝
	FAULT_9010, // 文件下载失败
	FAULT_9011, // 文件上传失败
	FAULT_9012, // 文件传输服务器身份验证失败
	FAULT_9013, // 不支持的文件传输协议
	FAULT_9014, // 文件下载失败：无法加入多播组
	FAULT_9015, // 文件下载失败：无法连接文件服务器
	FAULT_9016, // 文件下载失败：无法访问文件
	FAULT_9017, // 文件下载失败：无法完成下载
	FAULT_9018, // 文件下载失败：文件已顺坏或无法使用
	FAULT_9019, // 文件下载失败：文件验证失败
	__FAULT_MAX
};

/**
 * 失败结构体
*/
typedef struct
{
	char *code;
	char *type;
	char *string;
} fault_code;

extern fault_code fault_array[];

void printErrorInfo(int faultCode);

#endif