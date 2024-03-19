/**
 * @Copyright : Yangrongcan
 */
#if !defined(_CWMP_CLIENT_)
#define _CWMP_CLIENT_

#define NAME "cwmpclient"
#define HELP_INFO                                                                          \
    "Please use %s [OPTIONS]\n"                                                           \
    " -f, --foreground        Run in the foreground\n"                                    \
    " -b, --boot              Run with \"1 BOOT\" event\n"                                \
    " -g, --getrpcmethod      Run with \"2 PERIODIC\" event and with ACS GetRPCMethods\n" \
    " -h, --help              Display this help text\n"                                   \
    " -v, --version           Display the %s version\n"

#define FREE(x) do { free(x); x = NULL; } while (0);

enum start_event_enum {
	START_BOOT = 0x1,
	START_GET_RPC_METHOD = 0x2
};

#endif // _CWMP_CLIENT_
