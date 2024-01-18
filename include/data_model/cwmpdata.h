/**
 * @Copyright : Yangrongcan
*/
#ifndef _CWMP_SIMPLE_
#define _CWMP_SIMPLE_

#include<stdbool.h>
#define HELP_INFO                                                        \
    "-Help\n"                                                            \
    "Please use this program with parameters in the following format:\n" \
    "\t-----\tget parameter\n"                                            \
    "\t-----\tset parameter value\n"                                      \
    "\t-----\tget all"

#define ParameterType 0
#define ObjectType 1
#define NOTWRITABLE 0
#define WRITABLE 1

typedef struct
{
    const char *key;
    void (*function)();
} Operate;

typedef struct
{
    char *name;//参数名
    char *value;// 值
    bool writable;// 1 表示可写， 0 表示不可写(已有宏定义)
    char *valueType;// 值类型
} Parameter;

typedef struct Object
{
    char *name;//对象名
    // 孩子
    Parameter *child_parameter;
    struct Object *child_object;
    bool childType;//0 为Parameter类型， 1 为Object类型(已有宏定义)

    //兄弟
    Parameter *brother_parameter;
    struct Object *brother_object;
    bool broderType;//0 为Parameter类型， 1 为Object类型(已有宏定义)


};



void init();
void getAllParameters();
void getParameter(char *name);
void setParameter(char *name, char *value);
void printHelp();

extern Operate options[];

#endif