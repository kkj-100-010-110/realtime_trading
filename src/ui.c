#include "ui.h"

static int get_user_input(char *buffer, int max_len);
static void ticker_orderbook_ui();
static void balance_ui();
static int menu_ui();
static void order_ui();

void destroy_ui()
{
	endwin();
	reset_shell_mode();
}

void init_ui()
{
	initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
	start_color();
	init_pair(RISE_COLOR, COLOR_RED, COLOR_BLACK);
	init_pair(FALL_COLOR, COLOR_BLUE, COLOR_BLACK);
	init_pair(BID_COLOR, COLOR_CYAN, COLOR_BLACK);
	init_pair(ASK_COLOR, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(HIGHLIGHT_COLOR, COLOR_YELLOW, COLOR_BLACK);
	init_pair(DEFAULT_COLOR, COLOR_WHITE, COLOR_BLACK);
}

int update_ui()
{
	ticker_orderbook_ui();
	balance_ui();
	order_ui();
	if (menu_ui()) {
		return 1;
	}
	refresh();
	return 0;
}

static int get_user_input(char *buffer, int max_len)
{
    int ch = getch();
    if (ch == ERR) {
        return 0;
    }

    if (ch == '\n') {
        return 1;
    }

    if (ch == KEY_BACKSPACE || ch == 127) {
        if (strlen(buffer) > 0) {
            buffer[strlen(buffer) - 1] = '\0'; // backspace
        }
    }
    else if (strlen(buffer) < max_len - 1) {
        strncat(buffer, (char *)&ch, 1);
    }

	refresh();
    return 0;
}

static void ticker_orderbook_ui()
{
	int col_width = COLS / SYM_COUNT;
	for (int i = 0; i < SYM_COUNT; i++) {
        int col_start = i * col_width + 2;

        mvprintw(1, col_start, "[%s] Real-time Price and Orderbook", g_tickers[i].code);
        mvprintw(2, col_start, "----------------------------------------------");
        mvprintw(3, col_start, "Current Price: %'.2f KRW", g_tickers[i].trade_price);

		attron(COLOR_PAIR(strcmp(g_tickers[i].change, "RISE") == 0 ? RISE_COLOR :
            strcmp(g_tickers[i].change, "FALL") == 0 ? FALL_COLOR : DEFAULT_COLOR));

		mvprintw(4, col_start, "Change: %-*s", 40, "");
        mvprintw(4, col_start, "Change: %'.2f KRW (%'.2f%%) [%s]", g_tickers[i].signed_change_price,
				g_tickers[i].signed_change_rate * 100, g_tickers[i].change);

		attroff(COLOR_PAIR( strcmp(g_tickers[i].change, "RISE") == 0 ? RISE_COLOR :
					strcmp(g_tickers[i].change, "FALL") == 0 ? FALL_COLOR : DEFAULT_COLOR));

		if (g_tickers[i].high_price >= g_tickers[i].trade_price * 0.99) {
			attron(COLOR_PAIR(RISE_COLOR) | A_BOLD);
		} else {
			attron(COLOR_PAIR(RISE_COLOR));
		}

		mvprintw(5, col_start, "High: %'.2f KRW", g_tickers[i].high_price);

		attroff(COLOR_PAIR(RISE_COLOR) | A_BOLD);

		if (g_tickers[i].low_price <= g_tickers[i].trade_price * 1.01) {
			attron(COLOR_PAIR(FALL_COLOR) | A_BOLD);
		} else {
			attron(COLOR_PAIR(FALL_COLOR));
		}

		mvprintw(5, col_start + 20, "Low: %'.2f KRW", g_tickers[i].low_price);

		attroff(COLOR_PAIR(FALL_COLOR) | A_BOLD);

        mvprintw(6, col_start, "Prev Close: %'.2f KRW | Open: %'.2f KRW",
                 g_tickers[i].prev_closing_price, g_tickers[i].opening_price);
        mvprintw(7, col_start, "----------------------------------------------");
        mvprintw(8, col_start, "Orderbook (Best Bid / Ask)");

		attron(COLOR_PAIR(BID_COLOR));

		mvprintw(9, col_start, "Bid: %-*s", 40, "");
        mvprintw(9, col_start, "Bid: %'.2f KRW (%'.2f %s)",
                 g_orderbooks[i].best_bid_price, g_orderbooks[i].best_bid_size, g_symbols[i]);

		attroff(COLOR_PAIR(BID_COLOR));

		attron(COLOR_PAIR(ASK_COLOR));

		mvprintw(10, col_start, "Ask: %-*s", 40, "");
        mvprintw(10, col_start, "Ask: %'.2f KRW (%'.2f %s)",
                 g_orderbooks[i].best_ask_price, g_orderbooks[i].best_ask_size, g_symbols[i]);

		attroff(COLOR_PAIR(ASK_COLOR));

		mvprintw(11, col_start, "Spread: %-*s", 30, "");
        mvprintw(11, col_start, "Spread: %'.2f KRW", g_orderbooks[i].spread);
		mvprintw(12, col_start, "Bid/Ask Ratio: %-*s", 20, "");
        mvprintw(12, col_start, "Bid/Ask Ratio: %'.2f", g_orderbooks[i].bid_ask_ratio);
        mvprintw(13, col_start, "---------------------------------------------");
		mvprintw(14, col_start, "Recent Volume: %-*s", 20, "");
        mvprintw(14, col_start, "Recent Volume: %'.2f %s", g_tickers[i].trade_volume, g_symbols[i]);
		mvprintw(15, col_start, "24h Volume: %-*s", 20, "");
        mvprintw(15, col_start, "24h Volume: %'.2f %s", g_tickers[i].acc_trade_volume_24h, g_symbols[i]);
		mvprintw(16, col_start, "24h Trade Amount: %-*s", 20, "");
        mvprintw(16, col_start, "24h Trade Amount: %'.2f KRW", g_tickers[i].acc_trade_price_24h);
        mvprintw(17, col_start, "---------------------------------------------");
        mvprintw(18, col_start, "52W High: %'.2f KRW", g_tickers[i].highest_52_week_price);
        mvprintw(19, col_start, "52W Low: %'.2f KRW", g_tickers[i].lowest_52_week_price);
        mvprintw(20, col_start, "Market State: %s", g_tickers[i].market_state);
        mvprintw(21, col_start, "---------------------------------------------");
    }
}

static void balance_ui()
{
	mvprintw(23, 2, "Currency: %-8s Balance: %'15.2f Locked: %'10.2f",
			g_account[0].currency, g_account[0].balance, g_account[0].locked);
	mvprintw(24, 2, "Currency: %-8s Balance: %'15.2f Locked: %'10.2f",
			g_account[1].currency, g_account[1].balance, g_account[1].locked);
	mvprintw(25, 2, "Currency: %-8s Balance: %'15.2f Locked: %'10.2f",
			g_account[2].currency, g_account[2].balance, g_account[2].locked);
	mvprintw(26, 2, "Currency: %-8s Balance: %'15.2f Locked: %'10.2f",
			g_account[3].currency, g_account[3].balance, g_account[3].locked);
}

static int menu_ui()
{
    static int input_step = 0;
	static char menu_cmd[2] = {0};
	static char market_cmd[2] = {0};
    static char order_cmd[4] = {0};
    static char limit_cmd[7] = {0};
    static char price_str[20] = {0};
    static char volume_str[20] = {0};
    static char ok[2] = {0};
	static char cancel_num[2] = {0};
	static int market_idx = -1;
	static double price = 0.0, volume = 0.0;

	static int start = 0;
	if (start == 0) {
		enqueue_task(get_account_info_task, NULL);
		start = 1;
	}

	mvprintw(28, 2, "[COMMAND]OPTIONS >>>> [m]MAKE ORDER | [c]CANCEL ORDER | [q]QUIT >>>> ");
	mvprintw(28, 71, "%s", menu_cmd);
	clrtoeol();
	refresh();

    switch (input_step) {
        case 0:  // options
			if (get_user_input(menu_cmd, sizeof(menu_cmd))) {
				switch (menu_cmd[0]) {
					case 'm':
						if (atomic_load(&g_my_orders->is_full)) {
							mvprintw(28, 71, "Orders full.");
							refresh();
							napms(1000);
							input_step = 0;
						} else {
							input_step = 1;  // make order
						}
						menu_cmd[0] = '\0';
						break;
					case 'c':
						if (atomic_load(&g_my_orders->is_empty)) {
							mvprintw(28, 71, "No orders.");
							refresh();
							napms(1000);
							input_step = 0;
						} else {
							input_step = 10;
						}
						menu_cmd[0] = '\0';
						break;
					case 'q':
						atomic_store(&g_shutdown_flag, true);
						if (!atomic_load(&g_my_orders->is_empty)) {
							cancel_by_bulk("all", NULL, NULL, NULL, 10, NULL);
						}
						mvprintw(28, 71, "QUIT");
						refresh();
						napms(1000);
						return 1;
				}
			}
			break;

        case 1:
            mvprintw(29, 2, "[1]XRP/[2]ADA/[3]DOGE: ");
			mvprintw(29, 25, "%s", market_cmd);
            clrtoeol();
			refresh();
            if (get_user_input(market_cmd, sizeof(market_cmd))) {
				if (market_cmd[0] == '1' || market_cmd[0] == '2' ||
						market_cmd[0] == '3') {
					market_idx = market_cmd[0] - '0';
					market_idx -= 1;
					input_step = 2;
				} else {
					mvprintw(29, 25, "Invalid input. Please enter '1', '2' or '3'.");
					refresh();
					market_cmd[0] = '\0';
                    napms(1000);
                    move(29, 25);
                    clrtoeol();
                    refresh();
				}
            }
            break;

        case 2: // bid-ask
            mvprintw(29, 27, "[b]Bid/[a]Ask: ");
			mvprintw(29, 42, "%s", order_cmd);
            clrtoeol();
			refresh();
            if (get_user_input(order_cmd, sizeof(order_cmd))) {
				if (order_cmd[0] == 'a' || order_cmd[0] == 'b') {
					strcpy(order_cmd, order_cmd[0] == 'a' ? "ask" : "bid");
					input_step = 3;
				} else {
					mvprintw(29, 42, "Invalid input. Please enter 'b' or 'a'.");
					refresh();
					order_cmd[0] = '\0';
                    napms(1000);
                    move(29, 42);
                    clrtoeol();
                    refresh();
				}
            }
            break;

        case 3: // limit-market price
            mvprintw(29, 44, "[l]Limit/[m]Market: ");
			mvprintw(29, 64, "%s", limit_cmd);
            clrtoeol();
			refresh();
            if (get_user_input(limit_cmd, sizeof(limit_cmd))) {
				if (limit_cmd[0] == 'l') {
					strcpy(limit_cmd, "limit");
					input_step = 7;
				}
				else if (limit_cmd[0] == 'm') {
					strcpy(limit_cmd, order_cmd[0] == 'a' ? "market" : "price");
					input_step = limit_cmd[0] == 'm' ? 4 : 5;
				} else {
					mvprintw(29, 64, "Invalid input. Please enter 'l' or 'm'.");
					refresh();
					limit_cmd[0] = '\0';
                    napms(1000);
                    move(29, 64);
                    clrtoeol();
                    refresh();
				}
            }
            break;

        case 4: // ask, market
            mvprintw(29, 66, "Enter amount: ");
			mvprintw(29, 80, "%s", volume_str);
            clrtoeol();
			refresh();
            if (get_user_input(volume_str, sizeof(volume_str))) {
				char *endptr;
				volume = strtod(volume_str, &endptr);
				if (volume_str == endptr || *endptr != '\0') {
					mvprintw(29, 80, "Invalid volume.");
                    refresh();
                    volume_str[0] = '\0';
                    napms(1000);
                    move(29, 80);
                    clrtoeol();
                    refresh();
				} else {
					input_step = 6;
				}
            }
            break;

        case 5: // bid, price
            mvprintw(29, 67, "Enter price: ");
			mvprintw(29, 80, "%s", price_str);
            clrtoeol();
			refresh();
            if (get_user_input(price_str, sizeof(price_str))) {
				char *endptr;
				price = strtod(price_str, &endptr);
				if (price_str == endptr || *endptr != '\0') {
					mvprintw(29, 80, "Invalid price.");
                    refresh();
                    price_str[0] = '\0';
                    napms(1000);
                    move(29, 80);
                    clrtoeol();
                    refresh();
				} else {
					input_step = 6;
				}
            }
            break;

        case 6: // market price order
			if (order_cmd[0] == 'a') {
				mvprintw(29, 82 + strlen(volume_str), "send? [y]/[n] >> ");
				mvprintw(29, 100 + strlen(volume_str), "%s", ok);
			} else if (order_cmd[0] == 'b') {
				mvprintw(29, 81 + strlen(price_str), "send? [y]/[n] >> ");
				mvprintw(29, 99 + strlen(price_str), "%s", ok);
			}
            clrtoeol();
			refresh();
			if (get_user_input(ok, sizeof(ok))) {
				if (ok[0] == 'y') {
					if (order_cmd[0] == 'a') {
						order_t *o = make_order(g_codes[market_idx], order_cmd,
								0, volume, limit_cmd, NULL, NULL);
						enqueue_task(place_order_task, (void *)o);
						mvprintw(29, 100 + strlen(volume_str), "Sent.");
						refresh();
						napms(1000);
					} else if (order_cmd[0] == 'b') {
						order_t *o = make_order(g_codes[market_idx], order_cmd,
								price, 0, limit_cmd, NULL, NULL);
						enqueue_task(place_order_task, (void *)o);
						mvprintw(29, 99 + strlen(price_str), "Sent.");
						refresh();
						napms(1000);
					}
				} else {
					mvprintw(29, 100 + strlen(price_str), "Cancelled.");
					refresh();
					napms(1000);
				}
				menu_cmd[0] = '\0';
				market_cmd[0] = '\0';
				order_cmd[0] = '\0';
				limit_cmd[0] = '\0';
				price_str[0] = '\0';
				volume_str[0] = '\0';
				ok[0] = '\0';

				price = 0.0;
				volume = 0.0;
				market_idx = -1;

				input_step = 0;
				move(29, 0);
				clrtoeol();
				refresh();
			}
			break;

        case 7: // limit
            mvprintw(29, 66, "Enter price: ");
			mvprintw(29, 79, "%s", price_str);
            clrtoeol();
			refresh();
            if (get_user_input(price_str, sizeof(price_str))) {
				char *endptr;
				price = strtod(price_str, &endptr);
				if (volume_str == endptr || *endptr != '\0') {
					mvprintw(29, 79, "Invalid price.");
                    refresh();
                    price_str[0] = '\0';
                    napms(1000);
                    move(29, 79);
                    clrtoeol();
                    refresh();
				} else {
					input_step = 8;
				}
            }
            break;

        case 8:
            mvprintw(29, 81 + strlen(price_str), "Enter amount: ");
			mvprintw(29, 95 + strlen(price_str), "%s", volume_str);
            clrtoeol();
			refresh();
            if (get_user_input(volume_str, sizeof(volume_str))) {
				char *endptr;
				volume = strtod(volume_str, &endptr);
				if (volume_str == endptr || *endptr != '\0') {
					mvprintw(29, 81 + strlen(price_str), "Invalid volume.");
                    refresh();
                    volume_str[0] = '\0';
                    napms(1000);
                    move(29, 95 + strlen(price_str));
                    clrtoeol();
                    refresh();
				} else {
					input_step = 9;
				}
            }
            break;

        case 9:
            mvprintw(29, 97 + strlen(price_str) + strlen(volume_str), "send? [y]/[n] >> ");
			mvprintw(29, 114 + strlen(price_str) + strlen(volume_str), "%s", ok);
			clrtoeol();
			refresh();
			if (get_user_input(ok, sizeof(ok))) {
				if (ok[0] == 'y') {
					// send order rest api
					order_t *o = make_order(g_codes[market_idx], order_cmd,
							price, volume, limit_cmd, NULL, NULL);
					enqueue_task(place_order_task, (void *)o);
					mvprintw(29, 114 + strlen(price_str) + strlen(volume_str), "Sent.");
					refresh();
					napms(1000);
				} else {
					mvprintw(29, 114 + strlen(price_str) + strlen(volume_str), "Cancelled.");
					refresh();
					napms(1000);
				}
				menu_cmd[0] = '\0';
				market_cmd[0] = '\0';
				order_cmd[0] = '\0';
				limit_cmd[0] = '\0';
				price_str[0] = '\0';
				volume_str[0] = '\0';
				ok[0] = '\0';

				price = 0.0;
				volume = 0.0;
				market_idx = -1;

				input_step = 0;
				move(30, 0);
				move(29, 0);
				clrtoeol();
				refresh();
			}
            break;

        case 10: // cancel order
            mvprintw(29, 2, "Cancel order[num] >> ");
			mvprintw(29, 23, "%s", cancel_num);
			clrtoeol();
			refresh();
			if (get_user_input(cancel_num, sizeof(cancel_num))) {
				if (isdigit(cancel_num[0])) {
					// send order rest api
					int cn = cancel_num[0] - '0';
					char *uuid;
					MALLOC(uuid, 37);
					get_uuid_from_order_arr(cn, &uuid);
					if (uuid) {
						enqueue_task(cancel_order_task, (void *)uuid);
					} else {
						FREE(uuid);
					}
                    clrtoeol();
                    refresh();
				} else {
					mvprintw(29, 26, "Wrong input.");
					refresh();
                    napms(1000);
				}
				menu_cmd[0] = '\0';
				order_cmd[0] = '\0';
				cancel_num[0] = '\0';
				ok[0] = '\0';
                input_step = 0;

				move(29, 0);
                clrtoeol();
				refresh();
            }
            break;
    }

    return 0;
}

static void order_ui()
{
	int col_width = COLS / MAX_ORDER_NUM;
	for (int i = 0; i < MAX_ORDER_NUM; i++) {
        int col_start = i * col_width + 2;

		for (int row = 31; row <= 38; row++) {
            mvprintw(row, col_start, "|");
        }

		if (g_my_orders->order_arr != NULL && g_my_orders->order_arr[i] != NULL) {
			order_t *o = g_my_orders->order_arr[i];
			mvprintw(31, col_start + 2, "[%d]ORDER", i);
			mvprintw(32, col_start + 2, "%s", o->market);
			mvprintw(33, col_start + 2, "Price:");
			mvprintw(34, col_start + 2, "%'.2f", o->price);
			mvprintw(35, col_start + 2, "Volume:");
			mvprintw(36, col_start + 2, "%'.2f", o->volume);
			mvprintw(37, col_start + 2, "Side: %s", o->side);
			mvprintw(38, col_start + 2, "State: %s", o->status);
		} else {
			for (int row = 31; row <= 38; row++) {
				mvprintw(row, col_start + 2, "%-*s", col_width - 2, " ");
			}
		}
	}
}
