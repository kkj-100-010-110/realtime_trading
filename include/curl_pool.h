#ifndef _CURL_POOL_H
#define _CURL_POOL_H

#include <curl/curl.h>
#include "common.h"

void init_curl_pool();
void destroy_curl_pool();

#endif//_CURL_POOL_H
