#include "account_handler.h"

Account *account = NULL;

void init_account()
{
	MALLOC(account, sizeof(account) * Count);
}

void terminate_account()
{
	if (account)
		free(account);
}
