/**
 * @Copyright : Yangrongcan
*/
#include<stdio.h>
#include<time.h>
#include<stdarg.h>//标准 C 库，提供处理变长参数的功能

#ifdef __linux__
#include<syslog.h>

static const int log_class[] = {// 日志级别对应系统日志级别
	[L_CRIT] = LOG_CRIT,// 数组 log_class 的索引 L_CRIT 对应 LOG_CRIT 宏定义的值
	[L_WARNING] = LOG_WARNING,//这些宏在<syslog.h>中
	[L_NOTICE] = LOG_NOTICE,
	[L_INFO] = LOG_INFO,
	[L_DEBUG] = LOG_DEBUG
};
#endif

#ifdef _WIN32
#include<Windows.h>
#endif

#include"log.h"


static const char* log_str[] = {
	[L_CRIT] = "CRITICAL",// 数组 log_str 的索引 L_CRIT 对应 "CRITICAL" 字符串
	[L_WARNING] = "WARNING",
	[L_NOTICE] = "NOTICE",
	[L_INFO] = "INFO",
	[L_DEBUG] = "DEBUG"
};



void Log(char *name, int priority, const char *format, ...) {
    va_list vl;	// 保存变长参数的结构体

#ifdef __linux__ //在Linux平台下
    // 打开系统日志
    openlog(name, 0, LOG_DAEMON);
    va_start(vl, format);
    // 记录日志到系统日志
    vsyslog(log_class[priority], format, vl);//这将打印到系统日志/var/log/syslog中
    va_end(vl);
    closelog();//关闭系统日志
#elif _WIN32 //在win平台下
    time_t t = time(NULL);// 获取当前时间
    struct tm tm = *localtime(&t);//将日历时间转换为本地时间
    va_start(vl, format);
    char buffer[1024], result[1024];
    snprintf(buffer, sizeof(buffer), "%d-%02d-%02d %02d:%02d:%02d [%s] %s : %s", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, name, log_str[priority], format);
    vsnprintf(result, sizeof(result), buffer, vl);
    OutputDebugStringA(result); // 将日志信息输出到调试器
    // printf("%s\n",result);
    va_end(vl);
#else //其他
    time_t t = time(NULL);// 获取当前时间
    struct tm tm = *localtime(&t);//将日历时间转换为本地时间
    va_start(vl, format);// va_list 是一个用于访问变长参数的类型
    // 打印日志信息到标准输出
    printf("%d-%02d-%02d %02d:%02d:%02d [%s] %s : ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, name,log_str[priority]);
    vprintf(format, vl);// vprintf 用于按照格式输出变长参数
    va_end(vl);
#endif
}