#include "curl_pool.h"

void init_curl_pool()
{
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		pr_err("curl_global_init() failed: %s", curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}
}

void destroy_curl_pool()
{
	curl_global_cleanup();
}
