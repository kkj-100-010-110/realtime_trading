#include "order_handler.h"

/* for char states[2][7] in OrderStatus */
const char *closed_states[2] = { "done", "cancel" };
const char *open_states[2] = { "wait", "watch" };


RB_tree *orders = NULL;
pthread_mutex_t orders_mutex;
Order **order_arr = NULL;
atomic_bool order_full = ATOMIC_VAR_INIT(false);

static uint16_t indices = 0;

static int find_empty_index()
{
    for (int i = 0; i < MAX_ORDER; i++) {
        if (!(indices & (1 << i))) {
            return i;
        }
    }
    return -1; // full
}

Order *make_order(const char *market, const char *side, double price, double volume,
		const char *status)
{
	Order *o;
	MALLOC(o, sizeof(Order));
	if (!market || !side) {
		return NULL;
	}
	strcpy(o->market, market);
	strcpy(o->side, side);
	o->price = price;
	o->volume = volume;
	if (status) {
		strcpy(o->side, side);
	}
	return o;
}

CancelOption *create_cancel_option(const char *side, const char *pairs,
		const char *excluded_pairs, const char *quote_currencies, int count,
		const char *order_by)
{
	CancelOption *c;
	MALLOC(c, sizeof(CancelOption));

	if (side != NULL) strcpy(c->side, side);
	else c->side[0] = '\0';

	if (pairs != NULL) strcpy(c->pairs, pairs);
	else c->pairs[0] = '\0';
	
	if (excluded_pairs != NULL) strcpy(c->excluded_pairs, excluded_pairs);
	else c->excluded_pairs[0] = '\0';
	
	if (quote_currencies != NULL) strcpy(c->quote_currencies, quote_currencies);
	else c->quote_currencies[0] = '\0';
	
	c->count = count;

	if (order_by != NULL) strcpy(c->order_by, order_by);
	else c->order_by[0] = '\0';
	
	return c;
}

OrderStatus *create_order_status(const char *market, size_t states_count,
		long start_time, long end_time, int page, int limit, const char *order_by)
{
	OrderStatus *os;
	MALLOC(os, sizeof(OrderStatus));

	if (market) strcpy(os->market, market);
	else os->market[0] = '\0';

	if (order_by) strcpy(os->order_by, order_by);

	os->states_count = states_count;
	os->start_time = start_time;
	os->end_time = end_time;
	os->page = page;
	os->limit = limit;

	return os;
}

void init_order_handler()
{
	MALLOC(order_arr, sizeof(Order *) * MAX_ORDER);
	orders = rb_create(comparison_s, free_key, free_data);
	pthread_mutex_init(&orders_mutex, NULL);
}

void insert_order(const char *uuid, const char *market, double volume,
				  double price, const char *side, const char *status)
{
	pthread_mutex_lock(&orders_mutex);

	char *key = strdup(uuid);
	if (!key) {
		pr_err("strdup() failed: %s:%d", __FILE__, __LINE__);
		LOG_ERR("strdup() failed: %s:%d", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
	Order *data;
	MALLOC(data, sizeof(data));
	data->uuid = key;
	strncpy(data->market, market, strlen(market));
	data->volume = volume;
	data->price = price;
	strncpy(data->side, side, strlen(side));
	strncpy(data->status, status, strlen(status));
	rb_insert(orders, (void *)key, (void *)data);

	int idx = find_empty_index();
    if (idx != -1) {
        order_arr[idx] = data;
        indices |= (1 << idx);
    } else {
		atomic_store(&order_full, true);
    }

	pthread_mutex_unlock(&orders_mutex);
}


void find_order(const char *uuid, Order **o)
{
	pthread_mutex_lock(&orders_mutex);

	*o = rb_find(orders->root, orders->compare, (void *)uuid)->data;

	pthread_mutex_unlock(&orders_mutex);
}

static void remove_order_arr(const char *uuid)
{
	pthread_mutex_lock(&orders_mutex);

	for (int i = 0; i < MAX_ORDER; i+=1) {
		if (strcmp(order_arr[i]->uuid, uuid) == 0) {
			order_arr[i] = NULL;
			indices &= ~(1 << i);
			if (atomic_load(&order_full)) {
				atomic_store(&order_full, false);
			}
		}
	}

	pthread_mutex_unlock(&orders_mutex);
}

void remove_order(const char *uuid)
{
	pthread_mutex_lock(&orders_mutex);

	remove_order_arr(uuid);
	rb_remove(orders, (void *)uuid);

	pthread_mutex_unlock(&orders_mutex);
}

void ui_remove_order(int i)
{
	pthread_mutex_lock(&orders_mutex);

	if (i >= 0 && i < MAX_ORDER) {
		rb_remove(orders, (void *)order_arr[i]->uuid);
		order_arr[i] = NULL;
		indices &= ~(1 << i);
		if (atomic_load(&order_full)) {
			atomic_store(&order_full, false);
		}
	}

	pthread_mutex_unlock(&orders_mutex);
}


void update_order_status(const char *uuid, const char *status)
{
	pthread_mutex_lock(&orders_mutex);

	Order *o = NULL;
	find_order(uuid, &o);
	strcpy(o->status, status);

	pthread_mutex_unlock(&orders_mutex);
}

void destroy_order_handler()
{
	pthread_mutex_lock(&orders_mutex);

	if (order_arr) {
		free(order_arr);
	}

	rb_remove_tree(orders);
	orders = NULL;

	pthread_mutex_unlock(&orders_mutex);

	pthread_mutex_destroy(&orders_mutex);
}

void print_order(RB_node *root)
{
	if (!root) return;

	print_order(root->link[LEFT]);
	Order *order = (Order *)root->data;
	pr_out("Order: %s, %s, %.2f, %.2f, %s, %s\n",
			(char *)root->key, order->market, order->volume, order->price,
			order->side, order->status);
	print_order(root->link[RIGHT]);
}
