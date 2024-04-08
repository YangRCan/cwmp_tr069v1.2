/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <curl/curl.h>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#endif

#include "cwmpclient.h"
#include "cwmp.h"
#include "backup.h"
#include "CwmpConfig.h"
#include "log.h"
#include "time_tool.h"
#include "xml.h"
#include "tinyxml2.h"
#include "config.h"

cwmpInfo* cwmpInfo::cwmp = nullptr;
std::mutex cwmpInfo::create_mutex;
cwmpInfo* cwmp;

struct option arg_opts[] = {
    {"boot", no_argument, NULL, 'b'},
    {"foreground", no_argument, NULL, 'f'},
    {"getrpcmethod", no_argument, NULL, 'g'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}};

static void printHelp(void)
{
    printf(HELP_INFO, NAME, NAME);
}

static void printVersion(void)
{
    printf("%s version: %d.%d.%d\n", NAME, CWMP_VERSION_MAJOR, CWMP_VERSION_MINOR, CWMP_VERSION_PATCH);
}

int main(int argc, char **argv)
{
    int character;
    int startEvent = 0;
    bool foreground = false;
    while (true)
    {
        character = getopt_long(argc, argv, "bfghv", arg_opts, NULL);
        if (character == EOF)
            break;
        switch (character)
        {
        case 'b':
            startEvent |= START_BOOT; // 启动时上报
            break;
        case 'f':
            foreground = true; //是否为前台运行
            break;
        case 'g':
            startEvent |= START_GET_RPC_METHOD; // 启动时获取ACS的方法
            break;
        case 'h':
            printHelp();
            exit(EXIT_SUCCESS);
        case 'v':
            printVersion();
            exit(EXIT_SUCCESS);
        default:
            printHelp();
            exit(EXIT_FAILURE);
        }
    }

// 确保只有一个程序在运行，若没有管理员权限，退出
#ifdef __linux__
    // 对文件进行加锁，确保只存在一个该程序进程
    int fd = open(CONFIGFILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
        exit(EXIT_FAILURE);
    if (flock(fd, LOCK_EX | LOCK_NB) == -1)
        exit(EXIT_SUCCESS);
    if (fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC) < 0)
        Log(NAME, L_NOTICE, "error in fcntl\n");
    setlocale(LC_CTYPE, "");
    umask(0037);

    if (getuid() != 0)
    {
        Log(NAME, L_DEBUG, "Please run %s as root\n", NAME);
        exit(EXIT_FAILURE);
    }
    if(!foreground) {
        pid_t pid, sid;
        pid = fork();
        if (pid < 0) {
            Log(NAME, L_DEBUG, "fork() returned error\n");
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            Log(NAME, L_DEBUG, "Exit parent process\n");
            exit(EXIT_SUCCESS);//退出
        }

        sid = setsid();
        if (sid < 0) {
            Log(NAME, L_DEBUG, "setsid() returned error\n");
            exit(EXIT_FAILURE);
        }

        if ((chdir("/data/cwmp")) < 0) {
            Log(NAME, L_DEBUG, "chdir() returned error\n");
            exit(EXIT_FAILURE);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }
#elif _WIN32
    HANDLE hMutex = CreateMutex(NULL, TRUE, TEXT(NAME));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        printf("Another instance of the application is already running.\n");
        CloseHandle(hMutex);
        exit(EXIT_FAILURE);
    }

    BOOL bIsAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY; // 身份验证机构
    PSID administratorsGroup;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup))
    {
        CheckTokenMembership(NULL, administratorsGroup, &bIsAdmin);
        if (!bIsAdmin)
        {
            Log(NAME, L_DEBUG, "Please run %s as root\n", "cwmp");
            //exit(EXIT_FAILURE);
        }

        FreeSid(administratorsGroup);
    }
#endif

    cwmp = cwmpInfo::getCwmpInfoInstance();//获取单实例

    curl_global_init(CURL_GLOBAL_ALL);//多线程不安全，只在主函数调用一次
    backup_init();              // 从备份文件中加载uploads、downloads、events .etc.
    config_load();

    // 根据输入参数添加
    if (startEvent & START_BOOT)
    {
        cwmp->cwmp_add_event(EVENT_BOOT, "", 0, EVENT_BACKUP);
        cwmp->cwmp_add_inform_timer(1000); // 设置一个 inform 定时器，每隔 10 毫秒触发一次
    }
    if (startEvent & START_GET_RPC_METHOD)
    {
        cwmp->set_get_rpc_methods(true); // 设置cwmp对象中get_rpc_methods的值为true
        cwmp->cwmp_add_event(EVENT_PERIODIC, "", 0, EVENT_BACKUP);
        cwmp->cwmp_add_inform_timer(1000);
    }

#ifdef _WIN32
    CloseHandle(hMutex);
    ExitThread(0);//主动退出主线程
#elif __linux__
    Log(NAME, L_DEBUG, "Exit the child process and leave the task to the thread\n");
    pthread_exit(NULL);//退出子进程，剩下的事情由各个线程做
#endif
    
    return 0;
}