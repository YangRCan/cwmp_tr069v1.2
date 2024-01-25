/**
 * @Copyright : Yangrongcan
*/
#include <stdio.h>
#include <string.h>

#include "parameter.h"
#include "time_utils.h"
#include "cwmpdata.h"
#include "operate.h"

Operate operates[] = {
    {"get", getParameter},
    {"set", setParameter}
};

/**
 * 数据初始化
*/
void init()
{
    init_data();
    init_root();

    int Faultcode = addObjectToData("Device.DHCPv4.Server.{i}.", CreateObject);
    Faultcode = addObjectToData("Device.WiFi.AccessPoint.", CreateObject);
    iterateData(NULL, NULL);
    
    
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
                if(strcmp(operate, "get") == 0 && strcmp(argv[2], "all") == 0) {
                    getAllParameters();
                    break;
                } else if(strcmp(operate, "get") == 0) {
                    operates[i].function(argv[2]);
                    break;
                } else if(strcmp(operate, "set") == 0 && argc > 3) {
                    operates[i].function(argv[2], argv[3]);
                    break;
                } else {
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