/**
 * @Copyright : Yangrongcan
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

#include"parameter.h"

static struct Object *data;

void init_data() {
    data = malloc(sizeof(struct Object));
    init_object_struct(data);
    data->name = ROOT;
}

/**
 * 初始化Object对象，把成员变量全初始化
*/
void init_object_struct(struct Object *tmp) {
    tmp->name = NULL;

    tmp->childType = 0;//默认为Parameter类型
    tmp->child_object = NULL;
    tmp->child_parameter = NULL;

    tmp->siblingType = 0;
    tmp->sibling_object = NULL;
    tmp->sibling_parameter = NULL;

    tmp->placeholder = 0;

    tmp->ParameterKey = NULL;
}

/**
 * 获取所有属性
*/
void getAllParameters() {
    
}

/**
 * 获取某属性名对应的配置的值，
 * 参数 name 指的是某对应参数的完整路径
*/
void getParameter(char* path)
{
    
}

/**
 * 修改某属性名对应的配置的值
 * （注：前提这个属性允许被修改）
*/
void setParameter(char *path, char *value)
{
    
}

/**
 * 为data参数树添加一条新obj路径
 * 返回该新路径的最后一个Object对象
*/
struct Object *addObjectToData(char *path) {
    int length = strlen(path);
    if(path[length-1] != '.') return NULL;
    int count;
    const char *delimiter = ".";
    char **subString = GetSubstrings(path, delimiter, &count);
    int index = 0;

    struct Object *obj = findFinalMatchOBject(data, subString, count, &index);

    struct Object *newObj = createObject(subString, count, index + 1);
    
    struct Object *finalChild = findFinalChild(obj);
    if(finalChild == NULL) obj->child_object = newObj;
    else if(finalChild->placeholder >= 1){
        newObj->placeholder = finalChild->placeholder + 1;
        finalChild->sibling_object = newObj;
    } else finalChild->sibling_object = newObj;

    index = 0;
    obj = findFinalMatchOBject(data, subString, count, &index);
    return obj;
}

/**
 * 寻找与Object路径匹配的最长路径，返回匹配路径最后的Object对象
*/
struct Object *findFinalMatchOBject(struct Object *obj, char **str, const int count, int *index) {
    if(strcmp(obj->name, str[*index]) == 0 || *index >= count) {
        struct Object *tmp = NULL;
        ++(*index);
        //递归查看其子节点
        if(obj->child_object) tmp = findFinalMatchOBject(obj->child_object, str, count, index);
        // if(obj->sibling_object) tmp2 = findFinalMatchOBject(obj->sibling_object, str, count, index);
        return tmp ? tmp : obj;
    } else {
        struct Object *tmp = NULL;
        if(obj->sibling_object) tmp = findFinalMatchOBject(obj->sibling_object, str, count, index);
        return tmp;
    }
    return NULL;
}

/**
 * 递归创建object子树
*/
struct Object *createObject(char **str, const int count, const int index) {
    if(index >= count) return NULL;
    struct Object *obj = malloc(sizeof(struct Object));
    init_object_struct(obj);
    obj->name = str[index];
    if(isNumeric(str[index])) {
        obj->placeholder = 1;
    }

    if(count - 1 == index) return obj;

    obj->childType = ObjectType;
    obj->child_object = createObject(str, count, index+1);
    return obj;
}

/**
 * 判断一个参数树节点是否存在子节点
 * 并且返回子节点的最后一个（即其子节点链表的最后一个）
 * 否则返回NULL
*/
struct Object *findFinalChild(struct Object *obj) {
    if(obj->childType == ParameterType && obj->child_parameter == NULL) return NULL;//没有子节点（子树）
    // struct Object *tmp = obj->child_parameter;
    // if(obj->childType == ObjectType) tmp = obj->child_object;
    // while (tmp->siblingType != ParameterType || obj->sibling_parameter != NULL)
    // {
    //     if(tmp->siblingType == ParameterType) tmp = tmp->sibling_parameter;
    //     else tmp = tmp->sibling_object;
    // }
    return obj;
}

/**
 * 从字符串中提取出关键字，如“local.url.port”，提取出local，url，port,并且按原顺序排列
 * 返回二位字符数组
 */
char **GetSubstrings(const char *input, const char *delimiter, int *count)
{
    char *str = strdup(input); // 创建输入字符串的副本
    if (str == NULL)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    char **substrings = NULL;
    char *token = strtok(str, delimiter);
    *count = 0;

    while (token != NULL)
    {
        // 重新分配内存，它可以调整之前分配的内存块的大小
        substrings = realloc(substrings, (*count + 1) * sizeof(char *));
        if (substrings == NULL)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        substrings[*count] = strdup(token);
        if (substrings[*count] == NULL)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        (*count)++;
        token = strtok(NULL, delimiter);
    }

    free(str); // 释放副本字符串的内存
    return substrings;
}

/**
 * 判断字符串是否为数字
*/
bool isNumeric(const char *str) {
    int length = strlen(str);

    // 检查每个字符是否是数字
    for (int i = 0; i < length; i++) {
        if (!isdigit(str[i])) {
            return 0; // 如果有非数字字符，则返回0
        }
    }
    
    return 1; // 字符串中的所有字符都是数字
}