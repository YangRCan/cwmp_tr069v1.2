/**
 * @Copyright : Yangrongcan
 */
#include <time.h>
#include <stdio.h>
#include "time_tool.h"

static char local_time[32] = {0};
/**
 * 获取当前时间并将其格式化为指定的字符串格式
 */
char *get_time(void)
{
	time_t t_time;
	struct tm *t_tm;

	t_time = time(NULL);
	t_tm = localtime(&t_time);
	if (t_tm == NULL)
		return NULL;

	if (strftime(local_time, sizeof(local_time), "%Y-%m-%dT%H:%M:%S", t_tm) == 0)
		return NULL;

	local_time[25] = local_time[24];
	local_time[24] = local_time[23];
	local_time[22] = ':';
	local_time[26] = '\0';
	return local_time;
}