#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <signal.h>

void setup_signal_handlers()
{
	signal(sigalrm, signal_handler);
}

void siginal_handler(int signum)
{
}


#endif//_SIGNAL_H
