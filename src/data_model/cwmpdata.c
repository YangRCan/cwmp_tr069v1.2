/**
 * @Copyright : Yangrongcan
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "time_utils.h"
#include "cwmpdata.h"

Operate operates[] = {
    {"get", getParameter},
    {"set", setParameter},
    {"add", addObject},
    {"delete", deleteObject},
    {"download", downloadFile},
    {"upload", uploadFile},
    {"inform", NULL}};

/**
 * 数据初始化
 */
void init()
{
    init_root(); // 必须先执行
    init_dataModel();

    // 打印dataModal
    // iterateDataModel(NULL, NULL);

    // addObject测试
    // char *str;
    // addObject("Device.IP.Interface.{i}.", &str);
    // if(str) printf("创建成功，实例号为: %s", str);
    // addObject("Device.IP.Interface.");
    // addObject("Device.IP.Interface.1.IPv4Address.");

    // int count;
    // char *str = "InternetGatewayDevice.WANDevice.1.WANConnectionDevice.2.WANPPPConnection.1.Password";
    // const char *delimiter = ".";
    // char **result = GetSubstrings(str, delimiter, &count);
    // printf("路径长度为%d\n", count);
    // for (size_t i = 0; i < count; i++)
    // {
    //     printf("--%s--\n", result[i]);
    // }

    // 测试setParameterAttributes
    // char *str[] = {"测试1", "测试3"};
    // ParameterAttributeStruct param;
    // param.AccessList = str;
    // param.Name = "Device.DeviceInfo.MemoryStatus.Total";
    // param.Notification = 1;
    // setParameterAttributes(&param, true, true, 2);

    // 测试getParameterAttributes
    // char *str[] = {"", "Device.DeviceInfo.MemoryStatus.Total"};
    // ParameterAttributeStruct **param = getParameterAttributes((const char *const *)str, 2);
    // int index = 0;
    // while (param && param[index])
    // {
    //     printf("{ \" parameter \" : \" %s \"}, { \" Notification \" : \" %d \"}\n", param[index]->Name, param[index]->Notification);
    //     int len = 0;
    //     while (param[index]->AccessList[len])
    //     {
    //         printf("ACCESS HAVE: %s\n", param[index]->AccessList[len]);
    //         free(param[index]->AccessList[len]);
    //         len++;
    //     }
    //     free(param[index]->AccessList);
    //     free(param[index]->Name);
    //     free(param[index]);
    //     index++;
    // }
    // if (param)
    //     free(param);
}

/**
 * 打印提示信息
 */
void printHelp()
{
    printf("%s", HELP_INFO);
}

int main(int argc, char *argv[])
{
    init();

    if (argc >= 2)
    {
        char *operate = argv[1];
        size_t i = 0;
        for (; i < sizeof(operates) / sizeof(operates[0]); i++)
        {
            if (strcmp(operate, operates[i].key) == 0)
            {
                if (argc == 3 && strcmp(operate, "get") == 0 && strcmp(argv[2], "value") == 0)
                {
                    getAllParameters();
                    break;
                }
                else if (argc == 4 && strcmp(operate, "get") == 0 && strcmp(argv[2], "value") == 0)
                {
                    char **str = (char **)malloc(sizeof(char *));
                    *str = NULL;
                    operates[i].function(argv[3], str);
                    if (*str != NULL)
                        printf("{ \" parameter \" : \" %s \"}, { \" value \" : \"%s\"}\n", argv[3], *str);
                    if (*str)
                        free(*str);
                    free(str);
                    break;
                }
                else if (argc > 3 && strcmp(operate, "get") == 0 && strcmp(argv[2], "name") == 0)
                {
                    ParameterInfoStruct *parameterInfoStruct = NULL; // 结束标志
                    ParameterInfoStruct **List = (ParameterInfoStruct **)malloc(sizeof(ParameterInfoStruct *));
                    List[0] = parameterInfoStruct;
                    argc == 4 ? getParameterName(NULL, argv[3], &List) : getParameterName(argv[3], argv[4], &List);
                    int len = 0;
                    while (List[len] != NULL)
                    {
                        printf("{ \" parameter \" : \" %s \"}, { \" writable \" : \" %d \"}\n", List[len]->name, List[len]->writable);
                        free(List[len]->name);
                        free(List[len]);
                        len++;
                    }
                    free(List);
                    break;
                }
                else if (argc == 4 && strcmp(operate, "set") == 0)
                {
                    ExecuteResult rlt = operates[i].function(argv[2], argv[3], MUSTVERIFY);
                    if(rlt.status) printf("Successfully modified\n");
                    break;
                }
                else if (argc == 3 && strcmp(operate, "add") == 0)
                {
                    char *str;
                    ExecuteResult rlt = operates[i].function(argv[2], &str);
                    if(rlt.status) printf("{ \" parameter \" : \" %s \"} New successfully created, instance number: %s", argv[2], str);
                    break;
                }
                else if (argc == 3 && strcmp(operate, "delete") == 0)
                {
                    ExecuteResult rlt = operates[i].function(argv[2]);
                    if(rlt.status) printf("{ \" parameter \" : \" %s \"} has been deleted ", argv[2]);
                    break;
                }
                else if (argc == 7 && strcmp(operate, "download") == 0)
                {
                    operates[i].function(argv[2], argv[3], argv[4], argv[5], argv[6]);
                    break;
                }
                else if (argc >= 4 && strcmp(operate, "upload") == 0)
                {
                    if (argc == 4)
                        operates[i].function(argv[2], argv[3], NULL, NULL);
                    else
                        operates[i].function(argv[2], argv[3], argv[4], argv[5]);
                    break;
                }
                else if (argc >= 2 && strcmp(operate, "inform") == 0)
                {
                    InformParameter **inform_parameter = NULL;
                    if (argc == 2 || strcmp(argv[2], "parameter") == 0)
                        inform_parameter = getInformParameter();
                    else if (argc > 2 && strcmp(argv[2], "device_id"))
                        inform_parameter;
                    if(!inform_parameter) break;
                    for (size_t i = 0; inform_parameter[i] != NULL ; i++)
                    {
                        printf("{ \" parameter \" : \" %s \"}, { \" value \" : \" %s \"}, { \" type \" : \" %s \"}\n", inform_parameter[i]->name, inform_parameter[i]->data, inform_parameter[i]->type);
                        free(inform_parameter[i]->name);
                        free(inform_parameter[i]->type);
                        free(inform_parameter[i]->data);
                        free(inform_parameter[i]);
                    }
                    free(inform_parameter);
                    break;
                }
                else
                {
                    printHelp();
                    break;
                }
            }
        }

        if (i >= sizeof(operates) / sizeof(operates[0]))
        {
            printHelp();
        }
    }
    else
    {
        printHelp();
    }
    return 0;
}