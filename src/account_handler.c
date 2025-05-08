#include "account_handler.h"

static pthread_mutex_t account_mtx = PTHREAD_MUTEX_INITIALIZER; 

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
	memset(g_account, 0, sizeof(g_account));
	for (int i = 0; i < COUNT; i++) {
		SAFE_STRCPY(g_account[i].currency, currencies[i]);
		if (pthread_mutex_init(&g_account[i].lock, NULL) != 0) {
			pr_err("pthread_mutex_init() failed.");
			exit(EXIT_FAILURE);
		}
	}
}

void destroy_account()
{
	pthread_mutex_lock(&account_mtx);
	for (int i = 0; i < COUNT; i++) {
		pthread_mutex_destroy(&g_account[i].lock);
	}
	if (g_account) {
		free(g_account);
	}
	pthread_mutex_unlock(&account_mtx);
	pthread_mutex_destroy(&account_mtx);
}

void update_account(const char *currency, double balance, double locked)
{
	pthread_mutex_lock(&account_mtx);

	int idx;
	idx = get_index(currency);
	if (idx == -1) {
		pr_err("no matching currency.");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock(&account_mtx);

	pthread_mutex_lock(&g_account[idx].lock);

	g_account[idx].balance = balance;
	g_account[idx].locked = locked;

	pthread_mutex_unlock(&g_account[idx].lock);
}
