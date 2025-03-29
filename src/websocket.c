#include "websocket.h"

static struct lws_context *context = NULL;
static struct lws *wsi = NULL;
static pthread_t ws_thread;
atomic_bool ws_running = ATOMIC_VAR_INIT(true);

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
            pr_out("lws connection closed\n");
            break;

        default:
            break;
    }
    return 0;
}

void *websocket_thread(void *arg)
{
	struct lws_context *context = (struct lws_context *)arg;

	while (atomic_load(&ws_running)) {
		lws_service(context, 1000);
	}

	return NULL;
}

void websocket_thread_run()
{
	if (pthread_create(&ws_thread, NULL, websocket_thread, (void *)context) != 0) {
		pr_err("pthread_create() failed.");
		exit(EXIT_FAILURE);
	}
}

/* Websocket protocol */
static struct lws_protocols protocols[] = {
    { "upbit-protocol", callback_upbit, 0, 0, },
    { NULL, NULL, 0, 0 } /* terminator */
};

void init_socket()
{
    struct lws_context_creation_info info;

    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    // init SSL
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.ssl_ca_filepath = "/etc/ssl/certs/ca-certificates.crt"; // CA path

    context = lws_create_context(&info);
    if (!context) {
		LOG_ERR("lws_create_context() failed");
        lwsl_err("lws_create_context() failed\n");
		exit(EXIT_FAILURE);
    }
}

void connect_websocket()
{
    struct lws_client_connect_info connect_info;

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
		LOG_ERR("lws_client_connect_via_info() failed");
        lwsl_err("lws_client_connect_via_info() failed\n");
		cleanup_websocket();
		exit(EXIT_FAILURE);
    }
}

void cleanup_websocket()
{
	atomic_store(&ws_running, false);
	pthread_join(ws_thread, NULL);

	if (context) {
		lws_context_destroy(context);
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
			LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		free(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			LOG_ERR("lws_write() failed");
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
			LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		free(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			LOG_ERR("lws_write() failed");
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
			LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		free(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			LOG_ERR("lws_write() failed");
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
			LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		free(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, ticker_orderbook_trade, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}
