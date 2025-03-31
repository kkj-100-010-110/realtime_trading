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
} account_t;

extern account_t *g_account;

int get_index(const char *currency);
void init_account();
void destroy_account();

#endif//_ACCOUNT_HANDLER_H
