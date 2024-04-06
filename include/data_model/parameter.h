/**
 * @Copyright : Yangrongcan
 */
#ifndef _CWMP_PARAMETER_
#define _CWMP_PARAMETER_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"{
#endif

#include <stdbool.h>
#include <cJSON.h>

#define DATAFILE "./data.json"

// 对parameter的权限
#define READONLY 0
#define WRITABLE 1
#define Notification_Off 0     // 通知关闭
#define Passive_Notification 1 // 被动通知，值变化时，新值包括在下次向ACS建立会话时发送Inform消息的ParameterList中。
#define Active_Notification 2  // 主动通知。值每发生变化，CPE必须启动到ACS的会话，并在相关的Inform消息的ParameterList中包括新值。

// 对Object的权限
#define PresentObject 0 // 必须存在
#define CreateObject 1  // 包括了AddObject和DeletteObject, 可创建和删除
#define AddObject 2     // 可创建
#define DeletteObject 3 // 可删除

enum verifyParameter {
    NOVERIFY = 0,
    MUSTVERIFY
};

typedef struct
{
    char *name; // 参数名
    // char *value;// 值
    unsigned char writable;     // 1 表示可写， 0 表示只读(已有宏定义)，若为只读则不需要保存到JSON文件
    unsigned char notification; // 0 表示通知关闭， 1 表示被动通知， 2 表示主动通知(已有宏定义)
    /**
     * 为其授予对指定参数的写访问权限的零个或多个实体的数组
     * 即有哪些方式可以修改参数的值，如ACS，Lan侧接口
     * 目前，只定义了一种类型的实体，可以包括在此列表中：
     *  “Subscriber”，表示通过用户LAN上控制的接口进行写访问。
     * 目前，没有指定其他WAN端配置协议的访问限制。
     * 默认情况下，在ACS对访问列表进行任何更改之前，应向上面指定的所有实体授予访问权限。
     * TR-069 ACS总是具有对所有可写参数的写访问权，而不管是否在访问列表上。
     */
    char **AccessList;
    /*
        值类型包括：string、int、unsignedInt、boolean、dateTime、base64、anySimpleType
    */
    char *valueType;    // 值类型
    void (*function)(); // 与该参数相关的函数
} Parameter;

struct Object
{
    char *name; // 对象名
    // 孩子
    Parameter *child_parameter;  // 数组
    struct Object *child_object; // 数组

    size_t NumOfParameter, NumOfObject;

    /**
     * 1 表示可写， 0 表示只读(已有宏定义)
     * 表示AddObject是否可用于添加此对象的新实例。（不包括实例号）
     * 表示DeleteObject是否可用于删除此特定实例。（包括实例号）
     */
    unsigned char writable;
    /*
        下一个可用的占位符序号， 默认为 0，表示该节点的子节点不是占位符； 如果 ≥1，说明子节点是占位符{i},且肯定有Object类型子节点
        每添加子节点，占位符的值 i 被直接写入到子节点的成员变量name中；
        此外，该层添加实例时，该变量+1
        在程序重启后要从json文件中获取该值
    */
    unsigned int nextPlaceholder;
    // 必须存在 PresentObject， 可创建和删除 CreateObject， 可创建 AddObject， 可删除 DeletteObject (已有宏定义)
    unsigned char limit;
    char *ParameterKey;
    void (*function)(); // 与该参数相关的函数
};

typedef struct
{
    int fault_code;//失败代码
    bool status;//执行状态，1：成功，0：失败
} ExecuteResult;


// ParameterInfoStruct定义
typedef struct
{
    char *name;             // 可为部分路径或完整路径
    unsigned char writable; // 若为部分路径即Object，则表示是否可使用addObject添加实例，即该路径的下一Object是否为占位符
} ParameterInfoStruct;

// ParameterAttributeStruct结构定义
typedef struct
{
    char *Name;
    int Notification;
    char **AccessList;
} ParameterAttributeStruct;

typedef struct
{
    char *data;
    char *name;
    char *type;
	int fault_code;
} param_info;

// 初始化对应的函数
void init_dataModel();
void init_object_struct(struct Object *tmp);
void init_parameter_struct(Parameter *param);
void set_parameter_struct(Parameter *param, char *name, unsigned char writable, unsigned char notification, char *valueType, void (*function)());

// 具体操作对应的函数
void getAllParameters();
ExecuteResult setParameter(const char *path, const char *value, int verify);
ExecuteResult getParameter(const char *path, param_info ***param_list);
ExecuteResult getParameterNames(const char *path, const char *NextLevel, ParameterInfoStruct ***parameterList);
ExecuteResult setParameterAttributes(ParameterAttributeStruct *parameterAttribute, const bool NotificationChange, const bool AccessListChange, int numOfAccess);
ParameterAttributeStruct **getParameterAttributes(const char *const *ParameterNames, const int numOfParameter, ExecuteResult *exe_rlt);
ExecuteResult addObject(const char *path, char **instanceNumber);
ExecuteResult deleteObject(const char *path);
ExecuteResult downloadFile(const char *url, const char *fileType, const char *fileSize, const char *username, const char *password);
ExecuteResult applyDownloadFile(const char *fileType);
ExecuteResult uploadFile(const char *url, const char *fileType, const char *username, const char *password);
param_info **getInformParameter();
ExecuteResult do_factory_reset();
ExecuteResult do_reboot();

// 数据模型相关的函数
int addObjectToDataModel(char *path, const unsigned char writable, const unsigned char limit, void (*function)());
struct Object *createObjectToDataModel(struct Object *obj, const int index);
struct Object *findChildObject(struct Object *obj, const char *str);
int addParameterToDataModel(char *path, char *value, unsigned char writable, unsigned char notification, char *valueType, void (*function)());
void freePath(char **path, const int count);
void iterateDataModel(struct Object *obj, char *str);
struct Object *checkObjectPath();
int checkParameterPath();
int addObjectToData(char **instanceNumber);
unsigned char getWritable(const char *path);

// 操作JSON数据文件的函数
bool init_root();
ExecuteResult save_data();
cJSON *createObjectPathToJsonData();
void createParameterPathToJsonData(Parameter *param, char *value);
void setParameterAttributesToJsonData(cJSON *node, Parameter *param, const char *value);
char *createObjectToJsonData(struct Object *placeholder);
void objectInstanceAttributeSupplementation(cJSON *node, struct Object *obj);
int getPlaceholderMaxNum(cJSON *node);
cJSON *getParameterJSON();
ParameterInfoStruct **getChildFromJson(const char *path);
void getDescendantsFromJson(const char *path, cJSON *object, ParameterInfoStruct ***List, int *const index);
ParameterAttributeStruct **getAttributesFromJson(const char *parameter);
param_info **getInfoParamFromJson(const char *parameter);
param_info **getParameterlistByJsonNode(cJSON *node, const char *p_name);
void printAllParameters(cJSON *jsonObj, char *str);

// 类型转换或判断等相关的函数
char **getSubStrings(const char *input, int *count);
char *concatenateStrings(const char *str1, const char *str2);
bool isNumeric(const char *str);
int download_file_to_dir(const char *url, const char *username, const char *password, const char *dir);


#if defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif