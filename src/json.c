/**
 * @Copyright : Yangrongcan
*/
/**
 * @Copyright : Yangrongcan
*/
#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#include "json.h"

int test_JSON() {
    printf("开始执行！\n");

    // // 创建一个 JSON 对象
    // cJSON *config = cJSON_CreateObject();

    // // 向 JSON 对象添加键值对
    // cJSON_AddStringToObject(config, "name", "John");
    // cJSON_AddNumberToObject(config, "age", 30);
    // cJSON_AddBoolToObject(config, "isStudent", 0);

    // // 将 JSON 对象转换为字符串
    // char *configStr = cJSON_Print(config);

    // // 输出 JSON 字符串到控制台
    // printf("Generated JSON:\n%s\n", configStr);

    // // 将 JSON 字符串写入文件
    // FILE *file = fopen("../config.json", "w");
    // if (file) {
    //     fprintf(file, "%s", configStr);
    //     fclose(file);
    // }

    // // 释放资源
    // cJSON_free(configStr);
    // cJSON_Delete(config);

    // 读取 JSON 配置文件
    // FILE *readFile = fopen("config.json", "r");
    // if (readFile) {
    //     fseek(readFile, 0, SEEK_END);//指针移动到文件末尾
    //     long fileSize = ftell(readFile);//获取文件末尾指针的下标
    //     fseek(readFile, 0, SEEK_SET);//指针移动到文件开头

    //     char *fileContent = (char *)malloc(fileSize + 1);//申请一块与（文件大小+1）一致的空间
    //     fread(fileContent, 1, fileSize, readFile);//读取整个文件
    //     fclose(readFile);//关闭文件

    //     fileContent[fileSize] = '\0';//添加‘\0’表示字符数组结束

    //     // 解析 JSON 文件内容
    //     cJSON *parsedConfig = cJSON_Parse(fileContent);
    //     if (parsedConfig != NULL) {
    //         // 从 JSON 中获取特定键的值并打印
    //         // cJSON *name = cJSON_GetObjectItem(parsedConfig, "name");
    //         // cJSON *age = cJSON_GetObjectItem(parsedConfig, "age");
    //         // cJSON *isStudent = cJSON_GetObjectItem(parsedConfig, "isStudent");

    //         cJSON *local = cJSON_GetObjectItemCaseSensitive(parsedConfig, "local");
    //         if(local != NULL) {
    //             cJSON *port = cJSON_GetObjectItemCaseSensitive(local, "port");
    //             if(port != NULL && cJSON_IsNumber(port)) {
    //                 printf("Port: %d\n", port->valueint);
    //             } else {
    //                 printf("Not found port!");
    //             }
    //         } else {
    //             printf("Not found local!");
    //         }
    //         // printf("Read from JSON file:\n");
    //         // printf("Name: %s\n", name->valuestring);
    //         // printf("Age: %d\n", age->valueint);
    //         // printf("Is Student: %s\n", (isStudent->valueint == 0) ? "false" : "true");

    //         cJSON *atm = cJSON_GetObjectItemCaseSensitive(parsedConfig, "atm");
    //         if(atm == NULL) {
    //             atm = cJSON_AddObjectToObject(parsedConfig, "atm");
    //             if(atm != NULL) {
    //                 cJSON_AddStringToObject(atm, "nested_key", "123");

    //                 FILE *writeFile = fopen("config.json", "w");
    //                 if(writeFile) {
    //                     char *updateConfig = cJSON_Print(parsedConfig);
    //                     fprintf(writeFile, "%s", updateConfig);
    //                     fclose(writeFile);
    //                     free(updateConfig);
    //                 }
    //             }
    //         }

    //         cJSON_Delete(parsedConfig);
    //     } else {
    //         printf("Parse Failure!");
    //     }
    //     free(fileContent);
    // } else {
    //     printf("Open file fail!");
    //     fclose(readFile);//关闭文件
    // }

    // printf("执行结束！\n");


    const char *jsonString = "{\"student\": {\"age\": 123, \"name\": \"Tom\"}}";

    cJSON *root = cJSON_Parse(jsonString);
    if (root == NULL) {
        printf("JSON 解析失败\n");
        return 1;
    }

    cJSON *student = cJSON_GetObjectItemCaseSensitive(root, "student");
    if (student == NULL) {
        printf("未找到 student 节点\n");
        cJSON_Delete(root);
        return 1;
    }

    // 删除 age 节点
    cJSON_DeleteItemFromObject(student, "age");

    // 添加 age 节点
    cJSON_AddStringToObject(student, "age", "13");

    char *modifiedJsonString = cJSON_Print(root);
    printf("修改后的 JSON 字符串: %s\n", modifiedJsonString);

    free(modifiedJsonString);
    cJSON_Delete(root);
    printf("good");
    return 0;
}