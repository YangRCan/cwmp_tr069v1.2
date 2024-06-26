/**
 * @Copyright : Yangrongcan
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif
#if defined(__ANDROID__)
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#endif // __ANDROID__

#include "parameter.h"
#include "faultCode.h"
#include "tr181.h"
#include "tr135.h"

static struct Object *dataModel;
static cJSON *rootJSON;

static char **PATH;                 // 完整路径
static int count;                   // 路径节点数
static const char *delimiter = "."; // 路径分隔符

char *download_dir;

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
    init_tr181_object();//通用数据模型要先创建
    init_tr181_parameter();
    init_tr135_object();
    init_tr135_parameter();
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

    // 默认支持LAN侧修改
    char *Access = (char *)malloc(strlen("Subscriber") + 1);
    strcpy(Access, "Subscriber");
    param->AccessList = (char **)malloc(sizeof(char **));
    *(param->AccessList) = Access;

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
 * 修改某属性名对应的配置的值
 * （注：前提这个属性允许被修改）
 */
ExecuteResult setParameter(const char *path, const char *value, int verify)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    int length = strlen(path);
    if (path[length - 1] == '.')
    {
        printErrorInfo(FAULT_9003);
        result.fault_code = FAULT_9003;
        return result;
    }
    PATH = getSubStrings(path, &count);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9003);
        freePath(PATH, count);
        result.fault_code = FAULT_9003;
        return result;
    }

    int faultCode = checkParameterPath();
    if (faultCode > 0)
    {
        freePath(PATH, count);
        return result;
    }
    else if (faultCode < 0 && verify == MUSTVERIFY)
    {
        printErrorInfo(FAULT_9008);
        freePath(PATH, count);
        result.fault_code = FAULT_9008;
        return result;
    }

    cJSON *node = getParameterJSON();
    if (node)
    {
        cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(node, "value"), value);
        result.status = 1;
    }
    else
    {
        result.fault_code = FAULT_9003;
    }
    freePath(PATH, count);
    return result;
}

/**
 * 获取某属性名对应的配置的值，
 * 参数 path 指的是某对应参数的完整路径
 */
ExecuteResult getParameter(const char *path, param_info ***param_list)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    int length = strlen(path);
    PATH = getSubStrings(path, &count);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9005);
        freePath(PATH, count);
        result.fault_code = FAULT_9005;
        return result;
    }

    if (path[length - 1] == '.')
        result.fault_code = checkObjectPath() ? FAULT_0 : FAULT_9005;
    else
        result.fault_code = checkParameterPath();
    if (result.fault_code > 0)
    {
        freePath(PATH, count);
        return result;
    }

    cJSON *node = getParameterJSON(); // 获取路径的最后一个JSON节点
    if (node)
    {
        *param_list = getParameterlistByJsonNode(node, path);
        result.status = 1;
    }
    else
    {
        printErrorInfo(FAULT_9005);
        result.fault_code = FAULT_9005;
    }
    freePath(PATH, count);
    return result;
}

/**
 * NextLevel 的值只能是 0 或 1。0 表示列出该对象参数及其所有子对象或参数。
 * 1 表示列出路径中包含的所有参数。 如果路径为空且NextLevel为1，则仅列出ROOT
 */
ExecuteResult getParameterNames(const char *path, const char *NextLevel, ParameterInfoStruct ***parameterList)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
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
        result.fault_code = FAULT_9003;
        return result;
    }

    if (!path && nextLevel)
    {
        if (parameterList)
        {
            *parameterList = (ParameterInfoStruct **)realloc(*parameterList, 2 * sizeof(ParameterInfoStruct *));

            (*parameterList)[0] = (ParameterInfoStruct *)malloc(sizeof(ParameterInfoStruct));
            (*parameterList)[0]->name = concatenateStrings(ROOT, ".");
            (*parameterList)[0]->writable = 0;
            (*parameterList)[1] = NULL; // 结束标志
        }
        else
            printf("{ \" Object \" : \" %s \"}, { \" writable \" : \" %s \"}\n", concatenateStrings(ROOT, "."), "0");
        result.status = 1;
        return result;
    }

    // 判断路径类型
    // int length = strlen(path), pathType = 0; // pathType: 0为部分路径即Object, 1为完整路径即Parameter
    // if (path[length - 1] == '.')
    //     pathType = 0;
    // else
    //     pathType = 1;

    PATH = getSubStrings(path, &count);
    if (!path)
    {
        PATH = getSubStrings("Device.", &count);
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
            // *parameterList = (ParameterInfoStruct **)malloc(1 * sizeof(ParameterInfoStruct *));
            int index = 0;
            getDescendantsFromJson(path, node, parameterList, &index);
            // 添加结束符号NULL
            ParameterInfoStruct *objectInfo = NULL;
            *parameterList = (ParameterInfoStruct **)realloc(*parameterList, (index + 1) * sizeof(ParameterInfoStruct *));
            (*parameterList)[index] = objectInfo;
        }
        else
        {
            printErrorInfo(FAULT_9005);
            result.fault_code = FAULT_9005;
            return result;
        }
    }
    freePath(PATH, count);
    result.status = 1;
    return result;
}

/**
 * 修改与一个或多个CPE参数相关联的属性
 */
ExecuteResult setParameterAttributes(ParameterAttributeStruct *parameterAttribute, const bool NotificationChange, const bool AccessListChange, const int numOfAccess)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    int length = strlen(parameterAttribute->Name);
    char **pathList = getSubStrings(parameterAttribute->Name, &count);
    if (strcmp(dataModel->name, pathList[0]) != 0)
    {
        printErrorInfo(FAULT_9005);
        freePath(PATH, count);
        result.fault_code = FAULT_9005;
        return result;
    }

    cJSON *node = rootJSON;
    int index = 1;
    while (index < count)
    {
        node = cJSON_GetObjectItem(node, pathList[index]);
        if (!node)
            break;
        index++;
    }
    freePath(pathList, count);

    if (node)
    {
        if (cJSON_GetObjectItemCaseSensitive(node, "parameterType"))
        {
            if (NotificationChange)
            {
                cJSON *notification = cJSON_GetObjectItemCaseSensitive(node, "Notification");
                char str[2];
                sprintf(str, "%d", parameterAttribute->Notification);
                notification->valuestring = strdup(str);
            }

            if (AccessListChange)
            {
                cJSON_DeleteItemFromObject(node, "AccessList");
                cJSON *accessList = cJSON_CreateStringArray((const char *const *)parameterAttribute->AccessList, numOfAccess);
                cJSON_AddItemToObject(node, "AccessList", accessList);
            }

            // save_data();
        }
        else
        {
            node = node->child;
            while (node)
            {
                ParameterAttributeStruct param;
                param.AccessList = parameterAttribute->AccessList;
                param.Notification = parameterAttribute->Notification;
                param.Name = malloc(strlen(parameterAttribute->Name) + strlen(node->string) + 2);
                strcpy(param.Name, parameterAttribute->Name);
                strcat(param.Name, node->string);
                if (!cJSON_GetObjectItemCaseSensitive(node, "parameterType"))
                    strcat(param.Name, ".");
                ExecuteResult rlt = setParameterAttributes(&param, NotificationChange, AccessListChange, numOfAccess);
                free(param.Name);
                if (rlt.fault_code)
                    return rlt;
                node = node->next;
            }
        }
        result.status = 1;
        return result;
    }
    else
    {
        printErrorInfo(FAULT_9005);
        result.fault_code = FAULT_9005;
        return result;
    }
}

/**
 * 读取与一个或多个CPE参数相关联的属性。
 * 设定NULL为ParameterAttributeStruct列表结尾
 */
ParameterAttributeStruct **getParameterAttributes(const char *const *ParameterNames, const int numOfParameter, ExecuteResult *exe_rlt)
{
    exe_rlt->fault_code = FAULT_0;
    exe_rlt->status = 0;
    ParameterAttributeStruct **result = (ParameterAttributeStruct **)malloc(sizeof(ParameterAttributeStruct *));
    result[0] = NULL;
    int resultLength = 0;
    for (size_t i = 0; i < numOfParameter; i++)
    {
        ParameterAttributeStruct **row;
        if (ParameterNames && ParameterNames[i] && strcmp(ParameterNames[i], "") != 0)
        {
            int length = strlen(ParameterNames[i]);
            char **pathList = getSubStrings(ParameterNames[i], &count);
            if (strcmp(dataModel->name, pathList[0]) != 0)
            {
                printErrorInfo(FAULT_9005);
                freePath(pathList, count);
                exe_rlt->fault_code = FAULT_9005;
                return NULL;
            }
            freePath(pathList, count);
            row = getAttributesFromJson(ParameterNames[i]);
        }
        else
        {
            row = getAttributesFromJson("Device.");
        }
        int len = 0;
        while (row && row[len])
            len++;
        result = (ParameterAttributeStruct **)realloc(result, (resultLength + len + 1) * sizeof(ParameterAttributeStruct *));

        for (size_t i = 0; i < len; i++)
        {
            result[resultLength + i] = row[i];
        }
        result[resultLength + len] = NULL;
        free(row);
        resultLength += len;
    }
    exe_rlt->status = 1;
    return result;
}

/**
 * 添加Object实例
 * 需要检查该Object是否为可创建，即数据模型中该路径后为{i}占位符, 且支持PresentObject、CreateObject或AddObject权限
 */
ExecuteResult addObject(const char *path, char **instanceNumber)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    int length = strlen(path);
    if (path[length - 1] != '.')
    {
        printErrorInfo(FAULT_9005);
        result.fault_code = FAULT_9005;
        return result;
    }
    PATH = getSubStrings(path, &count);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9005);
        result.fault_code = FAULT_9005;
        return result;
    }
    if (addObjectToData(instanceNumber) > 0)
    {
        result.status = 0;
    }
    else
    {
        result.status = 1;
    }

    freePath(PATH, count);
    return result;
}

/**
 * 删除Object实例
 * 调用将对象实例的路径名（包括实例号）
 * 先检查该path是否为可删除，即该路径是最后是否为占位符的实例编号，且该实例在JSON中必须存在
 */
ExecuteResult deleteObject(const char *path)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    int length = strlen(path);
    if (path[length - 1] != '.')
    {
        printErrorInfo(FAULT_9005);
        result.fault_code = FAULT_9005;
        return result;
    }
    PATH = getSubStrings(path, &count);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9005);
        result.fault_code = FAULT_9005;
        return result;
    }

    struct Object *obj = checkObjectPath();    // 如果最后一个是数字，也会返回object对象{i}
    if (!obj && strcmp(obj->name, "{i}") != 0) // 路径不存在或者不是实例化对象
    {
        result.fault_code = FAULT_9005;
        return result;
    }
    count--;
    cJSON *node = getParameterJSON(); // 获取路径的最后一个JSON对象
    count++;
    cJSON *child = getParameterJSON();
    if (!node || !child)
    {
        result.fault_code = FAULT_9005;
        return result; // 路径不正确或者实例不存在
    }

    cJSON_DeleteItemFromObject(node, child->string); // 内部调用了cJSON_Delete将移除的对象释放
    // save_data();
    result.status = 1;
    return result;
}

/**
 * 文件下载函数
 * fileSize可为NULL
 */
ExecuteResult downloadFile(const char *url, const char *fileType, const char *fileSize, const char *username, const char *password)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    // 检查参数
    if (url == NULL || fileType == NULL)
    {
        printErrorInfo(FAULT_9003);
        result.fault_code = FAULT_9003;
        return result;
    }
    unsigned long long freeSpace = 0; // 剩余空间

#ifdef _WIN32
    // 默认下载到的路径
    download_dir = strdup("./tmp");
    // 路径不存在，创建
    if (CreateDirectory(download_dir, NULL) == 0)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            perror("Failed to create download directory");
            result.fault_code = FAULT_9010;
            return result;
        }
    }
    ULARGE_INTEGER freeBytesAvailable;
    if (GetDiskFreeSpaceEx(download_dir, &freeBytesAvailable, NULL, NULL) != 0)
    {
        freeSpace = freeBytesAvailable.QuadPart; // 这里需要根据系统调用获取空间大小 以字节为单位
    }
#elif defined(__ANDROID__)
    download_dir = strdup("/cache/update");
    if (access(download_dir, F_OK) != 0)
    {
        if (mkdir(download_dir, 0777) == -1)
        {
            perror("Failed to create download directory");
            result.fault_code = FAULT_9010;
            return result;
        }
    }
    struct statvfs stat;
    if (statvfs(download_dir, &stat) == 0)
    {
        freeSpace = (unsigned long long)stat.f_bsize * stat.f_bavail; // 以字节为单位
    }
#endif
    // fileSize存在，检测是否小于下载文件夹所剩空间，小于则下载，否则报错
    if (fileSize != NULL)
    {
        unsigned long long requiredSpace = atoll(fileSize);
        printf("require: %llu, free: %llu\n", requiredSpace, freeSpace);
        if (requiredSpace > freeSpace)
        {
            perror("Insufficient space to download the file\n");
            result.fault_code = FAULT_9010;
            return result;
        }
    }
    // 路径存在删除路径文件夹下的所有文件
    char command[100];
    snprintf(command, sizeof(command), "rm -rf %s/*", download_dir);
    if (system(command) == -1)
    {
        printf("Failed to remove existing files from download directory");
        result.fault_code = FAULT_9010;
        return result;
    }
    // 将用户名和密码拼接到url路径中
    char urlWithAuth[200];
    snprintf(urlWithAuth, sizeof(urlWithAuth), "%s://%s:%s@%s", fileType, username, password, url);
    // 进行下载
    printf("Downloading file from: %s\n", urlWithAuth);
    if (!download_file_to_dir(url, NULL, NULL, download_dir))
    {
        fprintf(stderr, "Download failed.\n");
        result.fault_code = FAULT_9010;
        return result;
    }
    // 下载成功
    result.status = 1;
    return result;
}

/**
 * 应用下载后的文件
 */
ExecuteResult applyDownloadFile(const char *fileType)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    printf("正在安装文件");
    if (strcmp(fileType, "1 Firmware Upgrade Image") == 0)
    {
#if defined(__ANDROID__)
        printf("Applying update.zip...\n");
        // 执行写入命令到/cache/recovery/command的操作
        char buffer[100];
        sprintf(buffer, "echo --update_package=%s/update.zip > /cache/recovery/command", download_dir);
        system(buffer);
        // 同步文件系统
        system("sync");
        // 重启到recovery模式
        system("reboot recovery");
#endif // __ANDROID__
    }
    result.status = 1;
    return result;
}

/**
 * 文件上传函数
 */
ExecuteResult uploadFile(const char *url, const char *fileType, const char *username, const char *password)
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    // 检查参数
    if (url == NULL || fileType == NULL)
    {
        printErrorInfo(FAULT_9003);
        result.fault_code = FAULT_9003;
        return result;
    }
    printf("Uploading, Upload complete!\n");
#if defined(__ANDROID__)

    FILE *fp;
    char *serial = (char *)malloc(32 * sizeof(char));
    // getprop | grep ro.serialno
    fp = popen("getprop ro.serialno", "r");
    if (fp == NULL)
    {
        printf("Error: Failed to execute getprop command\n");
        result.fault_code = FAULT_9002;
        return result;
    }
    if (fgets(serial, 32, fp) != NULL)
    {
        // 移除末尾的换行符
        serial[strcspn(serial, "\n")] = '\0';
    }
    else
    {
        printf("Error: Failed to read serial number\n");
        result.fault_code = FAULT_9002;
        return result;
    }
    pclose(fp);
    if (strlen(serial) == 0)
    {                                   // 如果序列号不存在
        strcpy(serial, "XUNION123456"); // 默认序列号
    }
    printf("Serial number: %s\n", serial);
    char up_url[255];
    // 处理URL
    if (username && username[0] != '\0' || password && password[0] != '\0')
    {
        snprintf(up_url, 48, "%s://%s:%s@", url, username, password);
        printf("%s\n", up_url);
    }
    // 上传日日志
    if (strcmp(fileType, "Vendor Log File") == 0)
    {
        char save_dir[] = "/cache/cwmp";
        if (access(save_dir, F_OK) == -1)
        {
            // 目录不存在，创建它
            if (mkdir(save_dir, 0777) == -1)
            {
                printf("Error creating directory %s\n", save_dir);
                result.fault_code = FAULT_9002;
                return result;
            }
        }
        char file_path[256];
        sprintf(file_path, "%s/log(%s).txt", save_dir, serial);
        // 执行 logcat 命令并获取输出
        FILE *fp = popen("logcat -d", "r");
        if (fp == NULL)
        {
            printf("Error executing logcat command\n");
            result.fault_code = FAULT_9002;
            return result;
        }
        // 创建文件并写入logcat输出
        FILE *outputFile = fopen(file_path, "w");
        if (outputFile == NULL)
        {
            printf("Error creating output file\n");
            pclose(fp);
            result.fault_code = FAULT_9002;
            return result;
        }
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            fputs(buffer, outputFile);
        }
        // 关闭文件和流
        fclose(outputFile);
        pclose(fp);
        printf("Logcat saved to: %s\n", file_path);

        // 将文件上传
        CURL *curl;
        CURLcode res;
        // 初始化
        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, file_path);
            if (username && username[0] != '\0' && password && password[0] != '\0')
            {
                char un_pw[100];
                snprintf(un_pw, 100, "%s:%s", username, password);
                curl_easy_setopt(curl, CURLOPT_USERPWD, un_pw);
            }
            res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                result.fault_code = FAULT_9011;
            }
            curl_easy_cleanup(curl);
        }
    }
    else if (strcmp(fileType, "Vendor Configuration File"))
    {
    }
    free(serial);

#endif                 // __ANDROID__
    result.status = 1; // 成功
    return result;
}

/**
 * 获取需要通知的参数
 */
param_info **getInformParameter()
{
    param_info **info_param = NULL;
    info_param = getInfoParamFromJson("Device.");
    return info_param;
}

/**
 * 执行恢复出厂设置命令
 */
ExecuteResult do_factory_reset()
{
    ExecuteResult rlt;
    rlt.fault_code = FAULT_0;
    rlt.status = 1;
    printf("factory_reset!");
#if defined(__ANDROID__)
    system("echo --wipe_data >> /cache/recovery/command");
    system("echo --wipe_cache >> /cache/recovery/command");
    // 同步文件系统
    system("sync");
    // 重启到recovery模式
    system("reboot recovery");
#endif // __ANDROID__
    return rlt;
}

/**
 * 执行重启命令
 */
ExecuteResult do_reboot()
{
    ExecuteResult rlt;
    rlt.fault_code = FAULT_0;
    rlt.status = 1;
    printf("重启！");
#if defined(__ANDROID__)
    sync();
    reboot(RB_AUTOBOOT);
#endif // __ANDROID__
    return rlt;
}

/**#################################
##                                ##
##      DataModel相关的函数        ##
##                                ##
#################################**/

/**
 * 为data model树添加一条新obj路径
 * 返回该新路径的最后一个Object对象
 */
int addObjectToDataModel(char *path, const unsigned char writable, const unsigned char limit, void (*function)())
{
    int length = strlen(path);
    if (path[length - 1] != '.')
    {
        printErrorInfo(FAULT_9003);
        return FAULT_9003;
    }
    PATH = getSubStrings(path, &count);
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

    freePath(PATH, count);
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
int addParameterToDataModel(char *path, char *value, unsigned char writable, unsigned char notification, char *valueType, void (*function)())
{
    int length = strlen(path);
    if (path[length - 1] == '.')
    {
        printErrorInfo(FAULT_9003);
        return FAULT_9003;
    }
    PATH = getSubStrings(path, &count);
    if (strcmp(dataModel->name, PATH[0]) != 0)
    {
        printErrorInfo(FAULT_9003);
        return FAULT_9003;
    }

    count--;                                                    // 去掉parameter
    struct Object *obj = createObjectToDataModel(dataModel, 1); // 先添加Object路径，存在不会创新路径

    obj->NumOfParameter++;
    obj->child_parameter = (Parameter *)realloc(obj->child_parameter, obj->NumOfParameter * sizeof(Parameter));
    init_parameter_struct(&(obj->child_parameter[obj->NumOfParameter - 1])); // 初始化参数
    set_parameter_struct(&(obj->child_parameter[obj->NumOfParameter - 1]), PATH[count], writable, notification, valueType, function);

    createParameterPathToJsonData(&(obj->child_parameter[obj->NumOfParameter - 1]), value); // 把参数Path添加到json文件中

    count++; // 恢复，为了正常释放PATH内存

    freePath(PATH, count);
    free(path);
    return FAULT_0;
}

/**
 * 释放全局变量PATH的空间
 */
void freePath(char **path, const int count)
{
    if (path)
    {
        for (size_t i = 0; i < count; ++i)
        {
            free(path[i]); // 释放每个子字符串
        }
        free(path); // 释放存储指针的数组
        path = NULL;
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
 * 检测Object路径是否符合数据模型, 并且返回该路径的最后一个Object对象
 */
struct Object *checkObjectPath()
{
    struct Object *obj = dataModel;
    int index = 1;
    while (index < count)
    {
        if (isNumeric(PATH[index]))
            obj = findChildObject(obj, "{i}");
        else
            obj = findChildObject(obj, PATH[index]);
        if (!obj)
            break;
        index++;
    }

    if (index < count)
    {
        printErrorInfo(FAULT_9005);
        return NULL;
    }
    return obj;
}

/**
 * 检测Object是否可以实例化, 可以则返回其占位符{i}
 * 即按路径在数据模型中进行检查
 */
struct Object *ObjectCanInstantiate()
{
    struct Object *obj = checkObjectPath(); // 检查路径是否正确并且获取该路径的最后一个Object对象
    if (!obj)
    {
        return NULL;
    }
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
    struct Object *obj = checkObjectPath(); // 正确则会返回路径的最后一个Object对象
    if (!obj)
    {
        return FAULT_9005;
    }

    // obj = dataModel;
    // obj = createObjectToDataModel *(obj, 1);
    count++;
    for (size_t i = 0; i < obj->NumOfParameter; i++)
    {
        if (strcmp(obj->child_parameter[i].name, PATH[count - 1]) == 0)
        {
            if (obj->child_parameter[i].writable == WRITABLE)
            {
                return FAULT_0;
            }
            else
            {
                return -1; // 不可set修改值
            }
        }
    }
    printErrorInfo(FAULT_9005);
    return FAULT_9005;
}

/**
 * 真正添加Object实例的函数
 */
int addObjectToData(char **instanceNumber)
{
    struct Object *obj = checkObjectPath();
    if (!obj)
    {
        return FAULT_9005;
    }

    obj = ObjectCanInstantiate(); // 可以实例化则会返回其下一级占位符{i}
    if (!obj)
    {
        return FAULT_9005;
    }
    // 在JSON中创建该实例
    *instanceNumber = createObjectToJsonData(obj);
    if (*instanceNumber == NULL)
        return FAULT_9003;
    return FAULT_0;
}

/**
 * 获取object或者parameter的writable属性返回
 */
unsigned char getWritable(const char *path)
{
    int pathLength = strlen(path);
    bool isObject = false;
    int count = 0, index = 1;
    if (path[pathLength - 1] == '.')
        isObject = true;

    char **pathList = getSubStrings(path, &count);

    if (!isObject)
        count--;
    struct Object *obj = dataModel;
    while (index < count)
    {
        if (isNumeric(pathList[index]))
            obj = findChildObject(obj, "{i}");
        else
            obj = findChildObject(obj, pathList[index]);
        index++;
    }

    if (index < count)
    {
        printErrorInfo(FAULT_9005);
        if (!isObject)
            count++;
        freePath(pathList, count);
        return -1;
    }

    if (isObject)
    {
        free(pathList);
        return obj->writable;
    }
    else
    {
        Parameter *param = NULL;
        for (size_t i = 0; i < obj->NumOfParameter; i++)
        {
            if (strcmp(obj->child_parameter[i].name, pathList[index]) == 0)
                param = &(obj->child_parameter[i]);
        }
        if (param)
            return param->writable;
        else
        {
            printErrorInfo(FAULT_9005);
            return -1;
        }
    }
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
        // 文件不存在，尝试重新创建该文件
        file = fopen(DATAFILE, "w");
        if (file == NULL)
        {
            printf("Failed to create file.\n");
            return false;
        }
        printf("File created successfully!\n");
        fclose(file);
        file = NULL;
    }
    if (file == NULL)
        file = fopen(DATAFILE, "r"); // 创建文件后应以读方式重新打开
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
ExecuteResult save_data()
{
    ExecuteResult result;
    result.fault_code = FAULT_0;
    result.status = 0;
    FILE *file = fopen(DATAFILE, "w");
    if (file == NULL)
    {
        printf("fail to open file!");
        result.fault_code = FAULT_9002;
        return result;
    }

    char *jsonString = cJSON_Print(rootJSON); // 转成字符串
    fprintf(file, "%s", jsonString);          // 写入
    fclose(file);                             // 关闭
    free(jsonString);                         // 释放字符串空间

    result.status = 1;
    return result;
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
    {
        return NULL;
    }
    return node;
}

/**
 * 把参数Path路径添加到Json文件中，如果其中已存在则不会创建
 */
void createParameterPathToJsonData(Parameter *param, char *value)
{
    cJSON *node = createObjectPathToJsonData();
    if (!node)
        return;
    if (!cJSON_GetObjectItemCaseSensitive(node, PATH[count]))
    {
        cJSON *target = cJSON_CreateObject();
        cJSON_AddItemToObject(node, PATH[count], target);
        // cJSON_AddStringToObject(node, PATH[count], "");
        setParameterAttributesToJsonData(target, param, value);
        save_data();
    }
}

/**
 * 为cJSON中的参数设置属性value、writable、AccessList
 */
void setParameterAttributesToJsonData(cJSON *node, Parameter *param, const char *value)
{
    // parameterType
    cJSON *target = cJSON_GetObjectItem(node, "parameterType");
    if (target)
    {
        free(target->valuestring);
        target->valuestring = strdup(param->valueType);
    }
    else
        cJSON_AddItemToObject(node, "parameterType", cJSON_CreateString(param->valueType));

    // value
    target = cJSON_GetObjectItem(node, "value");
    if (target)
    {
        free(target->valuestring);
        target->valuestring = strdup(value);
    }
    else
        cJSON_AddItemToObject(node, "value", cJSON_CreateString(value));

    // writable
    target = cJSON_GetObjectItem(node, "writable");
    char writable[2];
    sprintf(writable, "%d", param->writable);
    if (target)
    {
        free(target->valuestring);
        target->valuestring = strdup(writable);
    }
    else
        cJSON_AddItemToObject(node, "writable", cJSON_CreateString(writable));

    // Notification
    target = cJSON_GetObjectItem(node, "Notification");
    char notification[2];
    sprintf(notification, "%d", param->notification);
    if (target)
    {
        free(target->valuestring);
        target->valuestring = strdup(notification);
    }
    else
        cJSON_AddItemToObject(node, "Notification", cJSON_CreateString(notification));

    // AccessList
    target = cJSON_GetObjectItem(node, "AccessList");
    if (target)
    {
        cJSON_DeleteItemFromObject(node, "AccessList"); // 删除原有的AccessList属性
        // 创建新的AccessList属性并赋值
        cJSON *newAccessListNode = cJSON_CreateStringArray((const char *const *)param->AccessList, 1);
        cJSON_AddItemToObject(node, "AccessList", newAccessListNode);
    }
    else
    {
        // 创建新的AccessList属性并赋值
        cJSON *newAccessListNode = cJSON_CreateStringArray((const char *const *)param->AccessList, 1);
        cJSON_AddItemToObject(node, "AccessList", newAccessListNode);
    }
}

/**
 * 向Json文件中创建新的Object实例
 * placeholder: dataModel中PATH对应的占位符object对象
 */
char *createObjectToJsonData(struct Object *placeholder)
{
    if (!placeholder)
    {
        printErrorInfo(FAULT_9005);
        return NULL;
    }
    cJSON *node = createObjectPathToJsonData(); // 获取要添加的Object的Json对象
    if (!node)
    {
        printErrorInfo(FAULT_9003);
        return NULL;
    }
    cJSON *target = cJSON_CreateObject(); // 创建空JSON对象

    // 在JSON文件中获取最大实例号并加1
    placeholder->nextPlaceholder = getPlaceholderMaxNum(node) + 1 > placeholder->nextPlaceholder ? getPlaceholderMaxNum(node) + 1 : placeholder->nextPlaceholder;

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
    objectInstanceAttributeSupplementation(target, placeholder);

    // save_data();

    return str; // 返回实例号
}

/**
 * 给新创建的对象实例补充其应该有的属性(递归)
 * 参数: node是在JSON文件中新创建的实例节点；obj是数据模型中的对应的节点
 */
void objectInstanceAttributeSupplementation(cJSON *node, struct Object *obj)
{
    cJSON *target;
    for (size_t i = 0; i < obj->NumOfObject; i++)
    {
        if (strcmp(obj->child_object[i].name, "{i}") == 0)
            continue;
        target = cJSON_CreateObject();
        objectInstanceAttributeSupplementation(target, &(obj->child_object[i]));
        cJSON_AddItemToObject(node, obj->child_object[i].name, target);
    }

    for (size_t i = 0; i < obj->NumOfParameter; i++)
    {
        target = cJSON_CreateObject();
        cJSON_AddItemToObject(node, obj->child_parameter[i].name, target);
        setParameterAttributesToJsonData(target, &(obj->child_parameter[i]), "");
    }
}

/**
 * 从JSON文件中取出某占位符位置的最大实例号
 */
int getPlaceholderMaxNum(cJSON *node)
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
            }
        }

        child = child->next;
    }

    return maxNum;
}

/**
 * 获取 Parameter 路径 在JSON文件中对应的cJSON对象(最后一个cJOSN)
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
        printErrorInfo(FAULT_9005);
    }
    return node;
}

/**
 * 从Json中获取路径的子属性（包括对象类型）
 */
ParameterInfoStruct **getChildFromJson(const char *path)
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
            if (!cJSON_GetObjectItemCaseSensitive(node, "parameterType"))
                strcat(str, ".");

            ParameterInfoStruct *info = (ParameterInfoStruct *)malloc(sizeof(ParameterInfoStruct));
            info->name = str;
            info->writable = getWritable(str);

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
    if (!cJSON_GetObjectItemCaseSensitive(object, "parameterType"))
    {

        int Length = pathLength + 2;
        if (object->string)
            Length += strlen(object->string);
        name = (char *)malloc(Length);
        strcpy(name, path);
        if (object->string && !strstr(path, object->string))
        {
            strcat(name, object->string);
            strcat(name, ".");
        }
        ParameterInfoStruct *objectInfo = (ParameterInfoStruct *)malloc(sizeof(ParameterInfoStruct));
        objectInfo->name = name;
        objectInfo->writable = getWritable(name);
        (*index)++;
        *List = (ParameterInfoStruct **)realloc(*List, (*index) * sizeof(ParameterInfoStruct *));
        (*List)[*index - 1] = objectInfo;

        cJSON *child = object->child;
        while (child != NULL)
        {
            getDescendantsFromJson(name, child, List, index);
            child = child->next;
        }
    }
    else
    {
        // printf("Key: %s, Value: %s\n", object->string, cJSON_Print(object));
        int Length = pathLength + strlen(object->string) + 1;
        name = (char *)malloc(Length);
        strcpy(name, path);
        if (!strstr(name, object->string))
            strcat(name, object->string);
        ParameterInfoStruct *parameterInfo = (ParameterInfoStruct *)malloc(sizeof(ParameterInfoStruct));
        parameterInfo->name = name;
        parameterInfo->writable = getWritable(name);
        (*index)++;
        *List = (ParameterInfoStruct **)realloc(*List, (*index) * sizeof(ParameterInfoStruct *));
        (*List)[*index - 1] = parameterInfo;
    }
}

/**
 * 从JSON文件中获取参数的属性，返回ParameterAttributeStruct指针数组
 * 参数parameter可以是完整路径，也可以是部分路径，部分路径则返回包含该部分路径的所有参数属性
 * 返回值：ParameterAttributeStruct指针数组
 */
ParameterAttributeStruct **getAttributesFromJson(const char *parameter)
{
    int length = strlen(parameter), count = 0, index = 1;
    char **pathList = getSubStrings(parameter, &count);

    cJSON *node = rootJSON;
    while (index < count)
    {
        node = cJSON_GetObjectItemCaseSensitive(node, pathList[index]);
        if (!node)
            break;
        index++;
    }
    freePath(pathList, count);

    if (!node)
    {
        printErrorInfo(FAULT_9005);
        return NULL;
    }

    if (cJSON_GetObjectItemCaseSensitive(node, "parameterType"))
    {
        // 是参数，直接封装返回
        ParameterAttributeStruct **paramAttributes = (ParameterAttributeStruct **)calloc(2, sizeof(ParameterAttributeStruct *));
        paramAttributes[0] = (ParameterAttributeStruct *)malloc(sizeof(ParameterAttributeStruct));

        paramAttributes[0]->Name = strdup(parameter); // 记得释放

        cJSON *accessList = cJSON_GetObjectItemCaseSensitive(node, "AccessList");
        int arraySize = cJSON_GetArraySize(accessList);
        paramAttributes[0]->AccessList = (char **)malloc((arraySize + 1) * sizeof(char *));
        for (size_t i = 0; i < arraySize; i++)
        {
            cJSON *accessItem = cJSON_GetArrayItem(accessList, i);
            const char *accessValue = cJSON_GetStringValue(accessItem);
            paramAttributes[0]->AccessList[i] = strdup(accessValue);
        }
        paramAttributes[0]->AccessList[arraySize] = NULL; // 指针数组结束标志
        paramAttributes[0]->Notification = atoi(cJSON_GetObjectItemCaseSensitive(node, "Notification")->valuestring);
        paramAttributes[1] = NULL; // 数组结束标志

        return paramAttributes;
    }
    else
    {
        // 循环递归找参数
        ParameterAttributeStruct **paramAttributes = (ParameterAttributeStruct **)malloc(sizeof(ParameterAttributeStruct *));
        int length = 0;
        paramAttributes[0] = NULL;
        cJSON *child = node->child;
        while (child)
        {
            char *newPath = (char *)malloc(strlen(parameter) + strlen(child->string) + 2);
            strcpy(newPath, parameter);
            strcat(newPath, child->string);
            if (!cJSON_GetObjectItemCaseSensitive(child, "parameterType"))
                strcat(newPath, ".");

            ParameterAttributeStruct **param = getAttributesFromJson(newPath);
            int len = 0;
            while (param && param[len])
                len++;
            paramAttributes = (ParameterAttributeStruct **)realloc(paramAttributes, (length + len + 1) * sizeof(ParameterAttributeStruct *));
            for (size_t i = 0; i < len; i++)
            {
                paramAttributes[length + i] = param[i];
            }
            paramAttributes[length + len] = NULL;
            free(param);
            length += len;
            child = child->next;
        }
        return paramAttributes;
    }
}

/**
 * 从JSON文件中获取上报参数，返回param_info指针数组
 */
param_info **getInfoParamFromJson(const char *parameter)
{
    int length = strlen(parameter), count = 0, index = 1;
    char **pathList = getSubStrings(parameter, &count);

    cJSON *node = rootJSON;
    while (index < count)
    {
        node = cJSON_GetObjectItemCaseSensitive(node, pathList[index]);
        if (!node)
            break;
        index++;
    }
    freePath(pathList, count);

    if (!node)
    {
        printErrorInfo(FAULT_9005);
        return NULL;
    }

    if (cJSON_GetObjectItemCaseSensitive(node, "parameterType"))
    {
        int n = atoi(cJSON_GetObjectItemCaseSensitive(node, "Notification")->valuestring);
        if (n == 0)
            return NULL;
        // 是参数，直接封装返回
        param_info **info_param = (param_info **)calloc(2, sizeof(param_info *));
        info_param[0] = (param_info *)malloc(sizeof(param_info));

        info_param[0]->name = strdup(parameter); // 记得释放
        info_param[0]->type = strdup(cJSON_GetObjectItemCaseSensitive(node, "parameterType")->valuestring);
        info_param[0]->data = strdup(cJSON_GetObjectItemCaseSensitive(node, "value")->valuestring);
        info_param[1] = NULL; // 数组结束标志

        return info_param;
    }
    else
    {
        // 循环递归找参数
        param_info **info_param = (param_info **)malloc(sizeof(param_info *));
        int length = 0;
        info_param[0] = NULL;
        cJSON *child = node->child;
        while (child)
        {
            char *newPath = (char *)malloc(strlen(parameter) + strlen(child->string) + 2);
            strcpy(newPath, parameter);
            strcat(newPath, child->string);
            if (!cJSON_GetObjectItemCaseSensitive(child, "parameterType"))
                strcat(newPath, ".");

            param_info **info = getInfoParamFromJson(newPath);
            int len = 0;
            while (info && info[len])
                len++;
            info_param = (param_info **)realloc(info_param, (length + len + 1) * sizeof(param_info *));
            for (size_t i = 0; i < len; i++)
            {
                info_param[length + i] = info[i];
            }
            info_param[length + len] = NULL;
            free(info);
            length += len;
            child = child->next;
        }
        return info_param;
    }
}

/**
 * 获取json对象下的所有参数，若传入的node就是参数，着返回自身数据
 */
param_info **getParameterlistByJsonNode(cJSON *node, const char *p_name)
{
    param_info **pf = (param_info **)malloc(sizeof(param_info *));
    pf[0] = NULL;
    if (cJSON_GetObjectItemCaseSensitive(node, "parameterType"))
    { // 完整参数路径
        pf = (param_info **)realloc(pf, sizeof(param_info *) * 2);
        pf[0] = (param_info *)malloc(sizeof(param_info));
        pf[0]->name = strdup(p_name);
        pf[0]->fault_code = FAULT_0;
        pf[0]->type = strdup(cJSON_GetObjectItemCaseSensitive(node, "parameterType")->valuestring);
        pf[0]->data = strdup(cJSON_GetObjectItemCaseSensitive(node, "value")->valuestring);
        pf[1] = NULL;
        return pf;
    }
    else
    { // 不完整
        int length = 0;
        cJSON *child = node->child;
        while (child)
        {
            char *newPath = (char *)malloc(strlen(p_name) + strlen(child->string) + 2);
            strcpy(newPath, p_name);
            strcat(newPath, child->string);
            if (!cJSON_GetObjectItemCaseSensitive(child, "parameterType"))
                strcat(newPath, ".");

            param_info **list = getParameterlistByJsonNode(child, newPath);
            int len = 0;
            while (list && list[len])
                len++;
            pf = (param_info **)realloc(pf, (length + len + 1) * sizeof(param_info *));
            for (size_t i = 0; i < len; i++)
            {
                pf[length + i] = list[i];
            }
            pf[length + len] = NULL;
            free(list);
            length += len;
            child = child->next;
        }
        return pf;
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
        if (!cJSON_GetObjectItemCaseSensitive(child, "parameterType"))
            printAllParameters(child, destination);
        else
        {
            cJSON *value = cJSON_GetObjectItemCaseSensitive(child, "value");
            printf("{ \" parameter \" : \" %s%s \"}, { \" value \" : \"%s\"}\n", destination, child->string, value->valuestring);
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
char **getSubStrings(const char *input, int *count)
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

/**
 * 下载文件
 */
int download_file_to_dir(const char *url, const char *username, const char *password, const char *dir)
{
    CURL *curl;
    CURLcode res;
    FILE *fp;
    // 从 URL 中提取文件名
    const char *filename = strrchr(url, '/');
    if (!filename)
    {
        fprintf(stderr, "Invalid URL.\n");
        return 0;
    }
    printf("url is %s", url);
    filename++;                // 移除斜杠
    char output[FILENAME_MAX]; // 下载后保存的文件名
    snprintf(output, sizeof(output), "%s/%s", dir, filename);

    curl = curl_easy_init();
    if (curl)
    {
        fp = fopen(output, "wb");
        if (fp)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            // 设置用户名和密码（如果有的话）
            if (username && password)
            {
                curl_easy_setopt(curl, CURLOPT_USERPWD, username);
            }
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            res = curl_easy_perform(curl); // 程序会等待直到下载操作完成才会继续执行后续代码
            if (res != CURLE_OK)
            {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                fclose(fp);
                curl_easy_cleanup(curl);
                return 0; // 下载失败
            }
            fclose(fp);
        }
        curl_easy_cleanup(curl);
    }
    return 1; // 下载成功
}