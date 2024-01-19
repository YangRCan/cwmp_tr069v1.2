#ifndef _CWMP_OPERATE_
#define _CWMP_OPERATE_

#include<cjson/cJSON.h>

#define DATAFILE '../data_model.json'

extern cJSON* rootJSON;

bool init_root();
bool save_data();


#endif