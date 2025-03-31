#include "common.h"
#include "websocket.h"
#include "curl_pool.h"
#include "rest_api.h"
#include "log.h"
#include "thread_queue.h"
#include "account_handler.h"
#include "ui.h"

resource_node_t* g_resources = NULL;
pthread_mutex_t g_resources_mtx = PTHREAD_MUTEX_INITIALIZER;

void load_env();
void clean_up();

int main(void)
{
	atexit(clean_up);
	load_env();

	init_log();
	TRACK_RESOURCE(NULL, RES_LOG);

	init_txn();
	TRACK_RESOURCE(NULL, RES_TXN);

	init_json_config();
	TRACK_RESOURCE(NULL, RES_JSON_CONFIG);

	init_order_handler();
	TRACK_RESOURCE(NULL, RES_ORDER_HANDLER);

	init_sym_ticker_orderbook_info();
	TRACK_RESOURCE(NULL, RES_SYM_INFO);

	init_account();
	TRACK_RESOURCE(NULL, RES_ACCOUNT);

	init_curl_pool();
	TRACK_RESOURCE(NULL, RES_CURL);

	init_thread_queue();
	TRACK_RESOURCE(NULL, RES_THREAD_QUEUE);

	//init_ui();
	//TRACK_RESOURCE(NULL, RES_UI);

	init_websocket();
	TRACK_RESOURCE(NULL, RES_WEBSOCKET);

	char command[10] = {0};
	int market = -1;
	char type[10] = {0};
	char price[20] = {0};
	char volume[20] = {0};
	char ok[10] = {0};
	while (1) {
		printf("Update balance(u)\n"
			   "Make order(m)\n"
			   "Cancel order(c)\n"
			   "Show orders(s)\n"
			   "Quit(q)\n"
			   ">>> ");
		scanf("%s", command);
		if (strcmp(command, "u") == 0) {
			enqueue_task(get_account_info_task, NULL);
		} else if (strcmp(command, "m") == 0) {
			printf("Market 1-XRP, 2-ADA, 3-DOGE >>> ");
			scanf("%d", &market);
			printf("Bid(b)/Ask(a) >>> ");
			scanf("%s", type);
			if (strcmp(type, "a") == 0 || strcmp(type, "b") == 0) {
				strcpy(type, type[0] == 'a' ? "ask" : "bid");
				printf("Price: ");
				scanf("%s", price);
				printf("Volume: ");
				scanf("%s", volume);
				printf("Send? ");
				scanf("%s", ok);
				// order rest api
				//Order *o = make_order(codes[market_idx], order_cmd, price,
				//		volume, NULL);
				//enqueue_task(place_order_task, (void *)o);
				print_order(g_orders->root);
			}
		} else if (strcmp(command, "c") == 0) {
		} else if (strcmp(command, "s") == 0) {
		} else if (strcmp(command, "q") == 0) {
			// check if any orders are left

			// cancel all orders.
			//cancel_option_t *co = create_cancel_option("all", NULL, NULL, NULL, 10,
			//		NULL);
			//enqueue_task(cancel_by_bulk_task, (void *)co);
			// check all done.
			printf("Quit\n");
			break;
		} else {
			printf("Wrong command\n");
		}
		command[0] = '\0';
		type[0] = '\0';
		price[0] = '\0';
		volume[0] = '\0';
		ok[0] = '\0';
	}

#ifndef UI_ON
	while (1) {
		if (update_ui()) {
			break;
		}
		refresh();
		usleep(100000);
	}
#endif//UI_ON

	clean_up();

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

void clean_up()
{
	pthread_mutex_lock(&g_resources_mtx);
	resource_node_t* current = g_resources;
	while (current) {
		switch (current->type) {
			case RES_MEM:
				if (current->resource) {
					free(current->resource);
					current->resource = NULL;
				}
				break;
			case RES_LOG:
				destroy_log();
				break;
			case RES_TXN:
				destroy_txn();
				break;
			case RES_JSON_CONFIG:
				destroy_json_config();
				break;
			case RES_ORDER_HANDLER:
				destroy_order_handler();
				break;
			case RES_SYM_INFO:
				destroy_sym_ticker_orderbook_info();
				break;
			case RES_ACCOUNT:
				destroy_account();
				break;
			case RES_CURL:
				destroy_curl_pool();
				break;
			case RES_THREAD_QUEUE:
				destroy_thread_queue();
				break;
			case RES_UI:
				destroy_ui();
				break;
			case RES_WEBSOCKET:
				destroy_websocket();
				break;
		}
		resource_node_t* next = current->next;
		free(current);
		current = next;
	}
	g_resources = NULL;
	pthread_mutex_unlock(&g_resources_mtx);
}
