#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "common.h"
#include "json_handler.h"

#include <libwebsockets.h> // lws

/* WEBSOCKET */
struct lws_context *init_socket();
struct lws *connect_websocket(struct lws_context *context);
void cleanup_websocket(struct lws_context *context);
/* REQUEST */
void ticker_request(struct lws *wsi);
void orderbook_request(struct lws *wsi);
void ticker_orderbook_request(struct lws *wsi);
void ticker_orderbook_trade_request(struct lws *wsi);

#endif//_WEBSOCKET_H
