#ifndef _SYMBOL_HANDLER_H
#define _SYMBOL_HANDLER_H

//#include "common.h"
#include "../include/common.h"

enum {
	SYM_KRW_XRP,
	SYM_KRW_ADA,
	SYM_KRW_DOGE,
	SYM_COUNT
};

extern const char *symbols[SYM_COUNT];

int get_symbol_index(const char *market);

#endif//_SYMBOL_HANDLER_H
