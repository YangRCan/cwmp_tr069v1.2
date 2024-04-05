/**
 * @Copyright : Yangrongcan
 */
#ifndef _CWMP_SIMPLE_
#define _CWMP_SIMPLE_

#define HELP_INFO                                                                                            \
    "-Help\n"                                                                                                \
    "Please use this program with parameters in the following format:\n"                                     \
    "\t-----\tget value [parameter / object]\n"                                                              \
    "\t-----\tget name [object / parameter] NextLevel\n"                                                     \
    "\t-----\tThe value of NextLevel can only be 0 or 1. 0 indicates that the object/parameter\n"            \
    "\tand all its child objects or parameters are listed. 1 indicates that all parameters\n"                \
    "\tincluded in the path are listed. If the path is empty and NextLevel is 1, only ROOT will be listed\n" \
    "\t-----\tset [parameter] [value]\n"                                                                     \
    "\t-----\tadd [object]\n"                                                                                \
    "\t-----\tdelete [object]\n"                                                                             \
    "\t-----\tdownload [url] [fileType] [fileSize] [username] [password]\n"                                  \
    "\t-----\tupload [url] [fileType] [username] [password]\n"                                               \
    "\t-----\tfactory_reset\n"                                                                               \
    "\t-----\treboot\n"                                                                                      \
    "\t-----\tinform [parameter|device_id]\n"

#include "parameter.h"

typedef struct
{
    const char *key;
    ExecuteResult (*function)();
} Operate;

void init();
void printHelp();

extern Operate options[];

#endif