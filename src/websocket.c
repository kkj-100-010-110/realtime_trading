#include "websocket.h"
#include "json_handler.h"
#include "symbol_manager.h"

/* LOG UTILS */
static FILE *log_file = NULL;

static void log_emit(int level, const char *msg)
{
	if (log_file) {
		fprintf(log_file, "%s", msg);
		fflush(log_file);
	} else {
		fprintf(stderr, "%s", msg);
	}
}

int init_logging()
{
	log_file = fopen("./log/log_file.txt", "w");
	if (!log_file) {
		pr_err("Failed to open log_file");
		return -1;
	}
	lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE, log_emit);
	return 0;
}
/* LOG UTILS */


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
			parse_json((char *)in);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            // request to UPbit
            request_msg(wsi);
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
	}
}

void request_msg(struct lws *wsi)
{
	char *msg = create_subscription();
	size_t len = strlen(msg);
	unsigned char *tmp = (unsigned char *)malloc(LWS_PRE + len);
	if (!tmp) {
		pr_err("malloc failed.");
		return;
	}
	unsigned char *start = tmp + LWS_PRE;
	memcpy(start, msg, len);
	if (lws_write(wsi, start, len, LWS_WRITE_TEXT) < 0) {
		lwsl_err("lws_write() failed.\n");
	}
	free(msg);
	free(tmp);
}
