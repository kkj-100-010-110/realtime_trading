#include "common.h"

#include "websocket.h"
#include "rest_api.h"
#include "log.h"

#include "thread_queue.h"

#include <stdatomic.h>

atomic_bool ws_running = true;

void load_env();
void *websocket_thread(void *arg);

int main(void)
{
	/* clean extern variables */
	//atexit(clean_extern_json);

	/* create log_file and start logging on it */
	init_logging();

	/* setup env */
	load_env();

	/* setup config */
	set_json_config();

	/* libcurl init */
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		pr_err("libcurl init failed: %s", curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}

	/* setup thread queue */
	thread_queue_init();

	/* websocket thread */
	pthread_t ws_thread;

	/* configure socket and connect */
	struct lws_context *context = init_socket();
	struct lws *wsi = connect_websocket(context);
	if (!wsi) {
		lwsl_err("connect_websocket() failed.\n");
		cleanup_websocket(context);
		return -1;
	}

	/* Websocket thread run */
	if (pthread_create(&ws_thread, NULL, websocket_thread, (void *)context) != 0) {
		pr_err("pthread_create() failed.");
		cleanup_websocket(context);
		return -1;
	}

	char command[16];
	while (1) {
		printf("\n[Command] Check balance(c) | Bid(b) | Ask(a) | Quit(q) >>>");
		scanf("%s", command);
		if (strcmp(command, "c") == 0) {
			enqueue_task(get_account_info_task, NULL);
		} else if (strcmp(command, "b") == 0) {
			printf("Bid\n");
		} else if (strcmp(command, "a") == 0) {
			printf("Ask\n");
		} else if (strcmp(command, "q") == 0) {
			printf("Quit\n");
			break;
		} else {
			printf("Wrong command\n");
		}
	}

	/* stop websocket thread */
	ws_running = false;
	pthread_join(ws_thread, NULL);

	/* websocket cleanup */
	cleanup_websocket(context);

	/* libcurl cleanup */
	curl_global_cleanup();

	/* terminate logging */
	terminate_logging();

	/* terminate transaction */
	terminate_txn();

	/* thread queue terminate */
	thread_queue_destroy();

	/* clear extern json format variable*/
	clean_extern_json();

    return 0;
}

void load_env()
{
	FILE *file = fopen(".env", "r");
	if (!file) {
		pr_err("fopen() failed.");
		exit(EXIT_FAILURE);
	}

	char line[512];
	while (fgets(line, sizeof(line), file)) {
		if (line[0] == '#' || line[0] == '\n')
			continue;

		char *key = strtok(line, "=");
		char *value = strtok(NULL, "\n");

		if (key && value) {
			setenv(key, value, 1);
		}
	}
	fclose(file);
}

void *websocket_thread(void *arg)
{
	struct lws_context *context = (struct lws_context *)arg;

	while (ws_running) {
		lws_service(context, 1000);
	}

	return NULL;
}
