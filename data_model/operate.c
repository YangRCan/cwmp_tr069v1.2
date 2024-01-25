#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include"operate.h"
#include"faultCode.h"

cJSON* rootJSON;

/**
 * 初始化数据,从文件中读取数据，若无则创建一个空的cJSON实例
*/
bool init_root() {
    FILE *file= fopen(DATAFILE, "r");
    if(file == NULL) {
        printf("打开文件失败!");
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if(fileSize > 0) {
        char* jsonString = (char*)malloc(fileSize + 1);
        fread(jsonString, 1, fileSize, file);
        jsonString[fileSize] = '\0';

        rootJSON = cJSON_Parse(jsonString);
    } else {
        rootJSON = cJSON_CreateObject();
    }

    fclose(file);//关闭文件

    return 1;
}

/**
 * 将数据保存到文件中
*/
bool save_data() {
    FILE* file = fopen(DATAFILE, "w");
    if(file == NULL) {
        printf("打开文件失败!");
        return 0;
    }

    char* jsonString = cJSON_Print(rootJSON);//转成字符串
    fprintf(file, "%s", jsonString);//写入
    fclose(file);//关闭
    free(jsonString);//释放字符串空间

    return 1;
}

// int addObjectToData() {

// }

// int addParamToData() {

// }

// cJSON *findFinalMatchObject() {

// }

// cJSON *createObject() {
    
// }