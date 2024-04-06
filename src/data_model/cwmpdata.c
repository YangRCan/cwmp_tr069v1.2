/**
 * @Copyright : Yangrongcan
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "time_utils.h"
#include "cwmpdata.h"

struct option arg_opts[] = {
    {"get_values", no_argument, NULL, 'g'},
    {"get_value", required_argument, NULL, 'v'},
    {"get_name", required_argument, NULL, 'n'},
    {"set", required_argument, NULL, 's'},
    {"add", required_argument, NULL, 'a'},
    {"delete", required_argument, NULL, 'e'},
    {"download", required_argument, NULL, 'd'},
    {"upload", required_argument, NULL, 'u'},
    {"inform", required_argument, NULL, 'i'},
    {"factory_reset", no_argument, NULL, 'f'},
    {"reboot", no_argument, NULL, 'r'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/**
 * 数据初始化
 */
void init()
{
    init_root(); // 必须先执行
    init_dataModel();
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
    
    int c;
    while (true)
    {
        c = getopt_long(argc, argv, "gvnsaeduifrh", arg_opts, NULL);
        if (c == EOF)
            break;
        switch (c)
        {
        case 'g':
            getAllParameters();
            break;
        case 'v':
            if(argc == 3) {
                param_info *pf = NULL;
                param_info **pf_list = (param_info **)malloc(sizeof(param_info *));
                pf_list[0] = pf;
                getParameter(argv[2], &pf_list);
                int len = 0;
                while (pf_list[len] != NULL)
                {
                    printf("{ \" parameter \" : \" %s \"}, { \" value \" : \"%s\"}, { \" type \" : \"%s\"}\n", pf_list[len]->name, pf_list[len]->data, pf_list[len]->type);
                    free(pf_list[len]->name);
                    free(pf_list[len]->data);
                    free(pf_list[len]);
                    len++;
                }
                free(pf_list);
            }
            break;
        case 'n':
            if(argc > 2) {
                ParameterInfoStruct *parameterInfoStruct = NULL; // 结束标志
                ParameterInfoStruct **List = (ParameterInfoStruct **)malloc(sizeof(ParameterInfoStruct *));
                List[0] = parameterInfoStruct;
                argc == 3 ? getParameterNames(NULL, argv[2], &List) : getParameterNames(argv[2], argv[3], &List);
                int len = 0;
                while (List[len] != NULL)
                {
                    printf("{ \" parameter \" : \" %s \"}, { \" writable \" : \" %d \"}\n", List[len]->name, List[len]->writable);
                    free(List[len]->name);
                    free(List[len]);
                    len++;
                }
                free(List);
            }
            break;
        case 's':
            if (argc == 4)
            {
                ExecuteResult rlt = setParameter(argv[2], argv[3], MUSTVERIFY);//只会修改全局的rootJSON树
                if(rlt.status) {
                    save_data();//应用该修改，保存到持久化文件中
                    printf("Successfully modified\n");
                }
            }
            break;
        case 'a':
            if (argc == 3)
            {
                char *str;
                ExecuteResult rlt = addObject(argv[2], &str);
                if(rlt.status) printf("{ \" parameter \" : \" %s \"} New successfully created, instance number: %s.\n", argv[2], str);
                save_data();
            }
            break;
        case 'e':
            if (argc == 3)
            {
                ExecuteResult rlt = deleteObject(argv[2]);
                if(rlt.status) printf("{ \" parameter \" : \" %s \"} has been deleted.\n", argv[2]);
                save_data();
            }
            break;
        case 'd':
            if (argc == 7)
            {
                downloadFile(argv[2], argv[3], argv[4], argv[5], argv[6]);
            }
            break;
        case 'u':
            if (argc >= 4)
            {
                if (argc == 4)
                    uploadFile(argv[2], argv[3], NULL, NULL);
                else
                    uploadFile(argv[2], argv[3], argv[4], argv[5]);
            }
            break;
        case 'i':
            if (argc >= 2)
            {
                param_info **inform_parameter = NULL;
                if (argc == 2 || strcmp(argv[2], "parameter") == 0)
                    inform_parameter = getInformParameter();
                // else if (argc > 2 && strcmp(argv[2], "device_id"))
                //     inform_parameter;
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
            }
            break;
        case 'f':
            if(argc == 2)
            {
                do_factory_reset();
            }
            break;
        case 'r':
            if(argc == 2)
            {
                do_reboot();
            }
            break;
        case 'h':
            printHelp();
            exit(EXIT_SUCCESS);
        default:
            printHelp();
            exit(EXIT_FAILURE);
        }
    }

    if(argc == 1) printHelp();

    return 0;
}