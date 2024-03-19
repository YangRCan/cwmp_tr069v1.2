/**
 * @Copyright : Yangrongcan
 */
#include <curl/curl.h>

#include "http.h"
#include "cwmpclient.h"
#include "log.h"
#include "config.h"

static http_client http_c;
CURL *curl;
char *http_redirect_url = NULL;

/**
 * 设置curl实例，和ACS的url
 */
int http_client_init(void)
{
    if (http_redirect_url)
    { // 如果不为空
        if ((http_c.url = strdup(http_redirect_url)) == NULL)
            return -1;
    }
    else
    { // 否则就使用配置文件中的url
        if ((http_c.url = strdup(config->acs->url.c_str())) == NULL)
            return -1;
    }

    Log(NAME, L_DEBUG, "######## HTTP CLIENT CONFIGURATION ########\n");
    Log(NAME, L_DEBUG, "url: %s\n", http_c.url);
    if (config->acs->ssl_cert.empty())
        Log(NAME, L_DEBUG, "ssl_cert: %s\n", config->acs->ssl_cert);
    if (config->acs->ssl_cacert.empty())
        Log(NAME, L_DEBUG, "ssl_cacert: %s\n", config->acs->ssl_cacert);
    if (!config->acs->ssl_verify)
        Log(NAME, L_DEBUG, "ssl_verify: SSL certificate validation disabled.\n");
    Log(NAME, L_DEBUG, "======== HTTP CLIENT CONFIGURATION ========\n");

    curl = curl_easy_init(); // 初始化 CURL 实例
    if (!curl)               // 初始化curl失败
        return -1;
    curl_easy_setopt(curl, CURLOPT_URL, http_c.url);                                              // 指定要请求的 URL
    curl_easy_setopt(curl, CURLOPT_USERNAME, config->acs->username.empty() ? config->acs->username : ""); // 设置用户名，用于进行身份验证
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config->acs->password.empty() ? config->acs->password : ""); // 设置密码，用于进行身份验证
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC | CURLAUTH_DIGEST);                   // 设置 HTTP 认证类型，此处同时指定了 BASIC 和 DIGEST 两种认证方式
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_get_response);                             // 指定一个回调函数，当接收到数据时会调用这个函数进行处理
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);                                                  // 设置请求超时时间为 30 秒
#ifdef DEVEL                                                                                      // 若存在宏
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);                                                  // 让 libcurl 输出更多详细的调试信息
#endif                                                                                            /* DEVEL */
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, fc_cookies);                                       // 从指定文件读取 Cookie "/tmp/easycwmp_cookies"
    /**
     * 将收到的服务器响应中的 Cookie 保存到指定的文件中。在这里，将 Cookie 保存到 fc_cookies 指定的路径下的文件中。随后的请求可以使用这些 Cookie 来保持会话或进行身份验证
     */
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, fc_cookies);
    // 如果使用ssl验证就设置
    if (config->acs->ssl_cert.empty())
        curl_easy_setopt(curl, CURLOPT_SSLCERT, config->acs->ssl_cert);
    if (config->acs->ssl_cacert.empty())
        curl_easy_setopt(curl, CURLOPT_CAINFO, config->acs->ssl_cacert);
    if (!config->acs->ssl_verify)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    Log(NAME, L_NOTICE, "configured acs url %s\n", http_c.url);
    return 0; // 初始化成功
}

void http_client_exit(void)
{
	FREE(http_c.url);

	if(curl) {
	    curl_easy_cleanup(curl);
		curl = NULL;
	}
	curl_global_cleanup();

	if(remove(fc_cookies) < 0)
		Log(NAME, L_NOTICE, "can't remove file %s\n", fc_cookies);
}

static size_t http_get_response(char *buffer, size_t size, size_t rxed, char **msg_in)
{
    std::string response(*msg_in);
    response.append(buffer, size * rxed);
    char *new_msg = strdup(response.c_str());
    if (new_msg == nullptr)
    {
        free(*msg_in);
        return -1;
    }
    free(*msg_in);
    *msg_in = new_msg;
    return size * rxed;
}