#include "account_handler.h"

Account *account = NULL;

static const char *currencies[COUNT] = {
	"KRW",
	"XRP",
    "ADA",
    "DOGE"
};

int get_index(const char *currency)
{
	for (int i = 0; i < COUNT; i+=1) {
		if (strcmp(currency, currencies[i]) == 0)
			return i;
	}
	return -1;
}

void init_account()
{
	MALLOC(account, sizeof(Account) * COUNT);
}

void clear_account()
{
	if (account) {
		free(account);
	}
}
