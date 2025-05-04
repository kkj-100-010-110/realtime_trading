#include "common.h"
#include "websocket.h"
#include "curl_pool.h"
#include "rest_api.h"
#include "thread_queue.h"
#include "account_handler.h"
#include "sig_handler.h"
#include "ui.h"

resource_node_t* g_resources = NULL;
pthread_mutex_t g_resources_mtx = PTHREAD_MUTEX_INITIALIZER;

void load_env();
void set_up();
void clean_up();
void test();

int main(void)
{
	set_up();

	//test();

#if UI_ON
	while (!g_shutdown_flag) {
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

void set_up()
{
	atexit(clean_up);

	setlocale(LC_NUMERIC, "");

	load_env();

	init_log();
	TRACK_RESOURCE(NULL, RES_LOG);

	init_sig_handler();
	TRACK_RESOURCE(NULL, RES_SIG);

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

#if UI_ON
	init_ui();
	TRACK_RESOURCE(NULL, RES_UI);
#endif

	init_websocket_all();
	TRACK_RESOURCE(NULL, RES_WEBSOCKET);
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
			case RES_SIG:
				destroy_sig_handler();
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
				destroy_websocket_all();
				break;
		}
		resource_node_t* next = current->next;
		free(current);
		current = next;
	}
	g_resources = NULL;
	pthread_mutex_unlock(&g_resources_mtx);
}

void test()
{
	char command[2] = {0};
	char uuid[37] = {0};
	int market_idx = -1;
	while (!atomic_load(&g_shutdown_flag)) {
		printf("test1(1) - get account info()\n"
			   "test2(2) - order test(KRW-DOGE, bid, 5000, 0, price)\n"
			   "test3(3) - order test(KRW-XRP, bid, 5000, 0, best, fok)\n"
			   "test4(4) - order test(KRW-XRP, bid, 5000, 0, best, ioc)\n"
			   "test5(5) - print orders\n"
			   "test6(6) - order test(KRW-XRP, bid, 2400, 4, limit, fok)\n"
			   "test7(7) - order test(KRW-XRP, bid, 2500, 4, limit, ioc)\n"
			   "test7(8) - order test(KRW-XRP, bid, 2550, 4, limit)\n"
			   "test9(9) - cancel order\n"
			   "test10(10) - cancel all orders\n"
			   "test11(11) - get order status\n"
			   "test12(12) - get open orders status\n"
			   "test13(13) - get closed orders status\n"
			   "Quit(q)\n"
			   ">>> ");
		scanf("%s", command);
		if (strcmp(command, "1") == 0) {
			enqueue_task(get_account_info_task, NULL);
		} else if (strcmp(command, "2") == 0) {
			order_t *o = make_order("KRW-DOGE", "bid", 5000, 0, "price", NULL,
					NULL);
			enqueue_task(place_order_task, (void *) o);
		} else if (strcmp(command, "3") == 0) {
			order_t *o = make_order("KRW-XRP", "bid", 5000, 0, "best", "fok",
					NULL);
			enqueue_task(place_order_task, (void *) o);
		} else if (strcmp(command, "4") == 0) {
			order_t *o = make_order("KRW-XRP", "bid", 5000, 0, "best", "ioc",
					NULL);
			enqueue_task(place_order_task, (void *) o);
		} else if (strcmp(command, "5") == 0) {
			print_order();
		} else if (strcmp(command, "6") == 0) {
			order_t *o = make_order("KRW-XRP", "bid", 2400, 2, "limit", "fok",
					NULL);
			enqueue_task(place_order_task, (void *) o);
		} else if (strcmp(command, "7") == 0) {
			order_t *o = make_order("KRW-XRP", "bid", 2500, 4, "limit", "ioc",
					NULL);
			enqueue_task(place_order_task, (void *) o);
		} else if (strcmp(command, "8") == 0) {
			order_t *o = make_order("KRW-XRP", "bid", 2550, 4, "limit", NULL,
					NULL);
			enqueue_task(place_order_task, (void *) o);
		} else if (strcmp(command, "9") == 0) {
			printf("uuid >> ");
			scanf("%s", &uuid);
			char *arg;
			MALLOC(arg, sizeof(char) * 37);
			strcpy(arg, uuid);
			enqueue_task(cancel_order_task, (void *)arg);
		} else if (strcmp(command, "10") == 0) {
			// cancel all orders
			cancel_option_t *co = create_cancel_option("all", NULL, NULL, NULL,
					10, NULL);
			enqueue_task(cancel_by_bulk_task, (void *)co);
		} else if (strcmp(command, "11") == 0) {
			printf("uuid >> ");
			scanf("%s", &uuid);
			char *arg;
			MALLOC(arg, sizeof(char) * 37);
			strcpy(arg, uuid);
			enqueue_task(get_order_status_task, (void *)arg);
		} else if (strcmp(command, "12") == 0) {
			order_status_t *os = create_order_status(NULL, 2, 0, 0, 1, 100, NULL);
			enqueue_task(get_open_orders_status_task, (void *)os); 
		} else if (strcmp(command, "13") == 0) {
			order_status_t *os = create_order_status(NULL, 2, 0, 0, 0, 5, NULL);
			enqueue_task(get_closed_orders_status_task, (void *)os); 
		} else if (strcmp(command, "q") == 0) {
			// check if any orders are left
			atomic_store(&g_shutdown_flag, true);
			if (atomic_load(&g_my_orders->is_empty)) {
				printf("Quit, no orders\n");
			} else {
				cancel_by_bulk("all", NULL, NULL, NULL, 10, NULL);
			}
			break;
		} else {
			pr_err("Wrong command\n");
		}
		command[0] = '\0';
		uuid[0] = '\0';
	}
}
