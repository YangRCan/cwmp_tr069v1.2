/**
 * @Copyright : Yangrongcan
 */
#include <iostream>

#include "config.h"
#include "log.h"
#include "cwmp.h"
#include "cwmpclient.h"
#include "cJSON.h"

static bool first_run = true;
configInfo *config;
cJSON *config_root;

/**
 * 释放config
 */
void config_exit(void)
{
    if (config)
    {                         // 指针存在
        FREE(config->acs);    // 释放config->acs
        FREE(config->local);  // 释放config->local
        FREE(config->device); // 释放config->device
        FREE(config);         // 释放config指针指向的内存
    }
    if(config_root) cJSON_Delete(config_root);
}

/**
 * 为config及其成员指针 分配内存
 */
void config_load(void)
{
    // 错误处理函数
    auto handle_error = [=]()
    {
        Log(NAME, L_CRIT, "configuration (re)loading failed, exit daemon\n"); // 记录错误日志
        exit(EXIT_FAILURE);                                                   // 程序错误退出
    };

    config_init_package(); // 为config分配内存,读取配置文件到程序变量config_file中

    if (config_init_device())
        handle_error(); // 从配置文件中获取device的相关选项配置，并且赋值到 config->device 结构体中
    // if (config_init_local())
    //     handle_error(); // 从配置文件中获取local的相关选项配置，并且赋值到 config->local 结构体中
    // if (config_init_acs())
    //     handle_error(); // 从配置文件中获取acs的相关选项配置，并且赋值到 config->acs 结构体中

    // backup_check_acs_url(); // 检查是否已存在与 ACS URL 相关的节点，并检查其中的内容是否与配置中的 ACS URL 不一致
    // 检查是否已存在与 software_version 相关的节点，并检查其中的内容是否与配置中的 software_version 不一致, 若不一致，重构一个 "software_version" 的新节点，并更新配置文件中的值
    // backup_check_software_version();
    // cwmp->cwmp_periodic_inform_init(); // 定时inform初始化

    first_run = false;

    // cwmp->cwmp_update_value_change(); // 向子进程发送update_value_change命令
    return;
}

/**
 * 为config分配内存,读取配置文件到程序变量config_file中
 */
static void config_init_package(void)
{
    if (first_run)
    {                            // 首次运行，需要进行一些初始化操作
        config = new configInfo; // 使用 calloc 分配内存来存储 struct core_config 结构体，并将其赋值给 config 指针
        if (!config)
        {                  // 如果分配出错，执行出错处理
            config_exit(); // 调用 config_exit() 函数释放已分配的内存
            return;
        }

        config->acs = new acs; // 为 config 结构体中的 acs 成员分别分配内存空间
        if (!config->acs)
        {
            config_exit();
            return;
        }
        config->local = new local; // 为 config 结构体中的 local 成员分别分配内存空间
        if (!config->local)
        {
            config_exit();
            return;
        }
        config->device = new device; // 为 config 结构体中的 device 成员分别分配内存空间
        if (!config->device)
        {
            config_exit();
            return;
        }

        // 读取 JSON 文件内容到字符串
        FILE *file = fopen(CONFIGFILE, "r");
        if (!file) {
            Log(NAME, L_DEBUG, "Error opening file.\n");
            return;
        }

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        rewind(file);

        char *jsonData = (char *)malloc(fileSize + 1);
        if (!jsonData) {
            Log(NAME, L_DEBUG, "Error allocating memory.\n");
            fclose(file);
            return;
        }

        fread(jsonData, 1, fileSize, file);
        fclose(file);
        jsonData[fileSize] = '\0';

        // 解析 JSON 数据
        config_root = cJSON_Parse(jsonData);
        if (!config_root) {
            Log(NAME, L_DEBUG, "Error parsing JSON data.\n");
            free(jsonData);
            return;
        }
        free(jsonData);
    }
}

/**
 * 从配置文件中获取device的相关选项配置，并且赋值到 config->device 结构体中
*/
static int config_init_device(void)
{
    cJSON *deviceNode = cJSON_GetObjectItemCaseSensitive(config_root, "device");
    if (!deviceNode || !cJSON_IsObject(deviceNode)) {
        Log(NAME, L_DEBUG, "Error accessing device node.\n");
        cJSON_Delete(config_root);
        return -1;
    }
	cJSON *softwareVersionNode = cJSON_GetObjectItemCaseSensitive(deviceNode, "software_version");
    if (!softwareVersionNode || !cJSON_IsString(softwareVersionNode)) {
        Log(NAME, L_DEBUG, "Error accessing software_version field.\n");
        cJSON_Delete(config_root);
        return -1;
    }
	config->device->software_version = cJSON_GetStringValue(softwareVersionNode);
    std::cout << config->device->software_version << std::endl;

	Log(NAME, L_DEBUG, "json section device not found...\n");
	return 0;
}