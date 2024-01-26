/**
 * @Copyright : Yangrongcan
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

#include"parameter.h"
#include"faultCode.h"

static struct Object *data;

char **PATH;
int count;
const char *delimiter = ".";

void init_data() {
    data = malloc(sizeof(struct Object));
    init_object_struct(data);
    data->name = ROOT;
}

/**
 * 初始化Object对象，把成员变量全初始化
*/
void init_object_struct(struct Object *obj) {
    obj->name = NULL;

    obj->child_object = NULL;
    obj->child_parameter = NULL;
    obj->NumOfObject = 0;
    obj->NumOfParameter = 0;

    obj->placeholder = 0;
    obj->limit = PresentObject;
    obj->ParameterKey = NULL;
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
int addObjectToData(char *path, unsigned char limit) {
    int length = strlen(path);
    if(path[length-1] != '.') {
        printf("该路径不为Object路径!{%s}, {%s}", fault_array[FAULT_9003].code, fault_array[FAULT_9003].string);
        return FAULT_9003;
    }
    PATH = GetSubstrings(path);
    if(strcmp(data->name, PATH[0]) != 0) {
        printf("根元素不匹配, 路径错误!{%s}, {%s}", fault_array[FAULT_9003].code, fault_array[FAULT_9003].string);
        return FAULT_9003;
    }
    
    if(path[length - 2] == '}') count--;//扣除后面的占位符

    struct Object *obj = createObject(data, 1);
    obj->limit = limit;
    if(path[length - 2] == '}') {
        obj->placeholder = 1;
        count++;//因为释放空间时需要知道具体有多少个字符串，所以要还原
    }

    FreePATH();
    return FAULT_0;
}

/**
 * 递归创建object子树, 返回最底层的Object节点
*/
struct Object *createObject(struct Object *obj, const int index) {
    if(index >= count) return obj;
    struct Object *tmp = findChildObject(obj, PATH[index]);

    if(!tmp) {
        obj->NumOfObject++;
        obj->child_object = realloc(obj->child_object, obj->NumOfObject * sizeof(struct Object));
        tmp = &(obj->child_object[obj->NumOfObject - 1]);
        init_object_struct(tmp);
        tmp->name = strdup(PATH[index]);
    }

    return createObject(tmp, index + 1);
}

/**
 * 在Object节点中查找路径名称等于str的节点并返回该子节点，不存在则返回NULL
*/
struct Object *findChildObject(struct Object *obj, const char *str) {
    struct Object *tmp = NULL;
    for (size_t i = 0; i < obj->NumOfObject; i++)
    {
        if(strcmp(obj->child_object[i].name, str) == 0) {
            // printf("现在儿子有： %s\n", obj->child_object[i].name);
            tmp = &(obj->child_object[i]);
        }
    }
    return tmp;
}

/**
 * 从字符串中提取出关键字，如“local.url.port”，提取出local，url，port,并且按原顺序排列
 * 返回二位字符数组
 */
char **GetSubstrings(const char *input)
{
    char *str = strdup(input); // 创建输入字符串的副本
    if (str == NULL)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    char **substrings = NULL;
    char *token = strtok(str, delimiter);
    count = 0;

    while (token != NULL)
    {
        // 重新分配内存，它可以调整之前分配的内存块的大小
        substrings = realloc(substrings, (count + 1) * sizeof(char *));
        if (substrings == NULL)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        substrings[count] = strdup(token);
        if (substrings[count] == NULL)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        (count)++;
        token = strtok(NULL, delimiter);
    }

    free(str); // 释放副本字符串的内存
    return substrings;
}

/**
 * 释放全局变量PATH的空间
*/
void FreePATH() {
    if(PATH) {
        for (size_t i = 0; i < count; ++i) {
            free(PATH[i]); // 释放每个子字符串
        }
        free(PATH); // 释放存储指针的数组
        PATH = NULL;
    }
}

/**
 * 输出数据模型树中所有的路径
*/
void iterateData(struct Object *obj, char *str) {
    if(obj == NULL) {
        iterateData(data, NULL);
        return;
    }
    char *destination = NULL;
    size_t totalLength = 0;

    if (str) totalLength += strlen(str);
    if (obj->name) totalLength += strlen(obj->name);
    totalLength += 2;

    destination = (char *)malloc(totalLength);
    if (destination == NULL) {
        perror("Memory allocation failed");
        return;
    }
    destination[0] = '\0';

    if(str) strcat(destination, str);
    if(obj->name) strcat(destination, obj->name);
    strcat(destination, ".");
    if(obj->NumOfObject <= 0) printf("%s\n", destination);

    for (size_t i = 0; i < obj->NumOfObject; i++)
    {
        iterateData(&(obj->child_object[i]), destination);
    }
    free(destination);
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