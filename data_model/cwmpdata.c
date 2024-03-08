/**
 * @Copyright : Yangrongcan
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parameter.h"
#include "time_utils.h"
#include "cwmpdata.h"

Operate operates[] = {
    {"get", getParameter},
    {"set", setParameter},
    {"add", addObject}};

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
    // addObject("Device.DHCPv4.Server.Pool.");
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

    if (argc > 2)
    {
        char *operate = argv[1];
        size_t i = 0;
        for (; i < sizeof(operates) / sizeof(operates[0]); i++)
        {
            if (strcmp(operate, operates[i].key) == 0)
            {
                if (strcmp(operate, "get") == 0 && strcmp(argv[2], "value") == 0 && argc == 3)
                {
                    getAllParameters();
                    break;
                }
                else if (strcmp(operate, "get") == 0 && strcmp(argv[2], "value") == 0)
                {
                    char **str;
                    operates[i].function(argv[2], str);
                    printf("{ \" parameter \" : \" %s \"}, { \" value \" : \" %s \"}\n", argv[2], *str);
                    break;
                }
                else if (strcmp(operate, "get") == 0 && strcmp(argv[2], "name") == 0 && argc >= 4)
                {
                    ParameterInfoStruct *parameterInfoStruct = NULL; // 结束标志
                    ParameterInfoStruct **List = &parameterInfoStruct;
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
                else if (strcmp(operate, "set") == 0 && argc > 3)
                {
                    operates[i].function(argv[2], argv[3]);
                    break;
                }
                else if (strcmp(operate, "add") == 0 && argc > 2)
                {
                    operates[i].function(argv[2]);
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