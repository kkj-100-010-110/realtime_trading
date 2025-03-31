#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "common.h"
#include "json_handler.h"

#include <libwebsockets.h> // lws

extern atomic_bool g_ws_running;

/* WEBSOCKET */
void init_websocket();
void set_socket();
void connect_websocket();
void destroy_websocket();
void *websocket_thread(void *arg);
void websocket_thread_run();

/* REQUEST */
void ticker_request(struct lws *wsi);
void orderbook_request(struct lws *wsi);
void ticker_orderbook_request(struct lws *wsi);
void ticker_orderbook_trade_request(struct lws *wsi);

#endif//_WEBSOCKET_H
