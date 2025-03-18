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
	char *currency;
	double balance;
	double locked;
} Account;

extern Account *account;

void init_account();
void terminate_account();

#endif//_ACCOUNT_HANDLER_H
