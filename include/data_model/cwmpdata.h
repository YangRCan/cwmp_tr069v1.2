/**
 * @Copyright : Yangrongcan
 */
#ifndef _CWMP_SIMPLE_
#define _CWMP_SIMPLE_

#define HELP_INFO                                                                                        \
    "-Help\n"                                                                                            \
    "Please use this program with parameters in the following format:\n"                                 \
    "\t-----\tget value [parameter]\n"                                                                   \
    "\t-----\tget name [object/parameter] NextLevel\n"                                                   \
    "\t-----\t  The value of NextLevel can only be 0 or 1. 0 indicates that the object/parameter"        \
    "\tand all its child objects or parameters are listed. 1 indicates that all parameters"              \
    "included in the path are listed. If the path is empty and NextLevel is 1, only ROOT will be listed" \
    "\t-----\tset parameter value\n"                                                                     \
    "\t-----\tadd [object]\n"                                                                            \
    "\t-----\tdelete [object]\n"                                                                         \
    "\t-----\tdownload\n"                                                                                \
    "\t-----\tupload\n"                                                                                  \
    "\t-----\tfactory_reset\n"                                                                           \
    "\t-----\treboot\n"                                                                                  \
    "\t-----\tinform [parameter|device_id]"

typedef struct
{
    const char *key;
    void (*function)();
} Operate;

void init();
void printHelp();

extern Operate options[];

#endif