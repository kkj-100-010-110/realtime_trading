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
} Account;

extern Account *account;

int get_index(const char *currency);
void init_account();
void clear_account();

#endif//_ACCOUNT_HANDLER_H
