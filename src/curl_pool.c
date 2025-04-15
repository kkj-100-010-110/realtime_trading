#include "curl_pool.h"

static curl_pool_t *pool = NULL;

void init_curl_pool()
{
	/* curl global init() */
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		pr_err("curl_global_init() failed: %s", curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}

	/* create curl_pool */
	pool = (curl_pool_t *)malloc(sizeof(curl_pool_t));
	if (!pool) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}
	if (pthread_mutex_init(&pool->lock, NULL) != 0) {
		pr_err("pthread_mutex_init() failed.");
		free(pool);
		exit(EXIT_FAILURE);
	}
	pool->size = MAX_POOL_SIZE;

	// init curl handles
	for (int i = 0; i < MAX_POOL_SIZE; i++) {
		pool->handles[i].handle = curl_easy_init();
		pool->handles[i].in_use = false;
		pool->handles[i].last_used = time(NULL);

		// set common options
		curl_easy_setopt(pool->handles[i].handle, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(pool->handles[i].handle, CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT);
		curl_easy_setopt(pool->handles[i].handle, CURLOPT_TIMEOUT, TRANSFER_TIMEOUT);
		curl_easy_setopt(pool->handles[i].handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
		//curl_easy_setopt(pool->handles[i].handle, CURLOPT_TCP_KEEPALIVE, 1L);
		//curl_easy_setopt(pool->handles[i].handle, CURLOPT_TCP_KEEPIDLE, 60L);
		//curl_easy_setopt(pool->handles[i].handle, CURLOPT_TCP_KEEPINTVL, 30L);
	}
}

void destroy_curl_pool()
{
	pthread_mutex_lock(&pool->lock);
	for (int i = 0; i < MAX_POOL_SIZE; i++) {
		if (pool->handles[i].handle) {
			curl_easy_cleanup(pool->handles[i].handle);
		}
	}
	pthread_mutex_unlock(&pool->lock);
	pthread_mutex_destroy(&pool->lock);

	free(pool);

	/* curl global cleanup() */
	curl_global_cleanup();
}

bool get_curl_pool_handle(CURL **handle)
{
	pthread_mutex_lock(&pool->lock);
	for (int i = 0; i < MAX_POOL_SIZE; i++) {
		if (!pool->handles[i].in_use) {
			pool->handles[i].in_use = true;
			pool->handles[i].last_used = time(NULL);
			*handle = pool->handles[i].handle;
			pthread_mutex_unlock(&pool->lock);
			return true;
		}
	}
	pthread_mutex_unlock(&pool->lock);
	*handle = NULL; // No available handles
	return false;
}

void release_curl_pool_handle(CURL *handle)
{
	pthread_mutex_lock(&pool->lock);
	for (int i = 0; i < MAX_POOL_SIZE; i++) {
		if (pool->handles[i].handle == handle) {
			pool->handles[i].in_use = false;

			// reset common options to reuse
			curl_easy_reset(handle);
			curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT);
			curl_easy_setopt(handle, CURLOPT_TIMEOUT, TRANSFER_TIMEOUT);
			curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

			//curl_easy_setopt(pool->handles[i].handle, CURLOPT_TCP_KEEPALIVE, 1L);
			//curl_easy_setopt(pool->handles[i].handle, CURLOPT_TCP_KEEPIDLE, 60L);
			//curl_easy_setopt(pool->handles[i].handle, CURLOPT_TCP_KEEPINTVL, 30L);
			break;
		}
	}
	pthread_mutex_unlock(&pool->lock);
}
