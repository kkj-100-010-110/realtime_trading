#ifndef _ORDER_HANDLER_H
#define _ORDER_HANDLER_H

#include "common.h"

#define MAX_ORDER_NUM 10

typedef struct {
	char uuid[37];
	char market[16];
    double volume;
    double price;
    char side[8];
	char ord_type[7];
	char time_in_force[4];
    char status[16];
} order_t;

typedef struct {
	char side[4];				// "all", "bid", "ask"
    char pairs[16];				// "KRW-BTC,KRW-ETH"
    char excluded_pairs[16];	// "KRW-BTC"
    char quote_currencies[4];	// "KRW"
    int count;					// 0 (default: 20)
    char order_by[5];			// "asc", "desc"
} cancel_option_t;

typedef struct {
	char market[16];		// "KRW-BTC"
	char states[2][7];		// "wait", "watch" for open / "done", "cancel" for closed
	size_t states_count;	// 0, 1 and 2
	int page;				// default 1
	int limit;				// default 100 for open and 1000 for closed
	char order_by[5];		// "asc", "desc"
	long start_time;		// ISO-8601 or Timestamp. using timestamp here
	long end_time;			// start_time and end_time for closed orders
} order_status_t;

/* for char states[2][7] in order_status_t */
extern const char *g_closed_states[2];
extern const char *g_open_states[2];

/* data */
typedef struct {
	order_t **orders;
	size_t size;
	atomic_bool is_full;
	atomic_bool is_empty;
	pthread_mutex_t lock;
} order_manager_t;

extern order_manager_t *g_my_orders;

/* creating structures to make it one portion of args and send it to thread queue */
order_t *make_order(const char *market, const char *side, double price, double volume,
		const char *ord_type, const char *time_in_force, const char *status);
cancel_option_t *create_cancel_option(const char *side, const char *pairs,
		const char *excluded_pairs, const char *quote_currencies, int count,
		const char *order_by);
order_status_t *create_order_status(const char *market, size_t states_count,
		long start_time, long end_time, int page, int limit, const char *order_by);


/* init extern variables */
void init_order_handler();

void insert_order(const char *uuid, const char *market, double volume, double price,
		const char *side, const char *ord_type, const char *status);
void remove_order(const char *uuid);
void update_order_status(const char *uuid, const char *status);
void update_order_volume(const char *uuid, double remaining_volume);
void destroy_order_handler();

void get_uuid_from_orders(int idx, char **uuid);

/* print for test */
void print_order();

#endif//_ORDER_HANDLER_H
