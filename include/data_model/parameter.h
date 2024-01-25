/**
 * @Copyright : Yangrongcan
*/
#ifndef _CWMP_PARAMETER_
#define _CWMP_PARAMETER_

#include<stdbool.h>

#define ROOT "Device" //数据模型的根Object

// 对parameter的权限
#define READONLY 0
#define WRITABLE 1
#define Notification_Off 0
#define Passive_Notification 1
#define Active_Notification 1

// 对Object的权限
#define PresentObject 0
#define CreateObject 1 //包括了AddObject和DeletteObject
#define AddObject 2
#define DeletteObject 3


typedef struct
{
    char *name;//参数名
    char *value;// 值
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
        占位符累计数量， 默认为 0，表示该节点的子节点不是占位符； 如果 ≥1，说明子节点是占位符{i},且肯定有Object类型子节点
        每添加子节点，占位符的值 i 加一后被直接写入到子节点的成员变量name中；
        此外，该层新添加节点时，name值应该为该值 +1。
        在程序重启后要从json文件中获取最大值
        若该值 ≥1，则子节点的类型必须为Object。
    */
    unsigned int placeholder;
    // 必须存在 PresentObject， 可创建和删除 CreateObject， 可创建 AddObject， 可删除 DeletteObject (已有宏定义)
    unsigned char limit;
    char *ParameterKey;
};

void init_data();
void init_object_struct(struct Object *tmp);


void getAllParameters();
void getParameter(char *path);
void setParameter(char *path, char *value);

int addObjectToData(char *path, unsigned char limit);
struct Object *createObject(struct Object *obj, const int index);
struct Object *findChildObject(struct Object *obj, const char *str);
char **GetSubstrings(const char *input);
void FreePATH();
void iterateData(struct Object *obj, char *str);
bool isNumeric(const char *str);

#endif