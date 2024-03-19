/**
 * @Copyright : Yangrongcan
 */

#if !defined(_CWMP_CONFIG_)
#define _CWMP_CONFIG_

#include <string>

// 默认验证方式设置为 AUTH_DIGEST，即默认的认证类型为 1 摘要认证
#define DEFAULT_CR_AUTH_TYPE AUTH_DIGEST;
#define CONFIGFILE "./config/config.json"

// 定义了一个认证类型的枚举
enum auth_type_enum
{
    AUTH_BASIC,
    AUTH_DIGEST
};

// 分别定义了设备信息、ACS（Auto Configuration Server）信息和本地信息的结构体
struct device
{
    std::string software_version;
};

struct acs
{
    std::string url;
    std::string username;
    std::string password;
    bool periodic_enable;
    bool http100continue_disable;
    int periodic_interval;
    time_t periodic_time;
    std::string ssl_cert;
    std::string ssl_cacert;
    bool ssl_verify;
};

struct local
{
    std::string ip;
    std::string port;
    std::string _interface;
    std::string username;
    std::string password;
    std::string ubus_socket;
    int logging_level;
    int cr_auth_type;
};

struct configInfo
{
    device *device;
    acs *acs;
    local *local;
};

void config_exit(void);
void config_load(void);

static void config_init_package(void);
static int config_init_device(void);

extern configInfo *config;

#endif // _CWMP_CONFIG_
