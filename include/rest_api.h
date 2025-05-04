#ifndef _REST_API_H
#define _REST_API_H

#include "common.h"
#include "utils.h"
#include "json_handler.h"
#include "order_handler.h"
#include "curl_pool.h"

#include <curl/curl.h>

/* URL MACRO */
#define API_URL_ACCOUNTS "https://api.upbit.com/v1/accounts"
#define API_URL_ORDERS "https://api.upbit.com/v1/orders"
#define API_URL_ORDER "https://api.upbit.com/v1/order"
#define API_URL_ORDERS_OPEN "https://api.upbit.com/v1/orders/open"
#define API_URL_ORDERS_CLOSED "https://api.upbit.com/v1/orders/closed"

/* BUFFER SIZE */
#define RESPONSE_BUFFER_SIZE 8192
#define HEADER_BUFFER_SIZE 1024
#define POST_BUFFER_SIZE 1024
#define QUERY_BUFFER_SIZE 1024
#define URL_BUFFER_SIZE 2048

/* RETRY */
#define RETRY 3
#define RETRY_DELAY_MS 1000
#define DO_RETRY(res, code) ((res) != CURLE_OK || ((code) >= 500 && (code) <= 599))

/* POST DATA & QUERY STRING MACRO */
// common field
#define JSON_START "{"
#define JSON_END "}"
#define JSON_STR(key, value) "\"" key "\":\"" value "\""

// order type macro
#define PRICE_ORDER_TEMPLATE(identifier, market, ord_type, price, side) \
    JSON_START \
    JSON_STR("identifier", identifier) "," \
    JSON_STR("market", market) "," \
    JSON_STR("ord_type", ord_type) "," \
    JSON_STR("price", price) "," \
    JSON_STR("side", side) \
    JSON_END

#define MARKET_ORDER_TEMPLATE(identifier, market, ord_type, side, volume) \
    JSON_START \
    JSON_STR("identifier", identifier) "," \
    JSON_STR("market", market) "," \
    JSON_STR("ord_type", ord_type) "," \
    JSON_STR("side", side) "," \
    JSON_STR("volume", volume) \
    JSON_END

#define LIMIT_ORDER_NO_OP_TEMPLATE(identifier, market, ord_type, price, side, volume) \
    JSON_START \
    JSON_STR("identifier", identifier) "," \
    JSON_STR("market", market) "," \
    JSON_STR("ord_type", ord_type) "," \
    JSON_STR("price", price) "," \
    JSON_STR("side", side) "," \
    JSON_STR("volume", volume) \
    JSON_END

#define LIMIT_ORDER_TEMPLATE(identifier, market, ord_type, price, side, time_in_force, volume) \
    JSON_START \
    JSON_STR("identifier", identifier) "," \
    JSON_STR("market", market) "," \
    JSON_STR("ord_type", ord_type) "," \
    JSON_STR("price", price) "," \
    JSON_STR("side", side) "," \
    JSON_STR("time_in_force", time_in_force) "," \
    JSON_STR("volume", volume) \
    JSON_END

#define BEST_BID_ORDER_TEMPLATE(identifier, market, ord_type, price, side, time_in_force) \
    JSON_START \
    JSON_STR("identifier", identifier) "," \
    JSON_STR("market", market) "," \
    JSON_STR("ord_type", ord_type) "," \
    JSON_STR("price", price) "," \
    JSON_STR("side", side) "," \
    JSON_STR("time_in_force", time_in_force) \
    JSON_END

#define BEST_ASK_ORDER_TEMPLATE(identifier, market, ord_type, side, time_in_force, volume) \
    JSON_START \
    JSON_STR("identifier", identifier) "," \
    JSON_STR("market", market) "," \
    JSON_STR("ord_type", ord_type) "," \
    JSON_STR("side", side) "," \
    JSON_STR("time_in_force", time_in_force) "," \
    JSON_STR("volume", volume) \
    JSON_END

// common field in alphabetical order
#define Q_IDENTIFIER(id) "identifier=" id
#define Q_MARKET(market) "&market=" market
#define Q_ORD_TYPE(type) "&ord_type=" type
#define Q_PRICE(price) "&price=" price
#define Q_SIDE(side) "&side=" side
#define Q_TIME_IN_FORCE(tif) "&time_in_force=" tif
#define Q_VOLUME(vol) "&volume=" vol

#define QUERY_PRICE_ORDER(id, market, type, price, side) \
    Q_IDENTIFIER(id) \
	Q_MARKET(market) \
	Q_ORD_TYPE(type) \
	Q_PRICE(price) \
	Q_SIDE(side)

#define QUERY_MARKET_ORDER(id, market, type, side, vol) \
    Q_IDENTIFIER(id) \
	Q_MARKET(market) \
	Q_ORD_TYPE(type) \
	Q_SIDE(side) \
	Q_VOLUME(vol)

#define QUERY_LIMIT_NO_OP_ORDER(id, market, type, price, side, vol) \
    Q_IDENTIFIER(id) \
	Q_MARKET(market) \
	Q_ORD_TYPE(type) \
	Q_PRICE(price) \
	Q_SIDE(side) \
	Q_VOLUME(vol)

#define QUERY_LIMIT_ORDER(id, market, type, price, side, tif, vol) \
    Q_IDENTIFIER(id) \
	Q_MARKET(market) \
	Q_ORD_TYPE(type) \
	Q_PRICE(price) \
	Q_SIDE(side) \
	Q_TIME_IN_FORCE(tif) \
	Q_VOLUME(vol)

#define QUERY_BEST_BID_ORDER(id, market, type, price, side, tif) \
    Q_IDENTIFIER(id) \
	Q_MARKET(market) \
	Q_ORD_TYPE(type) \
	Q_PRICE(price) \
	Q_SIDE(side) \
	Q_TIME_IN_FORCE(tif)

#define QUERY_BEST_ASK_ORDER(id, market, type, side, tif, vol) \
    Q_IDENTIFIER(id) \
	Q_MARKET(market) \
	Q_ORD_TYPE(type) \
	Q_SIDE(side) \
	Q_TIME_IN_FORCE(tif) \
	Q_VOLUME(vol)

/* CURL WRITE CALLBACK FUNCTION */
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);

/* TASK FUNCTION FORMAT FOR THREAD QUEUE */
void get_account_info_task(void *arg);
void place_order_task(void *arg);
void get_order_status_task(void *arg);
void get_open_orders_status_task(void *arg);
void get_closed_orders_status_task(void *arg);
void cancel_order_task(void *arg);
void cancel_by_bulk_task(void *arg);

/* REQUEST FUNCTIONS */
void get_account_info();
void place_order(const char *market, const char *side, double volume,
		double price, const char *ord_type, const char *time_in_force);
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

/* RETRY FUNCTION */
int check_order_status(const char *uuid);

#endif//_REST_API_H
