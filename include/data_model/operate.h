#ifndef _CWMP_OPERATE_
#define _CWMP_OPERATE_

#include<cjson/cJSON.h>


extern cJSON* rootJSON;

bool init_root();
bool save_data();


#endif