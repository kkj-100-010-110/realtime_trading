#include "sig_handler.h"

volatile sig_atomic_t g_shutdown_flag = 0;

void sig_handler(int signum)
{
	const char *sig_name = NULL;
	switch (signum) {
		case SIGINT:  sig_name = "SIGINT"; break;
		case SIGTERM: sig_name = "SIGTERM"; break;
		case SIGHUP:  sig_name = "SIGHUP"; break;
		default:      sig_name = "UNKNOWN"; break;
	}
	syslog(LOG_ERR, "[Signal] %s received. Shutdown...", sig_name);
	g_shutdown_flag = 1;
}

void init_sig_handler()
{
	openlog("realtime_trading", LOG_PID | LOG_NDELAY, LOG_DAEMON);

	struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);
}

void destroy_sig_handler()
{
	closelog();
}
