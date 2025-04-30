#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "common.h"
#include "json_handler.h"
#include "utils.h"
#include "sig_handler.h"

#include <libwebsockets.h> // lws

#define PING_INTERVAL 30
#define DATA_REQUEST_INTERVAL 5
#define MAX_RETRY_ATTEMPTS 5

/* WEBSOCKET */
void init_websocket_all();
void destroy_websocket_all();
void init_websocket_public();
void destroy_websocket_public();
void init_websocket_private();
void destroy_websocket_private();

#endif//_WEBSOCKET_H
