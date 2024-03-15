/**
 * @Copyright : Yangrongcan
*/
#ifndef _CWMP_LOG_
#define _CWMP_LOG_

#define DEFAULT_LOG_LEVEL 3 //默认日志等级

//日志等级的枚举
enum {
    L_CRIT,
    L_WARNING,
	L_NOTICE,
	L_INFO,
	L_DEBUG
};

void Log(const char *name, int priority, const char *format, ...);

#endif