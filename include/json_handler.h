#ifndef _JSON_HANDLER_H
#define _JSON_HANDLER_H

#include "common.h"
#include "order_handler.h"
#include "rest_api.h"
#include "transaction.h"
#include "thread_queue.h"
#include "account_handler.h"
#include "symbol_handler.h"

#include <jansson.h>

/* EXTERN VARIABLE */
extern char *g_ticker_json;
extern char *g_orderbook_json;
extern char *g_ticker_orderbook_json;
extern char *g_ticker_orderbook_trade;

void init_json_config();
void destroy_json_config();

/* WEBSOCKET RESPONSE JSON DATA PARSE */
void parse_websocket_data(const char *data, size_t len);
void parse_ticker_json(json_t *root);
void parse_orderbook_json(json_t *root);
void parse_trade_json(json_t *root);

/* REST API RESPONSE JSON DATA PARSE */
void parse_account_json(const char *data);
void parse_place_order_response(const char *data);
void parse_cancel_order_response_json(const char *data);
void parse_get_order_status(const char *data);
void parse_get_open_orders_status(const char * data);
void parse_get_closed_orders_status(const char *data);
void parse_cancel_by_bulk_response(const char *data);

#endif//_JSON_HANDLER_H
