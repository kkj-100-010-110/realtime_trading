#ifndef _ORDER_HANDLER_H
#define _ORDER_HANDLER_H

#include "common.h"

#define MAX_ORDER 10

typedef struct {
	const char *uuid; // this points to its key value in rb_tree
	char market[16];
    double volume;
    double price;
    char side[8];
    char status[16];
} Order;

typedef struct {
	char side[4];				// "all", "bid", "ask"
    char pairs[16];				// "KRW-BTC,KRW-ETH"
    char excluded_pairs[16];	// "KRW-BTC"
    char quote_currencies[4];	// "KRW"
    int count;					// 0 (default: 20)
    char order_by[5];			// "asc", "desc"
} CancelOption;

typedef struct {
	char market[16];		// "KRW-BTC"
	char states[2][7];		// "wait", "watch" for open / "done", "cancel" for closed
	size_t states_count;	// 0, 1 and 2
	int page;				// default 1
	int limit;				// default 100 for open and 1000 for closed
	char order_by[5];		// "asc", "desc"
	long start_time;		// ISO-8601 or Timestamp. using timestamp here
	long end_time;			// start_time and end_time for closed orders
} OrderStatus;

/* for char states[2][7] in OrderStatus */
extern const char *closed_states[2];
extern const char *open_states[2];

/* data structure, tree & array */
extern RB_tree *orders;
extern pthread_mutex_t orders_mutex;
extern Order **order_arr;
extern atomic_bool order_full;

/* creating structures to make it one portion of args and send it to thread queue */
Order *make_order(const char *market, const char *side, double price, double volume,
		const char * status);
CancelOption *create_cancel_option(const char *side, const char *pairs,
		const char *excluded_pairs, const char *quote_currencies, int count,
		const char *order_by);
OrderStatus *create_order_status(const char *market, size_t states_count,
		long start_time, long end_time, int page, int limit, const char *order_by);


/* init extern variables */
void init_order_handler();
void insert_order(const char *uuid, const char *market, double volume,
				  double price, const char *side, const char *status);
void find_order(const char *uuid, Order **o);
void remove_order(const char *uuid);
/* remove_order for ui */
void ui_remove_order(int i);
void update_order_status(const char *uuid, const char *status);
void destroy_order_handler();
/* print for test */
void print_order(RB_node *root);

#endif//_ORDER_HANDLER_H
