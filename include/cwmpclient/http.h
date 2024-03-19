/**
 * @Copyright : Yangrongcan
*/
#if !defined(_CWMP_HTTP_)
#define _CWMP_HTTP_

#include <curl/curl.h>

static const char *fc_cookies = "./backup/easycwmp_cookies";
struct http_client
{
	curl_slist *header_list;
	char *url;
};

int http_client_init(void);
void http_client_exit(void);
static size_t http_get_response(char *buffer, size_t size, size_t rxed, char **msg_in);

#endif // _CWMP_HTTP_
