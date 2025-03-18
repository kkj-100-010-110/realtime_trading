#ifndef _JSON_HANDLER_H
#define _JSON_HANDLER_H

#include "common.h"
#include "order_handler.h"
#include "rest_api.h"
#include "transaction.h"
#include "thread_queue.h"

#include <jansson.h>

#define PRINT 0

extern char *ticker_json;
extern char *orderbook_json;
extern char *ticker_orderbook_json;
extern char *ticker_orderbook_trade;

void clean_extern_json();
void set_json_config();

void parse_websocket_data(const char *data, size_t len);
void parse_ticker_json(json_t *root);
void parse_orderbook_json(json_t *root);
void parse_trade_json(json_t *root);

void parse_balance_json(const char *data);
void parse_order_response_json(const char *data);
void parse_buy_response_json(const char *data);
void parse_sell_response_json(const char *data);

#endif//_JSON_HANDLER_H
