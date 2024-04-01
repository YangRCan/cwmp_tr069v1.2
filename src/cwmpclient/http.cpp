/**
 * @Copyright : Yangrongcan
 */
#include <iostream>
#include <curl/curl.h>

#include "http.h"
#include "cwmpclient.h"
#include "log.h"
#include "config.h"

static http_client http_c;
CURL *curl;
std::string http_redirect_url = "";

/**
 * 设置curl实例，和ACS的url
 */
int http_client_init(void)
{
    if (!http_redirect_url.empty())
    { // 如果不为空
        http_c.url = http_redirect_url;
    }
    else
    { // 否则就使用配置文件中的url
        http_c.url = config->acs->url;
    }

    Log(NAME, L_DEBUG, "######## HTTP CLIENT CONFIGURATION ########\n");
    Log(NAME, L_DEBUG, "url: %s\n", http_c.url.c_str());
    if (config->acs->ssl_cert.empty())
        Log(NAME, L_DEBUG, "ssl_cert: %s\n", config->acs->ssl_cert.c_str());
    if (config->acs->ssl_cacert.empty())
        Log(NAME, L_DEBUG, "ssl_cacert: %s\n", config->acs->ssl_cacert.c_str());
    if (!config->acs->ssl_verify)
        Log(NAME, L_DEBUG, "ssl_verify: SSL certificate validation disabled.\n");
    Log(NAME, L_DEBUG, "======== HTTP CLIENT CONFIGURATION ========\n");

    curl = curl_easy_init(); // 初始化 CURL 实例
    if (!curl)               // 初始化curl失败
        return -1;
    curl_easy_setopt(curl, CURLOPT_URL, http_c.url.c_str());                                              // 指定要请求的 URL
    curl_easy_setopt(curl, CURLOPT_USERNAME, config->acs->username.empty() ? "" : config->acs->username.c_str()); // 设置用户名，用于进行身份验证
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config->acs->password.empty() ? "" : config->acs->password.c_str()); // 设置密码，用于进行身份验证
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
    if (!config->acs->ssl_cert.empty())
        curl_easy_setopt(curl, CURLOPT_SSLCERT, config->acs->ssl_cert.c_str());
    if (!config->acs->ssl_cacert.empty())
        curl_easy_setopt(curl, CURLOPT_CAINFO, config->acs->ssl_cacert.c_str());
    if (!config->acs->ssl_verify)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    Log(NAME, L_NOTICE, "configured acs url %s\n", http_c.url.c_str());
    return 0; // 初始化成功
}

/**
 * 处理HTTP GET请求响应的函数
*/
// static size_t http_get_response(char *buffer, size_t size, size_t rxed, char **msg_in)
// {
//     std::string response(*msg_in);
//     response.append(buffer, size * rxed);
//     char *new_msg = strdup(response.c_str());
//     if (new_msg == nullptr)
//     {
//         free(*msg_in);
//         return -1;
//     }
//     free(*msg_in);
//     *msg_in = new_msg;
//     return size * rxed;
// }
static size_t http_get_response(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	//参数userdata是存放数据的指针  其他三个获取内容
	std::string *version = (std::string*)userdata;
	version->append((char*)ptr, size * nmemb);

	return (size * nmemb);
}

/**
 * 该函数用于发送 HTTP POST 请求并接收响应消息。msg_out 是待发送的消息，msg_in 是用于存储接收到的消息的指针
 * 若重定向会递归调用，重新发送
*/
int8_t http_send_message(std::string msg_out, std::string &msg_in)
{
	CURLcode res;
	char error_buf[CURL_ERROR_SIZE] = "";
    char *reception = (char*)malloc(sizeof(char));

	// 设置要发送的消息内容为 POST 请求体
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg_out.c_str());
	// 设置 HTTP 头信息
	http_c.header_list = NULL;
	http_c.header_list = curl_slist_append(http_c.header_list, "Accept:");
	if (!http_c.header_list) return -1;
	http_c.header_list = curl_slist_append(http_c.header_list, "User-Agent: cwmpclient");
	if (!http_c.header_list) return -1;
	http_c.header_list = curl_slist_append(http_c.header_list, "Content-Type: text/xml; charset=\"utf-8\"");
	if (!http_c.header_list) return -1;
	if (config->acs->http100continue_disable) {
		http_c.header_list = curl_slist_append(http_c.header_list, "Expect:");
		if (!http_c.header_list) return -1;
	}
	if (!msg_out.empty()) {//如果不为空
		//打印log
		Log(NAME, L_DEBUG, "+++ SEND HTTP REQUEST +++\n%s\n", msg_out.c_str());
		Log(NAME, L_DEBUG, "--- SEND HTTP REQUEST ---\n");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) msg_out.length());
		http_c.header_list = curl_slist_append(http_c.header_list, "SOAPAction;");// 添加 SOAPAction 头部
		if (!http_c.header_list) return -1;
	}
	else {//如果没有要传的xml数据
		Log(NAME, L_DEBUG, "+++ SEND EMPTY HTTP REQUEST +++\n");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);//数据长度为0
	}
	// 其他 HTTP 请求设置
	// 设置错误缓冲区
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buf);
	// 设置 HTTP 请求头部
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_c.header_list);
	// 设置接收到的数据存储位置
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &msg_in);//设置接收到的 HTTP 响应消息
	// 分配存储接收数据的内存
    
	//使用 curl_easy_perform 函数执行 HTTP 请求，并将执行结果保存在 res 变量中
	res = curl_easy_perform(curl);// 执行 HTTP 请求
	// 清理请求头部
	if (http_c.header_list) {
		curl_slist_free_all(http_c.header_list);
		http_c.header_list = NULL;
	}
	// 如果 LibCurl 返回错误信息，记录日志
	if (error_buf[0] != '\0')
		Log(NAME, L_NOTICE, "LibCurl Error: %s\n", error_buf);

    // if(reception) msg_in.assign(reception);
	//如果没有接收到消息，则释放占用的内存
    // FREE(reception);
	
	long httpCode = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);//获取 HTTP 状态码

	if (httpCode == 302 || httpCode == 307) {
		// 处理重定向，重新发送 HTTP 请求
		// 获取重定向 URL
        char *redirect_url = NULL;
		curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirect_url);
		// 赋值到 http_redirect_url
        http_redirect_url.assign(redirect_url);
		// 退出当前 HTTP 客户端
		http_client_exit();
		// 重新初始化 HTTP 客户端
		if (http_client_init()) {
			Log(NAME, L_DEBUG, "receiving http redirect: re-initializing http client failed\n");
			// 释放内存并返回错误
            http_redirect_url.clear();
			return -1;
		}
		// 释放内存
        http_redirect_url.clear();
		// 释放接收到的消息内存
        msg_in.clear();
		// 重定向至新 URL
		int redirect = http_send_message(msg_out, msg_in);
		return redirect;
	}

	//处理 HTTP 请求失败的情况，如果请求失败或者状态码不是成功或无内容，则返回错误
	if (res || (httpCode != 200 && httpCode != 204)) {
		Log(NAME, L_NOTICE, "sending http message failed\n");
		return -1;
	}

	if (!msg_in.empty()) {
		Log(NAME, L_DEBUG, "+++ RECEIVED HTTP RESPONSE +++\n%s\n", msg_in.c_str());
		Log(NAME, L_DEBUG, "--- RECEIVED HTTP RESPONSE ---\n");
	} else {
		Log(NAME, L_DEBUG, "+++ RECEIVED EMPTY HTTP RESPONSE +++\n");
	}
	// 返回成功状态
	return 0;
}

/**
 * 释放curl空间
*/
void http_client_exit(void)
{
    http_redirect_url.clear();
	if(curl) {
	    curl_easy_cleanup(curl);
		curl = NULL;
	}
	// curl_global_cleanup();//不是线程安全的

	if(remove(fc_cookies) < 0)//尝试删除 fc_cookies 文件。如果文件存在并且删除成功，则返回0
		Log(NAME, L_NOTICE, "can't remove file %s\n", fc_cookies);
}

