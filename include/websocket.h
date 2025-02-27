#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include <libwebsockets.h> // lws
#include "common.h"

int init_logging();
struct lws_context *init_socket();
struct lws *connect_websocket(struct lws_context *context);
void cleanup_websocket(struct lws_context *context);
void request_msg(struct lws *wsi);

#endif//_WEBSOCKET_H
