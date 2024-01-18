/**
 * @Copyright : Yangrongcan
*/
#include <string.h>

#include "parameter.h"
#include "time_utils.h"
#include "cwmpdata.h"

Operate operates[] = {
    {"get", getParameter},
    {"set", setParameter}
};

/**
 * 数据初始化
*/
void init()
{
    
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
void getParameter(char* name)
{
    
}

/**
 * 修改某属性名对应的配置的值
 * （注：前提这个属性允许被修改）
*/
void setParameter(char *name, char *value)
{
    
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
                    getAllProperties();
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