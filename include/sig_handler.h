#ifndef _SIG_HANDLER_H
#define _SIG_HANDLER_H

#include <signal.h>
#include <syslog.h>

extern volatile sig_atomic_t g_shutdown_flag;

void sig_handler(int signum);
void init_sig_handler();
void destroy_sig_handler();

#endif//_SIG_HANDLER_H
