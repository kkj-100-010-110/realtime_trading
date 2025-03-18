#include "websocket.h"

/* Websocket callback function */
static int callback_upbit(struct lws *wsi,
						  enum lws_callback_reasons reason,
						  void *user,
						  void *in,
						  size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            // response from UPbit
			parse_websocket_data((char *)in, len);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            // request to UPbit
			ticker_orderbook_trade_request(wsi);
            break;
        }

        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("Connection closed\n");
            break;

        default:
            break;
    }
    return 0;
}

/* Websocket protocol */
static struct lws_protocols protocols[] = {
    { "upbit-protocol", callback_upbit, 0, 0, },
    { NULL, NULL, 0, 0 } /* terminator */
};

struct lws_context *init_socket()
{
    struct lws_context_creation_info info;
    struct lws_context *context;

    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    // init SSL
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.ssl_ca_filepath = "/etc/ssl/certs/ca-certificates.crt"; // CA path

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws_create_context() failed\n");
        return NULL;
    }
	return context;
}

struct lws *connect_websocket(struct lws_context *context)
{
    struct lws_client_connect_info connect_info;
    struct lws *wsi;

    memset(&connect_info, 0, sizeof(connect_info));
    connect_info.context = context;
    connect_info.address = "api.upbit.com";
    connect_info.port = 443;
    connect_info.path = "/websocket/v1";
    connect_info.host = "api.upbit.com";
    connect_info.origin = "api.upbit.com";
    connect_info.protocol = protocols[0].name;
    connect_info.ssl_connection = LCCSCF_USE_SSL;

    wsi = lws_client_connect_via_info(&connect_info);
    if (!wsi) {
        lwsl_err("lws_client_connect_via_info() failed\n");
        return NULL;
    }
	return wsi;
}

void cleanup_websocket(struct lws_context *context)
{
	if (context) {
		lws_context_destroy(context);
		clean_extern_json();
	}
}

void ticker_request(struct lws *wsi)
{
	size_t len = strlen(ticker_json);
	if (len > 1024) {
		unsigned char *buf;
		MALLOC(buf, sizeof(unsigned char) * (LWS_PRE + len + 1));
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		free(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}

void orderbook_request(struct lws *wsi)
{
	size_t len = strlen(orderbook_json);
	if (len > 1024) {
		unsigned char *buf;
		MALLOC(buf, sizeof(unsigned char) * (LWS_PRE + len + 1));
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
			free(buf);
		}
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}

void ticker_orderbook_request(struct lws *wsi)
{
	size_t len = strlen(ticker_orderbook_json);
	if (len > 1024) {
		unsigned char *buf;
		MALLOC(buf, sizeof(unsigned char) * (LWS_PRE + len + 1));
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}

void ticker_orderbook_trade_request(struct lws *wsi)
{
	size_t len = strlen(ticker_orderbook_trade);
	if (len > 1024) {
		unsigned char *buf;
		MALLOC(buf, sizeof(unsigned char) * (LWS_PRE + len + 1));
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_orderbook_trade, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_orderbook_trade, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}
