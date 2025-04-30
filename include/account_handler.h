#ifndef _ACCOUNT_HANDLER_H
#define _ACCOUNT_HANDLER_H

#include "common.h"

enum {
	KRW,
	XRP,
	ADA,
	DOGE,
	COUNT
};

typedef struct {
	char currency[10];
	double balance;
	double locked;
	pthread_mutex_t lock;
} account_t;

extern account_t *g_account;

void init_account();
void destroy_account();
int get_index(const char *currency);
void update_account(const char *currency, double balance, double locked);

#endif//_ACCOUNT_HANDLER_H
