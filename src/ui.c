#include "ui.h"

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

        mvprintw(1, col_start + 2, "[%s] Real-time Price and Orderbook", g_tickers[i].code);
        mvprintw(2, col_start, "---------------------------------------------");
        mvprintw(3, col_start + 2, "Current Price: %.2f KRW", g_tickers[i].trade_price);
        mvprintw(4, col_start + 2, "Change: %.2f KRW (%.2f%%) [%s]", g_tickers[i].signed_change_price,
				g_tickers[i].signed_change_rate * 100, g_tickers[i].change);
        mvprintw(5, col_start + 2, "High: %.2f KRW | Low: %.2f KRW",
                 g_tickers[i].high_price, g_tickers[i].low_price);
        mvprintw(6, col_start + 2, "Prev Close: %.2f KRW | Open: %.2f KRW",
                 g_tickers[i].prev_closing_price, g_tickers[i].opening_price);
        mvprintw(7, col_start, "---------------------------------------------");
        mvprintw(8, col_start + 2, "Orderbook (Best Bid / Ask)");
        mvprintw(9, col_start + 2, "Bid: %.2f KRW (%.2f %s)",
                 g_orderbooks[i].best_bid_price, g_orderbooks[i].best_bid_size, g_symbols[i]);
        mvprintw(10, col_start + 2, "Ask: %.2f KRW (%.2f %s)",
                 g_orderbooks[i].best_ask_price, g_orderbooks[i].best_ask_size, g_symbols[i]);
        mvprintw(11, col_start + 2, "Spread: %.2f KRW", g_orderbooks[i].spread);
        mvprintw(12, col_start + 2, "Bid/Ask Ratio: %.2f", g_orderbooks[i].bid_ask_ratio);
        mvprintw(13, col_start, "--------------------------------------------");
        mvprintw(14, col_start + 2, "Recent Volume: %.2f %s", g_tickers[i].trade_volume, g_symbols[i]);
        mvprintw(15, col_start + 2, "24h Volume: %.2f %s", g_tickers[i].acc_trade_volume_24h, g_symbols[i]);
        mvprintw(16, col_start + 2, "24h Trade Amount: %.2f KRW", g_tickers[i].acc_trade_price_24h);
        mvprintw(17, col_start, "--------------------------------------------");
        mvprintw(18, col_start + 2, "52W High: %.2f KRW", g_tickers[i].highest_52_week_price);
        mvprintw(19, col_start + 2, "52W Low: %.2f KRW", g_tickers[i].lowest_52_week_price);
        mvprintw(20, col_start + 2, "Market State: %s", g_tickers[i].market_state);
        mvprintw(21, col_start, "--------------------------------------------");
    }
}

static void balance_ui()
{
	mvprintw(23, 2, "Currency: %s\t Balance: %.2f\t Locked: %.2f",
			g_account[0].currency, g_account[0].balance, g_account[0].locked);
	mvprintw(24, 2, "Currency: %s\t Balance: %.2f\t Locked: %.2f",
			g_account[1].currency, g_account[1].balance, g_account[1].locked);
	mvprintw(25, 2, "Currency: %s\t Balance: %.2f\t Locked: %.2f",
			g_account[2].currency, g_account[2].balance, g_account[2].locked);
	mvprintw(26, 2, "Currency: %s\t Balance: %.2f\t Locked: %.2f",
			g_account[3].currency, g_account[3].balance, g_account[3].locked);
}

static int menu_ui()
{
    static int input_step = 0;
	static char menu_cmd[2] = {0};
	static char market_cmd[2] = {0};
    static char order_cmd[4] = {0};
    static char price_str[20] = {0};
    static char volume_str[20] = {0};
    static char ok[2] = {0};

	int market_idx = -1;
	double price = 0.0, volume = 0.0;

	mvprintw(28, 2, "[COMMAND]OPTIONS >>>> [u]UPDATE ACCOUNT | [m]MAKE ORDER | "
			"[c]CANCEL ORDER | [n]AUTO TRADING ON | [f]AUTO TRADING OFF | "
			"[q]QUIT >>>> ");
	mvprintw(28, 134, "%s", menu_cmd);
	clrtoeol();
	refresh();

    switch (input_step) {
        case 0:  // options
			if (get_user_input(menu_cmd, sizeof(menu_cmd))) {
				switch (menu_cmd[0]) {
					case 'u':
						enqueue_task(get_account_info_task, NULL);
						input_step = 0;  // back to options
						menu_cmd[0] = '\0';
						break;
					case 'm':
						input_step = 1;  // make order
						menu_cmd[0] = '\0';
						break;
					case 'c':
						input_step = 6;
						break;
					case 'q':
						// cancel all orders.
						// check the orders done.
						return 1;
				}
			}
			break;

        case 1:
            mvprintw(29, 2, "[1]XRP/[2]ADA/[3]DOGE: ");
			mvprintw(29, 23, "%s", market_cmd);
            clrtoeol();
			refresh();
            if (get_user_input(order_cmd, sizeof(order_cmd))) {
				if (market_cmd[0] == '1' || market_cmd[0] == '2' ||
						market_cmd[0] == '3') {
					market_idx = market_cmd[0] - '0';
					market_idx -= 1;
					input_step = 2;
				} else {
					mvprintw(29, 23, "Invalid input. Please enter '1', '2' or '3'.");
					refresh();
					market_cmd[0] = '\0';
                    napms(1000);
                    move(29, 17);
                    clrtoeol();
                    refresh();
				}
            }
            break;

        case 2:
            mvprintw(29, 25, "[b]Bid/[a]Ask: ");
			mvprintw(29, 40, "%s", order_cmd);
            clrtoeol();
			refresh();
            if (get_user_input(order_cmd, sizeof(order_cmd))) {
				if (order_cmd[0] == 'a' || order_cmd[0] == 'b') {
					strcpy(order_cmd, order_cmd[0] == 'a' ? "ask" : "bid");
					input_step = 3;
				} else {
					mvprintw(29, 40, "Invalid input. Please enter 'b' or 'a'.");
					refresh();
					order_cmd[0] = '\0';
                    napms(1000);
                    move(29, 40);
                    clrtoeol();
                    refresh();
				}
            }
            break;

        case 3:
            mvprintw(29, 40, "Enter price: ");
			mvprintw(29, 53, "%s", price_str);
            clrtoeol();
			refresh();
            if (get_user_input(price_str, sizeof(price_str))) {
				char *endptr;
				price = strtod(price_str, &endptr);
				if (price_str == endptr || *endptr != '\0') {
					mvprintw(29, 53, "Invalid price.");
                    refresh();
                    price_str[0] = '\0';
                    napms(1000);
                    move(29, 53);
                    clrtoeol();
                    refresh();
				} else {
					input_step = 4;
				}
            }
            break;

        case 4:
            mvprintw(29, 55 + strlen(price_str), "Enter amount: ");
			mvprintw(29, 69 + strlen(price_str), "%s", volume_str);
            clrtoeol();
			refresh();
            if (get_user_input(volume_str, sizeof(volume_str))) {
				char *endptr;
				volume = strtod(volume_str, &endptr);
				if (volume_str == endptr || *endptr != '\0') {
					mvprintw(29, 69 + strlen(price_str), "Invalid volume.");
                    refresh();
                    volume_str[0] = '\0';
                    napms(1000);
                    move(29, 69 + strlen(price_str));
                    clrtoeol();
                    refresh();
				} else {
					input_step = 5;
				}
            }
            break;

        case 5:
            mvprintw(29, 69 + strlen(price_str) + strlen(volume_str), "send? [y]/[n] >> ");
			mvprintw(29, 87 + strlen(price_str) + strlen(volume_str), "%s", ok);
			clrtoeol();
			refresh();
			if (get_user_input(ok, sizeof(ok))) {
				if (ok[0] == 'y') {
					// send order rest api
					order_t *o = make_order(g_codes[market_idx], order_cmd, price,
							volume, NULL, NULL, NULL);
					enqueue_task(place_order_task, (void *)o);
					mvprintw(29, 87 + strlen(price_str) + strlen(volume_str), "Sent.");
                    refresh();
                    napms(1000);
				} 
				order_cmd[0] = '\0';
				price_str[0] = '\0';
				volume_str[0] = '\0';
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

		if (g_order_arr != NULL && g_order_arr[i] != NULL) {
			order_t *o = g_order_arr[i];
			mvprintw(31, col_start + 2, "[%d]ORDER", i);
			mvprintw(32, col_start + 2, "%s", o->market);
			mvprintw(33, col_start + 2, "Price: %.2f", o->price);
			mvprintw(34, col_start + 2, "Volume: %.2f", o->volume);
			mvprintw(35, col_start + 2, "Side: %s", o->side);
			mvprintw(36, col_start + 2, "Status: %s", o->status);
		}
	}
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
