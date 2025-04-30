#ifndef _UI_H
#define _UI_H

#include "common.h"
#include "symbol_handler.h"

#include <ncurses.h>

#define RISE_COLOR 1
#define FALL_COLOR 2
#define BID_COLOR 3
#define ASK_COLOR 4
#define HIGHLIGHT_COLOR 5
#define DEFAULT_COLOR 6

void destroy_ui();
void init_ui();
int update_ui();

#endif//_UI_H
