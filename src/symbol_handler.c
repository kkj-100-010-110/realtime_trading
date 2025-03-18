#include "symbol_handler.h"

const char *symbols[SYM_COUNT] = {
	"KRW-XRP",
    "KRW-ADA",
    "KRW-DOGE"
};

int get_symbol_index(const char *market)
{
	for (int i = 0; i < SYM_COUNT; i+=1) {
		if (strcmp(market, symbols[i]) == 0)
			return i;
	}
	return -1;
}
