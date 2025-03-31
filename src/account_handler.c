#include "account_handler.h"

account_t *g_account = NULL;

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
	g_account = (account_t *)malloc(sizeof(account_t) * COUNT);
	if (!g_account) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}
}

void destroy_account()
{
	if (g_account) {
		free(g_account);
	}
}
