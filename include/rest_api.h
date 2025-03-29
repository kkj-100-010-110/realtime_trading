#ifndef _REST_API_H
#define _REST_API_H

#include "common.h"
#include "jwt.h"
#include "json_handler.h"
#include "order_handler.h"

#include <curl/curl.h>

/* URL macro */
#define API_URL_ACCOUNTS "https://api.upbit.com/v1/accounts"
#define API_URL_ORDERS "https://api.upbit.com/v1/orders"
#define API_URL_ORDER "https://api.upbit.com/v1/order"
#define API_URL_ORDERS_OPEN "https://api.upbit.com/v1/orders/open"
#define API_URL_ORDERS_CLOSED "https://api.upbit.com/v1/orders/closed"

/* buffer size */
#define RESPONSE_BUFFER_SIZE 8192
#define HEADER_BUFFER_SIZE 1024
#define POST_BUFFER_SIZE 1024
#define URL_BUFFER_SIZE 2048

/* curl write callback function */
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);

/* task function format for thread queue */
void get_account_info_task(void *arg);
void place_order_task(void *arg);
void market_price_order_task(void *arg);
void get_order_status_task(void *arg);
void get_closed_orders_status_task(void *arg);
void cancel_order_task(void *arg);
void cancel_by_bulk_task(void *arg);

/* functions */
void get_account_info();
void place_order(const char *market, const char *side, double volume, double price);
void place_buy_order(const char *market, double volume, double price);
void place_sell_order(const char *market, double volume, double price);
void market_order(const char *market, const char *side, double amount);
void cancel_order(const char *uuid);
void get_order_status(const char *uuid);
void get_open_orders_status(const char *market, const char **states,
		size_t states_count, int page, int limit, const char *order_by);
void get_closed_orders_status(const char *market, const char **states,
		size_t states_count, long start_time, long end_time,
		int limit, const char *order_by);
void cancel_by_bulk(const char *cancel_side, const char *pairs,
					const char *excluded_pairs, const char *quote_currencies,
					int count, const char *order_by);

/* retry function */
int check_order_status(const char *uuid);

#endif//_REST_API_H
