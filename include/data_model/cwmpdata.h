/**
 * @Copyright : Yangrongcan
 */
#ifndef _CWMP_SIMPLE_
#define _CWMP_SIMPLE_

#define HELP_INFO                                                                                            \
    "-Help\n"                                                                                                \
    "Please use this program with parameters in the following format:\n"                                     \
    "\t--get_values\n"                                                                                    \
    "\t--get_value [parameter / object]\n"                                                              \
    "\t--get_name [object / parameter] NextLevel\n"                                                     \
    "\tThe value of NextLevel can only be 0 or 1. 0 indicates that the object/parameter\n"            \
    "\tand all its child objects or parameters are listed. 1 indicates that all parameters\n"                \
    "\tincluded in the path are listed. If the path is empty and NextLevel is 1, only ROOT will be listed\n" \
    "\t--set [parameter] [value]\n"                                                                     \
    "\t--add [object]\n"                                                                                \
    "\t--delete [object]\n"                                                                             \
    "\t--download [url] [fileType] [fileSize] [username] [password]\n"                                  \
    "\t--upload [url] [fileType] [username] [password]\n"                                               \
    "\t--factory_reset\n"                                                                               \
    "\t--reboot\n"                                                                                      \
    "\t--inform [parameter|device_id]\n"

#include "parameter.h"

void init();
void printHelp();

#endif