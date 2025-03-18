#ifndef _REST_API_H
#define _REST_API_H

#include "common.h"
#include "jwt.h"
#include "json_handler.h"
#include "order_handler.h"

#include <curl/curl.h>	// HTTP/HTTPS request

#define API_URL_ACCOUNT "https://api.upbit.com/v1/accounts"
#define API_URL_ORDERS "https://api.upbit.com/v1/orders"
#define API_URL_ORDER "https://api.upbit.com/v1/order"

#define BUFFER_SIZE 8192

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);

void get_account_info_task(void *arg);
void get_account_info();

void place_order(const char *market, const char *side, double volume, double price);
void place_buy_order(const char *market, double volume, double price);
void place_sell_order(const char *market, double volume, double price);

void cancel_order(const char *uuid);
void market_order(const char *market, const char *side, double amount);

void get_order_status(const char *uuid);

void cancel_by_bulk(const char *cancel_side, const char *pairs,
					const char *excluded_pairs, const char *quote_currencies,
					int count, const char *order_by);

#endif//_REST_API_H
