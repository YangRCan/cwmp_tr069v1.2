/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "cJSON.h"
#include "config.h"
#include "log.h"
#include "cwmp.h"
#include "cwmpclient.h"
#include "backup.h"

using TimePoint = std::chrono::system_clock::time_point;

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
    if (config_root)
        cJSON_Delete(config_root);
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
    if (config_init_local())
        handle_error(); // 从配置文件中获取local的相关选项配置，并且赋值到 config->local 结构体中
    if (config_init_acs())
        handle_error(); // 从配置文件中获取acs的相关选项配置，并且赋值到 config->acs 结构体中

    cwmp->cwmp_init_deviceInfo(); // 读取设备信息到cwmp的deviceInfo成员中

    backup_check_acs_url(); // 检查是否已存在与 ACS URL 相关的节点，并检查其中的内容是否与配置中的 ACS URL 不一致
    // 检查是否已存在与 software_version 相关的节点，并检查其中的内容是否与配置中的 software_version 不一致, 若不一致，重构一个 "software_version" 的新节点，并更新配置文件中的值
    backup_check_software_version();
    cwmp->cwmp_periodic_inform_init(); // 定时inform初始化

    first_run = false;

    cwmp->cwmp_update_value_change(); // 从配置文件读取更新，数据文件的内容
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
        if (!file)
        {
            Log(NAME, L_DEBUG, "Error opening file.\n");
            return;
        }

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        rewind(file);

        char *jsonData = (char *)malloc(fileSize + 1);
        if (!jsonData)
        {
            Log(NAME, L_DEBUG, "Error allocating memory.\n");
            fclose(file);
            return;
        }

        fread(jsonData, 1, fileSize, file);
        fclose(file);
        jsonData[fileSize] = '\0';

        // 解析 JSON 数据
        config_root = cJSON_Parse(jsonData);
        if (!config_root)
        {
            Log(NAME, L_DEBUG, "Error parsing JSON data.\n");
            free(jsonData);
            return;
        }
        free(jsonData);
    }
}

/**
 * 从配置文件中读取信息到 cwmp 的成员 device 中
*/
void read_config_to_cwmp_deviceid(void) {
    cJSON *deviceNode = cJSON_GetObjectItemCaseSensitive(config_root, "device");
    if (!deviceNode || !cJSON_IsObject(deviceNode))
    {
        Log(NAME, L_DEBUG, "Error accessing device node.\n");
        cJSON_Delete(config_root);
    }
    std::string manufacturer, oui, product_class, serial_number;
    cJSON *child = deviceNode->child;
    while (child != nullptr)
    {
        std::string str = child->string;
        if (str == "manufacturer")
        {
            manufacturer = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "device.manufacturer=%s\n", manufacturer.c_str());
            child = child->next;
            continue;
        }
        
        if (str == "oui")
        {
            oui = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "device.oui=%s\n", oui.c_str());
            child = child->next;
            continue;
        }

        if (str == "product_class")
        {
            product_class = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "device.product_class=%s\n", product_class.c_str());
            child = child->next;
            continue;
        }

        if (str == "serial_number")
        {
            serial_number = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "device.serial_number=%s\n", serial_number.c_str());
            child = child->next;
            continue;
        }
        child = child->next;
    }

    cwmp->set_deviceid(manufacturer, oui, product_class, serial_number);
}

/**
 * 从配置文件中获取device的相关选项配置，并且赋值到 config->device 结构体中
 */
static int config_init_device(void)
{
    cJSON *deviceNode = cJSON_GetObjectItemCaseSensitive(config_root, "device");
    if (!deviceNode || !cJSON_IsObject(deviceNode))
    {
        Log(NAME, L_DEBUG, "Error accessing device node.\n");
        cJSON_Delete(config_root);
        return -1;
    }
    cJSON *softwareVersionNode = cJSON_GetObjectItemCaseSensitive(deviceNode, "software_version");
    if (!softwareVersionNode || !cJSON_IsString(softwareVersionNode))
    {
        Log(NAME, L_DEBUG, "Error accessing software_version field.\n");
        cJSON_Delete(config_root);
        return -1;
    }
    config->device->software_version = cJSON_GetStringValue(softwareVersionNode);
    Log(NAME, L_NOTICE, "device.software_version=%s\n", config->device->software_version.c_str());
    return 0;
}

/**
 * 从配置文件中获取local相关的配置，并赋值到 config->local 结构体中
 */
static int config_init_local(void)
{
    cJSON *localNode = cJSON_GetObjectItemCaseSensitive(config_root, "local");
    if (!localNode || !cJSON_IsObject(localNode))
    {
        Log(NAME, L_DEBUG, "Error accessing local node.\n");
        cJSON_Delete(config_root);
        return -1;
    }
    config->local->logging_level = DEFAULT_LOG_LEVEL;   // 默认日志等级，该宏在log.h中
    config->local->cr_auth_type = DEFAULT_CR_AUTH_TYPE; // 默认的认证类型为1，该宏在config.h中，摘要认证

    cJSON *child = localNode->child;
    while (child != NULL)
    {
        std::string str = child->string;
        // 从配置中提取不同的设置，比如接口、端口、用户名、密码等，并将其赋值给 config->local 结构体的相应成员
        if (str == "interface")
        {
            config->local->interfaceName = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "local.interface=%s\n", config->local->interfaceName.c_str());
            child = child->next;
            continue;
        }

        if (str == "url")
        {
            config->local->ip = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "local.ip=%s\n", config->local->ip.c_str());
            child = child->next;
            continue;
        }

        if (str == "port")
        {
            std::string portStr = cJSON_GetStringValue(child) ? cJSON_GetStringValue(child) : std::to_string(child->valueint);
            int port = std::stoi(portStr);
            std::string portString = std::to_string(port);
            if (portString != portStr)
            {
                Log(NAME, L_DEBUG, "in section local port has invalid value...\n");
                return -1;
            }
            config->local->port = portStr;
            Log(NAME, L_DEBUG, "local.port=%s\n", config->local->port.c_str());
            child = child->next;
            continue;
        }

        if (str == "username")
        {
            config->local->username = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "local.username=%s\n", config->local->username.c_str());
            child = child->next;
            continue;
        }

        if (str == "password")
        {
            config->local->password = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "local.password=%s\n", config->local->password.c_str());
            child = child->next;
            continue;
        }

        if (str == "logging_level")
        {
            std::string logLevelStr = cJSON_GetStringValue(child);
            int log_level = std::stoi(logLevelStr);
            std::string logLevelAsString = std::to_string(log_level);
            if (logLevelAsString == logLevelStr)
            {
                config->local->logging_level = log_level;
            }
            Log(NAME, L_DEBUG, "local.logging_level=%d\n", config->local->logging_level);
            child = child->next;
            continue;
        }

        if (str == "authentication")
        {
            std::string authentication = cJSON_GetStringValue(child);
            if (authentication == "Basic")
                config->local->cr_auth_type = AUTH_BASIC;
            else
                config->local->cr_auth_type = AUTH_DIGEST;
            Log(NAME, L_DEBUG, "local.authentication=%s\n", (config->local->cr_auth_type == AUTH_BASIC) ? "Basic" : "Digest");
            child = child->next;
            continue;
        }
        child = child->next;
    }
    // 检查必要的信息是否已设置，如果没有则记录日志并返回错误
    if (config->local->interfaceName.empty())
    {
        Log(NAME, L_DEBUG, "in local you must define interface\n");
        return -1;
    }

    if (config->local->port.empty())
    {
        Log(NAME, L_DEBUG, "in local you must define port\n");
        return -1;
    }
    return 0; // 成功初始化 local 配置
}

/**
 * 从配置文件中获取acs相关的配置，并赋值到 config->acs 结构体中
*/
static int config_init_acs(void)
{
    cJSON *acsNode = cJSON_GetObjectItemCaseSensitive(config_root, "acs");
    if (!acsNode || !cJSON_IsObject(acsNode))
    {
        Log(NAME, L_DEBUG, "Error accessing ACS node.\n");
        cJSON_Delete(config_root);
        return -1;
    }
	std::tm tm; //时间类型, 属于time.h

    config->acs->periodic_time = -1; //周期初始化为-1
    cJSON *child = acsNode->child;
    while (child != NULL)
    {
        std::string str = child->string;
        if (str == "url") {
            std::string value = cJSON_GetStringValue(child);
            bool valid = false; //是否属性值有效？
            if (value.compare(0, 5, "http:") == 0)
                valid = true; //有效
            if (value.compare(0, 6, "https:") == 0)
                valid = true; //有效
            if (!valid) { //若无效，记录错误日志
                Log(NAME, L_DEBUG, "in section acs scheme must be either http or https...\n");
                return -1;
            }

            config->acs->url = value; // 为 config->acs->url 分配内存并赋值
            Log(NAME, L_DEBUG, "acs.url=%s\n", config->acs->url.c_str()); //记录成功日志
            child = child->next;
            continue;
        }

        if (str == "username") {
            config->acs->username = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "acs.username=%s\n", config->acs->username.c_str());
            child = child->next;
            continue;
        }

        if (str == "password") {
            config->acs->password = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "acs.password=%s\n", config->acs->password.c_str());
            child = child->next;
            continue;
        }

        if (str == "periodic_enable") {
            config->acs->periodic_enable = (child->valueint == 1) ? true : false;
            Log(NAME, L_DEBUG, "acs.periodic_enable=%d\n", config->acs->periodic_enable);
            child = child->next;
            continue;
        }

        if (str == "periodic_interval") {
            config->acs->periodic_interval = child->valueint;
            Log(NAME, L_DEBUG, "acs.periodic_interval=%d\n", config->acs->periodic_interval);
            child = child->next;
            continue;
        }

        if (str == "periodic_time") {
            /**
             * " %FT%T" 是一个时间格式化字符串，指示了时间字符串的格式。
             * %FT 是 ISO 8601 格式中日期的表示法，F 表示完整的年份-月份-日期，T 表示时间字段的开始。
             * %T 是时间的表示法，包含小时-分钟-秒。
             * &tm 是指向 struct tm 结构体的指针，strptime() 将解析后的时间信息存储在这个结构体中。
            */
            const char* timeStr = cJSON_GetStringValue(child);
            TimePoint timePoint = parseTime(timeStr);
            // strptime(cJSON_GetStringValue(child),"%FT%T", &tm);
            /**
             * mktime() 函数用于将 struct tm 结构体表示的时间转换为日历时间（即时间戳）。它将 struct tm 结构体中的时间信息
             * 转换为从 Epoch（1970 年 1 月 1 日 00:00:00 UTC）开始计算的秒数。
             * 当你调用 mktime(&tm) 时，它会根据 tm 结构体中的年、月、日、时、分、秒等信息计算出相应的时间戳，并返回对应的时间戳值。
             * 例如，假设 tm 结构体中包含的时间是 2023 年 12 月 25 日 15:30:00，mktime(&tm) 将会返回 时间戳，表示从 Epoch 开始到那个时间的秒数
            */
            config->acs->periodic_time = std::chrono::system_clock::to_time_t(timePoint);
            Log(NAME, L_DEBUG, "acs.periodic_time=%s\n", cJSON_GetStringValue(child));
            child = child->next;
            continue;
        }

        if (str == "http100continue_disable") {
            config->acs->http100continue_disable = child->valueint ? true : false;
            Log(NAME, L_DEBUG, "acs.http100continue_disable=%d\n", config->acs->http100continue_disable);
            child = child->next;
            continue;
        }

        if (str == "ssl_cert") {
            config->acs->ssl_cert = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "acs.ssl_cert=%s\n", config->acs->ssl_cert.c_str());
            child = child->next;
            continue;
        }
        if (str == "ssl_cacert") {
            config->acs->ssl_cacert = cJSON_GetStringValue(child);
            Log(NAME, L_DEBUG, "acs.ssl_cacert=%s\n", config->acs->ssl_cacert.c_str());
            child = child->next;
            continue;
        }

        if (str == "ssl_verify") {
            std::string value = cJSON_GetStringValue(child);
            if (value == "enabled") {
                config->acs->ssl_verify = true;
            } else {
                config->acs->ssl_verify = false;
            }
            Log(NAME, L_DEBUG, "acs.ssl_verify=%d\n", config->acs->ssl_verify);
            child = child->next;
            continue;
        }
        child = child->next;
    }
    // 检查必要的信息是否已设置，如果没有则记录日志并返回错误
    if (config->acs->url.empty()) {
        Log(NAME, L_DEBUG, "acs url must be defined in the config\n");
        return -1;
    }

    return 0; // 成功初始化 acs 配置
}

/**
 * 修改本地配置
*/
// void config_modify() {

// }

/**
 * 解析时间字符串并返回时间点
*/
TimePoint parseTime(const char* timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        Log(NAME, L_DEBUG, "Invalid time string format");
        return TimePoint::min();
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}