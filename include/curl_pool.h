#ifndef _CURL_POOL_H
#define _CURL_POOL_H

#include <curl/curl.h>
#include "common.h"

#define MAX_POOL_SIZE 10
#define CONNECTION_TIMEOUT 2L
#define TRANSFER_TIMEOUT 5L

typedef struct {
	CURL *handle;
	bool in_use;
	time_t last_used;
} curl_handle_t;

typedef struct {
	curl_handle_t handles[MAX_POOL_SIZE];
	int size;
	pthread_mutex_t lock;
} curl_pool_t;

void init_curl_pool();
void destroy_curl_pool();
bool get_curl_pool_handle(CURL **handle);
void release_curl_pool_handle(CURL *handle);

#endif//_CURL_POOL_H
