#include "common.h"
#include "websocket.h"

int main(void)
{
	/* create log_file and start logging on it */
	if (init_logging() != 0) {
		pr_err("init_logging() failed.");
		return -1;
	}

	/* configure socket and connect */
	struct lws_context *context = init_socket();
	struct lws *wsi = connect_websocket(context);
	if (!wsi) {
		lwsl_err("connect_websocket() failed.\n");
		cleanup_websocket(context);
		return -1;
	}

	/* run service */
    while (1) {
        lws_service(context, 1000);
    }

	/* terminate */
	cleanup_websocket(context);

    return 0;
}
