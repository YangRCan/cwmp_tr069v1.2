/**
 * @Copyright : Yangrongcan
*/
#ifndef _CWMP_PARAMETER_
#define _CWMP_PARAMETER_

#include<stdbool.h>

#define ROOT "Device"

#define ParameterType 0
#define ObjectType 1
#define NOT_WRITABLE 0
#define WRITABLE 1
#define Notification_Off 0
#define Passive_Notification 1
#define Active_Notification 1

typedef struct
{
    char *name;//参数名
    char *value;// 值
    bool writable;// 1 表示可写， 0 表示不可写(已有宏定义)
    int notification;// 0 表示通知关闭， 1 表示被动通知， 2 表示主动通知(已有宏定义)
    /*
        值类型包括：string、int、unsignedInt、boolean、dateTime、base64、anySimpleType
    */
    char *valueType;// 值类型
} Parameter;

struct Object
{
    char *name;//对象名
    // 孩子
    Parameter *child_parameter;
    struct Object *child_object;
    bool childType;//0 为Parameter类型， 1 为Object类型(已有宏定义)

    //兄弟
    Parameter *sibling_parameter;
    struct Object *sibling_object;
    bool siblingType;//0 为Parameter类型， 1 为Object类型(已有宏定义)

    /*
        占位符累计数量， 默认为 0，表示这一层(该节点与其所有兄弟)不是占位符, 只是普通的Object路径； 如果 ≥1，说明这一层是占位符{i}, 若该点被删除，则应该将该值赋值给其兄弟
        占位符的值 i 被直接写入到成员变量name中；
        删除某该层某节点时，该值不变；若这一层占符被全被删除，则丢弃该值；
        此外，该层新添加节点时，name值应该为该值 +1。
        若该值 ≥1，则兄弟节点的类型必须为Object。
    */
    int placeholder;

    char *ParameterKey;
};

void init_data();
void init_object_struct(struct Object *tmp);


void getAllParameters();
void getParameter(char *path);
void setParameter(char *path, char *value);

struct Object *addObjectToData(char *path);
struct Object *findFinalMatchOBject(struct Object *obj, char **str, const int count, int *index);
struct Object *createObject(char **str, const int count, const int index);
struct Object *findFinalChild(struct Object *obj);
char **GetSubstrings(const char *input, const char *delimiter, int *count);
bool isNumeric(const char *str);

#endif