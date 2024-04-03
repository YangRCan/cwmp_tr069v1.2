/**
 * @Copyright : Yangrongcan
*/
#if !defined(_CWMP_HTTP_)
#define _CWMP_HTTP_

#include <curl/curl.h>
#include <string>

static const char *fc_cookies = "./backup/easycwmp_cookies";
struct http_client
{
	curl_slist *header_list;
	std::string url;
};

int http_client_init(void);
static size_t http_get_response(void *ptr, size_t size, size_t nmemb, void *userdata);
int8_t http_send_message(std::string msg_out, std::string &msg_in);
void http_client_exit(void);

#endif // _CWMP_HTTP_
