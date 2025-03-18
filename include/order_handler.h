#ifndef _ORDER_HANDLER_H
#define _ORDER_HANDLER_H

//#include "common.h"
#include "../include/common.h"

typedef struct {
	char market[16];
    double volume;
    double price;
    char side[8];
    char status[16];
} Order;

extern RB_tree *orders;
extern pthread_mutex_t orders_mutex;

void init_order_handler();
void insert_order(const char *uuid, const char *market, double volume,
				  double price, const char *side, const char *status);
void find_order(const char *uuid, Order **o);
void remove_order(const char *uuid);
void update_order_status(const char *uuid, const char *status);
void destroy_order_handler();

void print_order(RB_node *root);


#endif//_ORDER_HANDLER_H
