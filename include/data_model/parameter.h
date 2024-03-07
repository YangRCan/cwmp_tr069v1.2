/**
 * @Copyright : Yangrongcan
*/
#ifndef _CWMP_PARAMETER_
#define _CWMP_PARAMETER_

#include<stdbool.h>
#include<cjson/cJSON.h>

#define DATAFILE "../data.json"

// 对parameter的权限
#define READONLY 0
#define WRITABLE 1
#define Notification_Off 0
#define Passive_Notification 1
#define Active_Notification 2

// 对Object的权限
#define PresentObject 0 //必须存在
#define CreateObject 1 //包括了AddObject和DeletteObject, 可创建和删除
#define AddObject 2 //可创建
#define DeletteObject 3 //可删除


typedef struct
{
    char *name;//参数名
    // char *value;// 值
    unsigned char writable;// 1 表示可写， 0 表示不可写(已有宏定义)，若为只读则不需要保存到data_model
    unsigned char notification;// 0 表示通知关闭， 1 表示被动通知， 2 表示主动通知(已有宏定义)
    /*
        值类型包括：string、int、unsignedInt、boolean、dateTime、base64、anySimpleType
    */
    char *valueType;// 值类型
    void (*function)(); //与该参数相关的函数
} Parameter;

struct Object
{
    char *name;//对象名
    // 孩子
    Parameter *child_parameter; //数组
    struct Object *child_object; //数组

    size_t NumOfParameter, NumOfObject;

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
    void (*function)(); //与该参数相关的函数
};

// ParameterInfoStruct定义
typedef struct 
{
    char *name; // 可为部分路径或完整路径
    bool writable; // 若为部分路径即Object，则表示是否可使用addObject添加实例，即该路径的下一Object是否为占位符
} ParameterInfoStruct;


// 初始化对应的函数
void init_dataModel();
void init_object_struct(struct Object *tmp);
void init_parameter_struct(Parameter *param);
void set_parameter_struct(Parameter *param, char *name, unsigned char writable, unsigned char notification, char *valueType, void (*function)());

// 具体操作对应的函数
void getAllParameters();
void getParameter(char *path, char** str);
void getParameterName(char *path, char *NextLevel, ParameterInfoStruct ***parameterList);
void setParameter(char *path, char *value);
void addObject(char *path);

// 数据模型相关的函数
int addObjectToDataModel(char *path, unsigned char limit, void (*function)());
struct Object *createObjectToDataModel(struct Object *obj, const int index);
struct Object *findChildObject(struct Object *obj, const char *str);
int addParameterToDataModel(char *path, unsigned char writable, unsigned char notification, char *valueType, void (*function)());
void FreePATH();
void iterateDataModel(struct Object *obj, char *str);
int checkObjectPath();
int checkParameterPath();
int addObjectToData();


// 操作JSON数据文件的函数
bool init_root();
bool save_data();
cJSON* createObjectPathToJsonData();
void createParameterPathToJsonData();
void createObjectToJsonData(struct Object *placeholder);
void ObjectInstanceAttributeSupplementation(cJSON *node, struct Object *obj);
int GetPlaceholderMaxNum(cJSON *node);
cJSON* getParameterJSON();
void printAllParameters(cJSON *jsonObj, char *str);

// 类型转换或判断等相关的函数
char **GetSubstrings(const char *input);
char *concatenateStrings(const char *str1, const char *str2);
bool isNumeric(const char *str);

#endif