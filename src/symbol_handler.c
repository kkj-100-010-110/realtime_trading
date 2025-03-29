#include "symbol_handler.h"

SymTicker *tickers = NULL;
SymOrderbook *orderbooks = NULL;

const char *codes[SYM_COUNT] = {
	"KRW-XRP",
    "KRW-ADA",
    "KRW-DOGE"
};

const char *symbols[SYM_COUNT] = {
	"XRP",
    "ADA",
    "DOGE"
};

int get_code_index(const char *market)
{
	for (int i = 0; i < SYM_COUNT; i+=1) {
		if (strcmp(market, codes[i]) == 0)
			return i;
	}
	return -1;
}

int get_symbol_index(const char *market)
{
	for (int i = 0; i < SYM_COUNT; i+=1) {
		if (strcmp(market, symbols[i]) == 0)
			return i;
	}
	return -1;
}

void init_sym_ticker_orderbook_info()
{
	MALLOC(tickers, sizeof(SymTicker) * SYM_COUNT);
	MALLOC(orderbooks, sizeof(SymOrderbook) * SYM_COUNT);
}

void destroy_sym_ticker_orderbook_info()
{
	if (tickers)
		free(tickers);
	if (orderbooks)
		free(orderbooks);
}
