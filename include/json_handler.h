#ifndef _JSON_HANDLER_H
#define _JSON_HANDLER_H

#include "common.h"
#include "order_handler.h"
#include "rest_api.h"
#include "transaction.h"
#include "thread_queue.h"
#include "account_handler.h"
#include "symbol_handler.h"
#include "websocket.h"

#include <jansson.h>

/* MACRO */
#define MIN_VOLUME_THRESHOLD 1e-8 // remaining volume check
#define JSON_DOUBLE(obj, key) \
	(json_is_real(json_object_get(obj, key)) ? \
	 json_real_value(json_object_get(obj, key)) : \
	 (double)json_integer_value(json_object_get(obj, key)))

/* EXTERN VARIABLE */
extern char *g_ticker_json;
extern char *g_orderbook_json;
extern char *g_ticker_orderbook_json;
extern char *g_ticker_orderbook_trade;

void init_json_config();
void destroy_json_config();

/* WEBSOCKET RESPONSE JSON DATA PARSE */
void parse_websocket_public_data(const char *data, size_t len);
void parse_websocket_private_data(const char *data, size_t len);
void parse_ticker_json(json_t *root);
void parse_orderbook_json(json_t *root);
//void parse_trade_json(json_t *root);
void parse_my_order_json(json_t *root);
void parse_my_asset_json(json_t *root);

/* REST API RESPONSE JSON DATA PARSE */
void parse_account_json(const char *data);
void parse_place_order_response(const char *data);
void parse_cancel_order_response_json(const char *data);
void parse_get_order_status(const char *data);
void parse_get_open_orders_status(const char * data);
void parse_get_closed_orders_status(const char *data);
void parse_cancel_by_bulk_response(const char *data);

#endif//_JSON_HANDLER_H
