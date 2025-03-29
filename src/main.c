#include "common.h"

#include "websocket.h"
#include "rest_api.h"
#include "log.h"
#include "thread_queue.h"
#include "account_handler.h"
#include "ui.h"

//void clean_up() {
//    ResourceNode* current = g_resources;
//    while (current) {
//        switch (current->type) {
//            case RES_MEM:
//                if (current->resource) {
//                    free(current->resource);
//                    current->resource = NULL;
//                }
//                break;
//            case RES_LOG:
//                terminate_log();
//                break;
//            // ... (다른 타입 처리)
//        }
//        ResourceNode* next = current->prev;
//        free(current);
//        current = next;
//    }
//    g_resources = NULL;
//}
void load_env();

int main(void)
{
	/* create log_file and start logging on it */
	init_log();

	/* setup env */
	load_env();

	/* setup extern variables */
	set_json_config();
	init_order_handler();
	init_sym_ticker_orderbook_info();
	init_account();

	/* libcurl init */
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        pr_err("curl_global_init() failed: %s", curl_easy_strerror(res));
        return -1;
    }

	/* init thread queue */
	thread_queue_init();

	/* init ui */
	//init_ui();

	/* websocket thread */

	/* configure socket and connect */
	init_socket();
	connect_websocket();

	/* Websocket thread run */
	websocket_thread_run();

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
				print_order(orders->root);
			}
		} else if (strcmp(command, "c") == 0) {
		} else if (strcmp(command, "s") == 0) {
		} else if (strcmp(command, "q") == 0) {
			// check if any orders are left

			// cancel all orders.
			CancelOption *co = create_cancel_option("all", NULL, NULL, NULL, 10,
					NULL);
			enqueue_task(cancel_by_bulk_task, (void *)co);
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

	/* websocket thread and context cleanup */
	cleanup_websocket();

	/* libcurl cleanup */
	curl_global_cleanup();

	/* terminate transaction */
	terminate_txn();

	/* thread queue terminate */
	thread_queue_destroy();

	/* clear extern variables */
	clear_extern_json();
	clear_account();
	destroy_order_handler();
	destroy_sym_ticker_orderbook_info();

	/* clear ui */
	//destroy_ui();

	/* terminate logging */
	terminate_log();

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
