#include "symbol_handler.h"

sym_ticker_t *g_tickers = NULL;
sym_orderbook_t *g_orderbooks = NULL;

const char *g_codes[SYM_COUNT] = {
	"KRW-XRP",
    "KRW-ADA",
    "KRW-DOGE"
};

const char *g_symbols[SYM_COUNT] = {
	"XRP",
    "ADA",
    "DOGE"
};

int get_code_index(const char *market)
{
	for (int i = 0; i < SYM_COUNT; i+=1) {
		if (strcmp(market, g_codes[i]) == 0)
			return i;
	}
	return -1;
}

int get_symbol_index(const char *market)
{
	for (int i = 0; i < SYM_COUNT; i+=1) {
		if (strcmp(market, g_symbols[i]) == 0)
			return i;
	}
	return -1;
}

void init_sym_ticker_orderbook_info()
{
	g_tickers = (sym_ticker_t *)malloc(sizeof(sym_ticker_t) * SYM_COUNT);
	if (!g_tickers) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}
	g_orderbooks =(sym_orderbook_t *)malloc(sizeof(sym_orderbook_t) * SYM_COUNT);
	if (!g_orderbooks) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}
}

void destroy_sym_ticker_orderbook_info()
{
	if (g_tickers)
		free(g_tickers);
	if (g_orderbooks)
		free(g_orderbooks);
}
