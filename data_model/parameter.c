/**
 * @Copyright : Yangrongcan
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parameter.h"
#include "faultCode.h"
#include "tr181.h"
#include "tr135.h"

static struct Object *dataModel;
static cJSON *rootJSON;

static char **PATH;                 // 完整路径
static int count;                   // 路径节点数
static const char *delimiter = "."; // 路径分隔符

/**######################
##                     ##
##     初始化函数       ##
##                     ##
######################**/

/**
 * 初始化DataModel的函数(该变量属于全局static)
 */
void init_dataModel()
{
    dataModel = malloc(sizeof(struct Object));
    init_object_struct(dataModel);
    dataModel->name = ROOT;

    // 此处添加入数据模型初始化函数
    init_tr181_object();
    init_tr181_parameter();
}

/**
 * 初始化Object对象，把成员变量全初始化
 */
void init_object_struct(struct Object *obj)
{
    obj->name = NULL;

    obj->child_object = NULL;
    obj->child_parameter = NULL;
    obj->NumOfObject = 0;
    obj->NumOfParameter = 0;
    obj->writable = READONLY;

    obj->nextPlaceholder = 0;
    obj->limit = PresentObject;
    obj->ParameterKey = NULL;
    obj->function = NULL;
}

/**
 * 初始化Parameter对象，把成员变量全初始化
 */
void init_parameter_struct(Parameter *param)
{
    param->name = NULL;
    param->writable = READONLY;
    param->notification = Notification_Off;
    param->valueType = NULL;
    param->function = NULL;
}

/**
 * 设置参数的各个成员变量
 */
void set_parameter_struct(Parameter *param, char *name, unsigned char writable, unsigned char notification, char *valueType, void (*function)())
{
    param->name = strdup(name);
    param->writable = writable;
    param->notification = notification;
    param->valueType = valueType;
    param->function = function;
}

/**#################################
##                                ##
##    get、set等操作的入口函数      ##
##                                ##
#################################**/

/**
 * 获取所有属性
 */
void getAllParameters()
{
    char root[] = ROOT;
    printAllParameters(rootJSON, root);
}

/**
 * 获取某属性名对应的配置的值，
 * 参数 name 指的是某对应参数的完整路径
 */
void getParameter(char *path, char **str)
{
    int length = strlen(path);
    if (path[length - 1] == '.')
    {
        printErrorInfo(FAULT_9003);
        return;
    }
    PATH = GetSubstrings(path);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9003);
        return;
    }

    int faultCode = checkParameterPath();
    if (faultCode > 0)
    {
        printErrorInfo(faultCode);
        return;
    }

    cJSON *node = getParameterJSON();
    if (node)
    {
        *str = node->valuestring;
    }
}

/**
 * NextLevel 的值只能是 0 或 1。0 表示列出该对象参数及其所有子对象或参数。
 * 1 表示列出路径中包含的所有参数。 如果路径为空且NextLevel为1，则仅列出ROOT
 */
void getParameterName(char *path, char *NextLevel, ParameterInfoStruct ***parameterList)
{
    int nextLevel;
    if (isNumeric(NextLevel))
        nextLevel = atoi(NextLevel);
    else if (strcmp(NextLevel, "true") == 0)
        nextLevel = 1;
    else if (strcmp(NextLevel, "false") == 0)
        nextLevel = 0;
    else
    {
        printErrorInfo(FAULT_9003);
        return;
    }

    if (!path && nextLevel)
    {
        if (parameterList)
        {
            *parameterList = (ParameterInfoStruct **)malloc(2 * sizeof(ParameterInfoStruct *));

            (*parameterList)[0] = (ParameterInfoStruct *)malloc(sizeof(ParameterInfoStruct));
            (*parameterList)[0]->name = concatenateStrings(ROOT, ".");
            (*parameterList)[0]->writable = 0;
            (*parameterList)[1] = NULL; // 结束标志
        }
        else
            printf("{ \" Object \" : \" %s \"}, { \" writable \" : \" %s \"}\n", concatenateStrings(ROOT, "."), "0");
        return;
    }

    // 判断路径类型
    // int length = strlen(path), pathType = 0; // pathType: 0为部分路径即Object, 1为完整路径即Parameter
    // if (path[length - 1] == '.')
    //     pathType = 0;
    // else
    //     pathType = 1;

    PATH = GetSubstrings(path);
    if (!path)
    {
        PATH = GetSubstrings("Device.");
        path = "Device.";
    }

    if (nextLevel)
    { // 包含所有参数和作为ParameterPath参数给定对象的下一级子对象的对象（如果有的话）
        *parameterList = getChildFromJson(path);
    }
    else
    {
        /*
            包含名称与ParameterPath参数完全匹配的Parameter或对象，
            以及ParameterPath参数给定的对象的子代的所有Parameters和对象（如果有的话）
        */
        cJSON *node = rootJSON;
        int index = 1;
        while (index < count)
        {
            node = cJSON_GetObjectItem(node, PATH[index]);
            if (!node)
                break;
            index++;
        }
        if (node)
        {
            *parameterList = (ParameterInfoStruct **)malloc(1 * sizeof(ParameterInfoStruct *));
            int index = 0;
            getDescendantsFromJson(path, node, parameterList, &index);
        }
    }
}

/**
 * 修改某属性名对应的配置的值
 * （注：前提这个属性允许被修改）
 */
void setParameter(char *path, char *value)
{
    int length = strlen(path);
    if (path[length - 1] == '.')
    {
        printErrorInfo(FAULT_9003);
        return;
    }
    PATH = GetSubstrings(path);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9003);
        return;
    }

    int faultCode = checkParameterPath();
    if (faultCode > 0)
    {
        printErrorInfo(faultCode);
        return;
    }
    else if (faultCode < 0)
    {
        printErrorInfo(FAULT_9008);
        return;
    }

    cJSON *node = getParameterJSON();
    if (node)
    {
        cJSON_SetValuestring(node, value);
        save_data();
        printf("Successfully modified");
    }
}

/**#################################
##                                ##
##      DataModel相关的函数        ##
##                                ##
#################################**/

/**
 * 添加Object实例
 * 需要检查该Object是否为可创建，即数据模型中该路径后为{i}占位符, 且支持PresentObject、CreateObject或AddObject权限
 */
void addObject(char *path)
{
    int length = strlen(path);
    if (path[length - 1] != '.')
    {
        printErrorInfo(FAULT_9003);
        return;
    }
    PATH = GetSubstrings(path);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9003);
        return;
    }

    addObjectToData();

    FreePATH();
}

/**
 * 为data model树添加一条新obj路径
 * 返回该新路径的最后一个Object对象
 */
int addObjectToDataModel(char *path, unsigned char writable,unsigned char limit, void (*function)())
{
    int length = strlen(path);
    if (path[length - 1] != '.')
    {
        printErrorInfo(FAULT_9003);
        return FAULT_9003;
    }
    PATH = GetSubstrings(path);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9003);
        return FAULT_9003;
    }

    struct Object *obj = createObjectToDataModel(dataModel, 1); // 添加Object路径，存在不会创新路径
    obj->writable = writable;
    obj->limit = limit;
    obj->function = function;
    if (path[length - 2] == '}')
    {
        obj->nextPlaceholder = 1; // 表示该对象下个实例可用的占位符序号
    }

    createObjectPathToJsonData(); // 并在数据文件中创建该路径

    FreePATH();
    free(path);
    return FAULT_0;
}

/**
 * 递归创建object子树, 返回最底层的Object节点
 */
struct Object *createObjectToDataModel(struct Object *obj, const int index)
{
    if (index >= count)
        return obj;
    struct Object *tmp = findChildObject(obj, PATH[index]);
    if (isNumeric(PATH[index]))
        tmp = findChildObject(obj, "{i}");

    if (!tmp)
    {
        obj->NumOfObject++;
        obj->child_object = (struct Object *)realloc(obj->child_object, obj->NumOfObject * sizeof(struct Object));
        tmp = &(obj->child_object[obj->NumOfObject - 1]);
        init_object_struct(tmp);
        tmp->name = strdup(PATH[index]);
    }

    return createObjectToDataModel(tmp, index + 1);
}

/**
 * 在Object节点中查找路径名称等于str的节点并返回该子节点，不存在则返回NULL
 */
struct Object *findChildObject(struct Object *obj, const char *str)
{
    struct Object *tmp = NULL;
    for (size_t i = 0; i < obj->NumOfObject; i++)
    {
        if (strcmp(obj->child_object[i].name, str) == 0)
        {
            // printf("现在儿子有： %s\n", obj->child_object[i].name);
            tmp = &(obj->child_object[i]);
        }
    }
    return tmp;
}

/**
 * 添加parameter到dataModel中，即dataModel树的叶子
 * path以'.'分隔的最后一个元素为parameter的name
 */
int addParameterToDataModel(char *path, unsigned char writable, unsigned char notification, char *valueType, void (*function)())
{
    int length = strlen(path);
    if (path[length - 1] == '.')
    {
        printErrorInfo(FAULT_9003);
        return FAULT_9003;
    }
    PATH = GetSubstrings(path);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9003);
        return FAULT_9003;
    }

    count--;                                                    // 去掉parameter
    struct Object *obj = createObjectToDataModel(dataModel, 1); // 先添加Object路径，存在不会创新路径

    obj->NumOfParameter++;
    obj->child_parameter = (Parameter *)realloc(obj->child_parameter, obj->NumOfParameter * sizeof(Parameter));
    set_parameter_struct(&(obj->child_parameter[obj->NumOfParameter - 1]), PATH[count], writable, notification, valueType, function);

    createParameterPathToJsonData(); // 把参数Path添加到json文件中

    count++; // 恢复，为了正常释放PATH内存

    FreePATH();
    free(path);
    return FAULT_0;
}

/**
 * 释放全局变量PATH的空间
 */
void FreePATH()
{
    if (PATH)
    {
        for (size_t i = 0; i < count; ++i)
        {
            free(PATH[i]); // 释放每个子字符串
        }
        free(PATH); // 释放存储指针的数组
        PATH = NULL;
    }
}

/**
 * 输出数据模型树中所有的路径
 */
void iterateDataModel(struct Object *obj, char *str)
{
    if (obj == NULL)
    {
        iterateDataModel(dataModel, NULL);
        return;
    }
    char *destination = NULL;
    size_t totalLength = 0;

    if (str)
        totalLength += strlen(str);
    if (obj->name)
        totalLength += strlen(obj->name);
    totalLength += 2;

    destination = (char *)malloc(totalLength);
    if (destination == NULL)
    {
        perror("Memory allocation failed");
        return;
    }
    destination[0] = '\0';

    if (str)
        strcat(destination, str);
    if (obj->name)
        strcat(destination, obj->name);
    strcat(destination, ".");
    printf("%s\n", destination);

    // 输出参数节点
    if (obj->NumOfParameter > 0)
    {
        for (size_t i = 0; i < obj->NumOfParameter; i++)
        {
            printf("%s%s\n", destination, obj->child_parameter[i].name);
        }
    }

    for (size_t i = 0; i < obj->NumOfObject; i++)
    {
        iterateDataModel(&(obj->child_object[i]), destination);
    }
    free(destination);
}

/**
 * 检测Object路径是否符合数据模型
 */
int checkObjectPath()
{
    struct Object *obj = dataModel;
    int index = 1;
    while (obj && index < count)
    {
        if (isNumeric(PATH[index]))
            obj = findChildObject(obj, "{i}");
        else
            obj = findChildObject(obj, PATH[index]);
        index++;
    }

    if (index < count)
    {
        printErrorInfo(FAULT_9003);
        return FAULT_9003;
    }
    return FAULT_0;
}

/**
 * 检测Object是否可以实例化, 可以则返回其占位符{i}
 */
struct Object *ObjectCanInstantiate()
{
    struct Object *obj = createObjectToDataModel(dataModel, 1); // 获取最后一个Object
    for (size_t i = 0; i < obj->NumOfObject; i++)
    {
        if (strcmp(obj->child_object[i].name, "{i}") == 0)
        {
            return &(obj->child_object[i]);
        }
    }
    return NULL;
}

/**
 * 检查输入的参数路径是否符合已存在的数据模型(即路径是否在数据模型中存在)
 */
int checkParameterPath()
{
    count--;
    int faultCode = checkObjectPath();
    if (faultCode > 0)
        return faultCode;

    struct Object *obj = dataModel;
    obj = createObjectToDataModel(obj, 1);
    count++;
    for (size_t i = 0; i < obj->NumOfParameter; i++)
    {
        if (strcmp(obj->child_parameter[i].name, PATH[count - 1]) == 0)
        {
            if (obj->child_parameter[i].writable == WRITABLE)
                return FAULT_0;
            else
                return -1; // 不可set修改值
        }
    }
    return FAULT_9003;
}

/**
 * 真正添加Object实例的函数
 */
int addObjectToData()
{
    int faultCode = checkObjectPath();
    if (faultCode > 0)
        return faultCode;

    struct Object *obj = ObjectCanInstantiate();

    createObjectToJsonData(obj);
}

/**#################################
##                                ##
##     操作JSON文件相关的函数       ##
##                                ##
#################################**/

/**
 * 初始化数据,从文件中读取数据，若无则创建一个空的cJSON实例
 */
bool init_root()
{
    FILE *file = fopen(DATAFILE, "r");
    if (file == NULL)
    {
        printf("fail to open file!");
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize > 0)
    {
        char *jsonString = (char *)malloc(fileSize + 1);
        fread(jsonString, 1, fileSize, file);
        jsonString[fileSize] = '\0';

        rootJSON = cJSON_Parse(jsonString);
        free(jsonString);
    }
    else
    {
        rootJSON = cJSON_CreateObject();
    }

    fclose(file); // 关闭文件

    return true;
}

/**
 * 将数据保存到文件中
 */
bool save_data()
{
    FILE *file = fopen(DATAFILE, "w");
    if (file == NULL)
    {
        printf("fail to open file!");
        return false;
    }

    char *jsonString = cJSON_Print(rootJSON); // 转成字符串
    fprintf(file, "%s", jsonString);          // 写入
    fclose(file);                             // 关闭
    // free(jsonString);//释放字符串空间

    return true;
}

/**
 * 把Path中的Object路径添加到Json文件中
 */
cJSON *createObjectPathToJsonData()
{
    int index = 1;
    cJSON *node = rootJSON;
    cJSON *targetNode = node;
    while (index < count)
    {
        targetNode = cJSON_GetObjectItem(targetNode, PATH[index]);
        if (targetNode)
            node = targetNode;
        else
            break;
        index++;
    }

    while (index < count)
    {
        if (PATH[index][strlen(PATH[index]) - 1] == '}' || isNumeric(PATH[index]))
            break;
        targetNode = cJSON_CreateObject();
        cJSON_AddItemToObject(node, PATH[index], targetNode);
        node = targetNode;
        index++;
    }
    save_data();
    if (index < count)
        return NULL;
    return node;
}

/**
 * 把参数Path路径添加到Json文件中，如果其中已存在则不会创建
 */
void createParameterPathToJsonData()
{
    cJSON *node = createObjectPathToJsonData();
    if (!node)
        return;
    if (!cJSON_GetObjectItemCaseSensitive(node, PATH[count]))
    {
        cJSON_AddStringToObject(node, PATH[count], "");
        save_data();
    }
}

/**
 * 向Json文件中创建新的Object实例
 * placeholder: dataModel中PATH对应的占位符object对象
 */
void createObjectToJsonData(struct Object *placeholder)
{
    cJSON *node = createObjectPathToJsonData(); // 获取要添加的Object的Json对象
    cJSON *target = cJSON_CreateObject();       // 创建空JSON对象
    if (!node)
    {
        printErrorInfo(FAULT_9003);
        return;
    }

    // 在JSON文件中获取最大实例号并加1
    placeholder->nextPlaceholder = GetPlaceholderMaxNum(node) + 1;

    // 将占位符数字转为字符串
    int len = 0, num = placeholder->nextPlaceholder;
    while (num)
    {
        len++;
        num /= 10;
    }
    char *str = (char *)malloc(len + 1);
    sprintf(str, "%d", placeholder->nextPlaceholder);

    // 将实例插入到JSON文件中
    cJSON_AddItemToObject(node, str, target);
    placeholder->nextPlaceholder++;

    // 给新创建的实例补充属性
    ObjectInstanceAttributeSupplementation(target, placeholder);
    save_data();
}

/**
 * 给新创建的对象实例补充其应该有的属性(递归)
 */
void ObjectInstanceAttributeSupplementation(cJSON *node, struct Object *obj)
{
    cJSON *target;
    for (size_t i = 0; i < obj->NumOfObject; i++)
    {
        if (strcmp(obj->child_object[i].name, "{i}") == 0)
            continue;
        target = cJSON_CreateObject();
        ObjectInstanceAttributeSupplementation(target, &(obj->child_object[i]));
        cJSON_AddItemToObject(node, obj->child_object[i].name, target);
    }

    for (size_t i = 0; i < obj->NumOfParameter; i++)
    {
        cJSON_AddStringToObject(node, obj->child_parameter[i].name, "");
    }
}

/**
 * 从JSON文件中取出某占位符位置的最大实例号
 */
int GetPlaceholderMaxNum(cJSON *node)
{
    cJSON *child = node->child;
    int maxNum = 0;
    // cJSON *maxNumObject = NULL;

    while (child != NULL)
    {
        const char *name = child->string;
        if (name != NULL && isdigit(name[0]))
        {
            int num = atoi(name);

            if (num > maxNum)
            {
                maxNum = num;
                // maxNumObject = child;
            }
        }

        child = child->next;
    }

    return maxNum;
}

/**
 * 获取Parameter在JSON文件中对应的cJSON对象
 */
cJSON *getParameterJSON()
{
    int index = 1;
    cJSON *node = rootJSON;
    while (index < count)
    {
        node = cJSON_GetObjectItemCaseSensitive(node, PATH[index]);
        if (!node)
            break;
        index++;
    }

    if (index < count)
    {
        printErrorInfo(FAULT_9003);
    }
    return node;
}

/**
 * 从Json中获取路径的子属性（包括对象类型）
 */
ParameterInfoStruct **getChildFromJson(char *path)
{
    cJSON *node = rootJSON;
    int index = 1;
    while (index < count)
    {
        node = cJSON_GetObjectItem(node, PATH[index]);
        if (!node)
            break;
        index++;
    }

    ParameterInfoStruct **List;

    if (node)
    {
        List = (ParameterInfoStruct **)malloc(sizeof(ParameterInfoStruct *));
        node = node->child;
        index = 1;
        while (node != NULL)
        {
            size_t pathLen = strlen(path);
            size_t nodeLen = strlen(node->string);
            char *str = (char *)malloc(pathLen + nodeLen + 2);
            strcpy(str, path);
            strcat(str, node->string);
            if (node->type == cJSON_Object)
                strcat(str, ".");

            ParameterInfoStruct *info = (ParameterInfoStruct *)malloc(sizeof(ParameterInfoStruct));
            info->name = str;
            info->writable = false;

            List = (ParameterInfoStruct **)realloc(List, (index + 1) * sizeof(ParameterInfoStruct *));
            List[index - 1] = info;
            List[index] = NULL; // 添加结束标志

            node = node->next;
            index++;
        }

        return List;
    }
    else
    {
        return NULL;
    }
}

/**
 * 从Json中获取路径的所有子代属性
 */
void getDescendantsFromJson(const char *path, cJSON *object, ParameterInfoStruct ***List, int *const index)
{
    if (object == NULL)
    {
        getDescendantsFromJson(path, rootJSON, List, index);
    }

    char *name;
    int pathLength = strlen(path);
    if (object->type == cJSON_Object)
    {
        // printf("Object: %s\n", object->string);
        int Length = pathLength + 2;
        if (object->string)
            Length += strlen(object->string);
        name = (char *)malloc(Length);
        strcpy(name, path);
        if (object->string)
        {
            strcat(name, object->string);
            strcat(name, ".");
        }
        ParameterInfoStruct *objectInfo = (ParameterInfoStruct *)malloc(sizeof(ParameterInfoStruct));
        objectInfo->name = name;
        objectInfo->writable = false;
        (*index)++;
        *List = (ParameterInfoStruct **)realloc(*List, (*index) * sizeof(ParameterInfoStruct *));
        (*List)[*index - 1] = objectInfo;
    }
    else
    {
        // printf("Key: %s, Value: %s\n", object->string, cJSON_Print(object));
        int Length = pathLength + strlen(object->string) + 1;
        name = (char *)malloc(Length);
        strcpy(name, path);
        strcat(name, object->string);
        ParameterInfoStruct *parameterInfo = (ParameterInfoStruct *)malloc(sizeof(ParameterInfoStruct));
        parameterInfo->name = name;
        parameterInfo->writable = false;
        (*index)++;
        *List = (ParameterInfoStruct **)realloc(*List, (*index) * sizeof(ParameterInfoStruct *));
        (*List)[*index - 1] = parameterInfo;
    }

    cJSON *child = object->child;
    while (child != NULL)
    {
        getDescendantsFromJson(name, child, List, index);
        child = child->next;
    }
}

/**
 * 打印出所有的参数和值
 */
void printAllParameters(cJSON *jsonObj, char *str)
{
    char *destination = NULL;
    size_t Length = 0;

    if (str)
        Length += strlen(str);
    if (jsonObj->string)
        Length += strlen(jsonObj->string);
    Length += 2;

    destination = (char *)malloc(Length);
    if (destination == NULL)
    {
        perror("Memory allocation failed");
        return;
    }
    destination[0] = '\0';

    if (str)
        strcat(destination, str);
    if (jsonObj->string)
        strcat(destination, jsonObj->string);
    strcat(destination, ".");

    cJSON *child = jsonObj->child;
    while (child != NULL)
    {
        if (child->type == cJSON_Object)
            printAllParameters(child, destination);
        else
        {
            printf("{ \" parameter \" : \" %s%s \"}, { \" value \" : \" %s \"}\n", destination, child->string, child->valuestring);
        }
        child = child->next;
    }
}

/**#################################
##                                ##
##    类型转换或判断等相关的函数     ##
##                                ##
#################################**/

/**
 * 从字符串中提取出关键字，如“local.url.port”，提取出local，url，port,并且按原顺序排列
 * 返回二位字符数组
 */
char **GetSubstrings(const char *input)
{
    if (!input)
        return NULL;
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
 * 两个字符串拼接，返回新的字符串
 */
char *concatenateStrings(const char *str1, const char *str2)
{
    // 计算两个字符串的长度
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);

    // 分配足够的内存(+1是为了'\0')
    char *result = (char *)malloc(len1 + len2 + 1);

    // 检查内存分配是否成功
    if (result == NULL)
    {
        printErrorInfo(FAULT_9004);
        return NULL;
    }

    // 拷贝两个字符串
    strcpy(result, str1);
    strcat(result, str2);

    return result;
}

/**
 * 判断字符串是否为数字
 */
bool isNumeric(const char *str)
{
    int length = strlen(str);

    // 检查每个字符是否是数字
    for (int i = 0; i < length; i++)
    {
        if (!isdigit(str[i]))
        {
            return false; // 如果有非数字字符，则返回0
        }
    }

    return true; // 字符串中的所有字符都是数字
}