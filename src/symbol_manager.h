#ifndef _SYMBOL_MANAGER_H
#define _SYMBOL_MANAGER_H

#include "common.h"

/* Current monitoring events */
#define MAX_SYMBOLS 10
#define MAX_SYMBOL_LEN 10

void load_symbols(char symbols[MAX_SYMBOLS][MAX_SYMBOL_LEN], int *symbol_cnt);
char *create_subscription();

#endif//_SYMBOL_MANAGER_H
