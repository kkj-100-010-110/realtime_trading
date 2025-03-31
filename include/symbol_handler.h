#ifndef _SYMBOL_HANDLER_H
#define _SYMBOL_HANDLER_H

#include "common.h"

/*
 * Ticker and orderbook data for symbols
 */

enum {
	SYM_KRW_XRP,
	SYM_KRW_ADA,
	SYM_KRW_DOGE,
	SYM_COUNT
};

typedef struct {
	const char *code;
	double trade_price;
	char change[5];
	double signed_change_price;
	double signed_change_rate;
	double high_price;
	double low_price;
	double opening_price;
	double prev_closing_price;
	double trade_volume;
	double acc_trade_volume_24h;
	double acc_trade_price_24h;
	double highest_52_week_price;
	double lowest_52_week_price;
	char market_state[9];
} sym_ticker_t;

typedef struct {
	const char *code;
	int timestamp;
	double total_bid_size;
	double total_ask_size;
	double best_bid_price;
	double best_bid_size;
	double best_ask_price;
	double best_ask_size;
	double spread;
	double bid_ask_ratio;
} sym_orderbook_t;

extern sym_ticker_t *g_tickers;
extern sym_orderbook_t *g_orderbooks;
extern const char *g_codes[SYM_COUNT];
extern const char *g_symbols[SYM_COUNT];

int get_code_index(const char *market);
int get_symbol_index(const char *market);
void init_sym_ticker_orderbook_info();
void destroy_sym_ticker_orderbook_info();

#endif//_SYMBOL_HANDLER_H
