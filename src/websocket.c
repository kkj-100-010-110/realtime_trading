#include "websocket.h"

// websocket public resources
static struct lws_context *public_context = NULL;
static struct lws *public_wsi = NULL;
static pthread_t ws_public_thread;
static atomic_bool ws_public_running = ATOMIC_VAR_INIT(true);

// websocket private resources
static struct lws_context *private_context = NULL;
static struct lws *private_wsi = NULL;
static pthread_t ws_private_thread;
static atomic_bool ws_private_running = ATOMIC_VAR_INIT(true);
static time_t last_ping_time = 0;
static char jwt_token[JWT_BUFFER_SIZE];

static bool order_subscribed = false;
static bool asset_subscribed = false;
static bool order_asset_subscribed = false;

static int callback_upbit_public(struct lws *wsi,
								 enum lws_callback_reasons reason,
								 void *user, void *in, size_t len);
static int callback_upbit_private(struct lws *wsi,
								  enum lws_callback_reasons reason,
								  void *user, void *in, size_t len);
static void setup_jwt_token();
static void set_socket_public();
static void connect_websocket_public();
static void set_socket_private();
static void connect_websocket_private();
static void *websocket_public_thread(void *arg);
static void websocket_public_thread_run();
static void *websocket_private_thread(void *arg);
static void websocket_private_thread_run();

/* REQUEST */
static void ticker_request(struct lws *wsi);
static void orderbook_request(struct lws *wsi);
static void ticker_orderbook_request(struct lws *wsi);
static void ticker_orderbook_trade_request(struct lws *wsi);
static void my_order_request(struct lws *wsi);
static void my_asset_request(struct lws *wsi);
static void my_order_asset_request(struct lws *wsi);

/* WEBSOCKET PROTOCOL */
static struct lws_protocols public_protocols[] = {
    { "upbit-protocol-public", callback_upbit_public, 0, 0, },
    { NULL, NULL, 0, 0 } /* terminator */
};

static struct lws_protocols private_protocols[] = {
    { "upbit-protocol-private", callback_upbit_private, 0, 0, },
    { NULL, NULL, 0, 0 } /* terminator */
};

static int callback_upbit_public(struct lws *wsi,
								 enum lws_callback_reasons reason,
								 void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            // response from UPbit
			parse_websocket_public_data((char *)in, len);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            // request to UPbit
			ticker_orderbook_request(wsi);
			// trade will be used later
			//ticker_orderbook_trade_request(wsi);
            break;
        }

		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        case LWS_CALLBACK_CLIENT_CLOSED:
			atomic_store(&g_shutdown_flag, true);
			atomic_store(&ws_public_running, false);
            pr_out("PUBLIC LWS CONNECTION CLOSED.");
            break;

        default:
            break;
    }
    return 0;
}

static int callback_upbit_private(struct lws *wsi,
								  enum lws_callback_reasons reason,
								  void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
			last_ping_time = time(NULL);
            lws_callback_on_writable(wsi);
            break;

		case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
			{
				unsigned char **p = (unsigned char **)in;
				unsigned char *end = *p + 1024;

				char jwt_header[JWT_BUFFER_SIZE];
				snprintf(jwt_header, sizeof(jwt_header), "Bearer %s", jwt_token);

				if (lws_add_http_header_by_name(wsi,
							(const unsigned char *)"authorization",
							(const unsigned char *)jwt_header,
							strlen(jwt_header), p, end)) {
					MY_LOG_ERR("Failed to add Authorization header");
					lwsl_err("Failed to add Authorization header\n");
					return -1;
				}
				break;
			}

        case LWS_CALLBACK_CLIENT_RECEIVE:
            // response from UPbit
			parse_websocket_private_data((char *)in, len);
            break;

		case LWS_CALLBACK_CLIENT_WRITEABLE:
			time_t now = time(NULL);
			if (now - last_ping_time >= PING_INTERVAL) {
				unsigned char ping_buf[LWS_PRE + 1];
				memset(ping_buf, 0, sizeof(ping_buf));
				if (lws_write(wsi, &ping_buf[LWS_PRE], 0, LWS_WRITE_PING) < 0) {
					pr_out("PING SEND FAILED");
				} else {
					pr_out("PING SENT");
					last_ping_time = now;
				}
			}

			if (!order_asset_subscribed) {
				my_order_asset_request(wsi);
				order_asset_subscribed = true;
			}

			lws_callback_on_writable(wsi);
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
			pr_out("PONG");
			break;

		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        case LWS_CALLBACK_CLIENT_CLOSED:
			atomic_store(&g_shutdown_flag, true);
			atomic_store(&ws_private_running, false);
            pr_out("PRIVATE LWS CONNECTION CLOSED.");
            break;

        default:
            break;
    }
    return 0;
}

static void setup_jwt_token()
{
	generate_jwt(jwt_token, sizeof(jwt_token));
}

static void *websocket_public_thread(void *arg)
{
	struct lws_context *context = (struct lws_context *)arg;

	while (atomic_load(&ws_public_running)) {
		lws_service(context, 1000);
	}

	return NULL;
}

static void websocket_public_thread_run()
{
	if (pthread_create(&ws_public_thread, NULL, websocket_public_thread,
				(void *)public_context) != 0) {
		pr_err("pthread_create() failed.");
		exit(EXIT_FAILURE);
	}
}

static void *websocket_private_thread(void *arg)
{
	struct lws_context *context = (struct lws_context *)arg;

	while (atomic_load(&ws_private_running)) {
		lws_service(context, 1000);
	}

	return NULL;
}

static void websocket_private_thread_run()
{
	if (pthread_create(&ws_private_thread, NULL, websocket_private_thread,
				(void *)private_context) != 0) {
		pr_err("pthread_create() failed.");
		exit(EXIT_FAILURE);
	}
}

static void set_socket_public()
{
    struct lws_context_creation_info info;

    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = public_protocols;

    // init SSL
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.ssl_ca_filepath = "/etc/ssl/certs/ca-certificates.crt"; // CA path

    public_context = lws_create_context(&info);
    if (!public_context) {
		MY_LOG_ERR("lws_create_context() failed");
        lwsl_err("lws_create_context() failed\n");
		exit(EXIT_FAILURE);
    }
}

static void connect_websocket_public()
{
    struct lws_client_connect_info connect_info;

    memset(&connect_info, 0, sizeof(connect_info));
    connect_info.context = public_context;
    connect_info.address = "api.upbit.com";
    connect_info.port = 443;
    connect_info.path = "/websocket/v1";
    connect_info.host = "api.upbit.com";
    connect_info.origin = "api.upbit.com";
    connect_info.protocol = public_protocols[0].name;
    connect_info.ssl_connection = LCCSCF_USE_SSL;

    public_wsi = lws_client_connect_via_info(&connect_info);
    if (!public_wsi) {
		MY_LOG_ERR("lws_client_connect_via_info() failed");
        lwsl_err("lws_client_connect_via_info() failed\n");
		destroy_websocket_public();
		exit(EXIT_FAILURE);
    }
}

static void set_socket_private()
{
    struct lws_context_creation_info info;

    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = private_protocols;

    // init SSL
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.ssl_ca_filepath = "/etc/ssl/certs/ca-certificates.crt"; // CA path

    private_context = lws_create_context(&info);
    if (!private_context) {
		MY_LOG_ERR("lws_create_context() failed");
        lwsl_err("lws_create_context() failed\n");
		exit(EXIT_FAILURE);
    }
}

static void connect_websocket_private()
{
	struct lws_client_connect_info connect_info;

	memset(&connect_info, 0, sizeof(connect_info));
	connect_info.context = private_context;
	connect_info.address = "api.upbit.com";
	connect_info.port = 443;
	connect_info.path = "/websocket/v1/private";
	connect_info.host = "api.upbit.com";
	connect_info.origin = "api.upbit.com";
	connect_info.protocol = private_protocols[0].name;
	connect_info.ssl_connection = LCCSCF_USE_SSL;

	private_wsi = lws_client_connect_via_info(&connect_info);
	if (!private_wsi) {
		MY_LOG_ERR("lws_client_connect_via_info() failed");
		lwsl_err("lws_client_connect_via_info() failed\n");
		destroy_websocket_private();
		exit(EXIT_FAILURE);
	}
}

void init_websocket_all()
{
	init_websocket_public();
	init_websocket_private();
}

void destroy_websocket_all()
{
	destroy_websocket_public();
	destroy_websocket_private();
}

void init_websocket_public()
{
	set_socket_public();
	connect_websocket_public();
	websocket_public_thread_run();
}

void destroy_websocket_public()
{
	atomic_store(&ws_public_running, false);

	if (public_context)
		lws_cancel_service(public_context);

	pthread_join(ws_public_thread, NULL);

	if (public_context) {
		lws_context_destroy(public_context);
		public_context = NULL;
	}
}

void init_websocket_private()
{
	setup_jwt_token();
	set_socket_private();
	connect_websocket_private();
	websocket_private_thread_run();
}

void destroy_websocket_private()
{
	atomic_store(&ws_private_running, false);

	if (private_context)
		lws_cancel_service(private_context);

	pthread_join(ws_private_thread, NULL);

	if (private_context) {
		lws_context_destroy(private_context);
		private_context = NULL;
	}
}

static void ticker_request(struct lws *wsi)
{
	size_t len = strlen(g_ticker_json);
	if (len > 1024) {
		unsigned char *buf;
		MALLOC(buf, sizeof(unsigned char) * (LWS_PRE + len + 1));
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, g_ticker_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			MY_LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		FREE(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, g_ticker_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			MY_LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}

static void orderbook_request(struct lws *wsi)
{
	size_t len = strlen(g_orderbook_json);
	if (len > 1024) {
		unsigned char *buf;
		MALLOC(buf, sizeof(unsigned char) * (LWS_PRE + len + 1));
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, g_orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			MY_LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		FREE(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, g_orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			MY_LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}

static void ticker_orderbook_request(struct lws *wsi)
{
	size_t len = strlen(g_ticker_orderbook_json);
	if (len > 1024) {
		unsigned char *buf;
		MALLOC(buf, sizeof(unsigned char) * (LWS_PRE + len + 1));
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, g_ticker_orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			MY_LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		FREE(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, g_ticker_orderbook_json, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			MY_LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}

static void ticker_orderbook_trade_request(struct lws *wsi)
{
	size_t len = strlen(g_ticker_orderbook_trade);
	if (len > 1024) {
		unsigned char *buf;
		MALLOC(buf, sizeof(unsigned char) * (LWS_PRE + len + 1));
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, g_ticker_orderbook_trade, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			MY_LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
		FREE(buf);
	} else {
		unsigned char buf[LWS_PRE + 1024];
		unsigned char *msg = &buf[LWS_PRE];
		memcpy(msg, g_ticker_orderbook_trade, len);
		if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
			MY_LOG_ERR("lws_write() failed");
			lwsl_err("lws_write() failed.\n");
			pr_err("lws_write() failed.");
		}
	}
}

static void my_order_request(struct lws *wsi)
{
	const char *my_order_json =
		"["
		"{\"ticket\":\"my_order_sub\"},"
		"{\"type\":\"myOrder\"},"
		"{\"format\":\"DEFAULT\"}"
		"]";

	size_t len = strlen(my_order_json);
	unsigned char buf[LWS_PRE + 1024];
	unsigned char *msg = &buf[LWS_PRE];
	memcpy(msg, my_order_json, len);
	if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
		MY_LOG_ERR("lws_write() failed");
		lwsl_err("lws_write() failed.\n");
		pr_err("lws_write() failed.");
	}
}

static void my_asset_request(struct lws *wsi)
{
    const char *my_asset_json =
        "["
        "{\"ticket\":\"my_asset_sub\"},"
        "{\"type\":\"myAsset\"},"
        "{\"format\":\"DEFAULT\"}"
        "]";

	size_t len = strlen(my_asset_json);
	unsigned char buf[LWS_PRE + 1024];
	unsigned char *msg = &buf[LWS_PRE];
	memcpy(msg, my_asset_json, len);
	if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
		MY_LOG_ERR("lws_write() failed");
		lwsl_err("lws_write() failed.\n");
		pr_err("lws_write() failed.");
	}
}

static void my_order_asset_request(struct lws *wsi)
{
	const char *my_order_asset_json =
		"["
		"{\"ticket\":\"my_order_asset_sub\"},"
		"{\"type\":\"myOrder\"},"
        "{\"type\":\"myAsset\"},"
		"{\"format\":\"DEFAULT\"}"
		"]";

	size_t len = strlen(my_order_asset_json);
	unsigned char buf[LWS_PRE + 1024];
	unsigned char *msg = &buf[LWS_PRE];
	memcpy(msg, my_order_asset_json, len);
	if (lws_write(wsi, msg, len, LWS_WRITE_TEXT) < 0) {
		MY_LOG_ERR("lws_write() failed");
		lwsl_err("lws_write() failed.\n");
		pr_err("lws_write() failed.");
	}
}
