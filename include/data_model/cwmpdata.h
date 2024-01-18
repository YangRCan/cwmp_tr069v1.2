/**
 * @Copyright : Yangrongcan
*/
#ifndef _CWMP_SIMPLE_
#define _CWMP_SIMPLE_

#define HELP_INFO                                                        \
    "-Help\n"                                                            \
    "Please use this program with parameters in the following format:\n" \
    "\t-----\tget property\n"                                            \
    "\t-----\tset property value\n"                                      \
    "\t-----\tget all"

typedef struct
{
    const char *key;
    void (*function)();
} Operate;

void init();
void getAllProperties();
void getProperty(char *name);
void setProperty();
void printHelp();

extern Operate options[];

#endif