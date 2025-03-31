#include "order_handler.h"

/* for char states[2][7] in order_status_t */
const char *g_closed_states[2] = { "done", "cancel" };
const char *g_open_states[2] = { "wait", "watch" };


rb_tree_t *g_orders = NULL;
order_t **g_order_arr = NULL;
atomic_bool g_orders_full = ATOMIC_VAR_INIT(false);
atomic_bool g_orders_empty = ATOMIC_VAR_INIT(true);

static uint16_t indices = 0;
static pthread_mutex_t order_mtx = PTHREAD_MUTEX_INITIALIZER;

static int find_empty_index()
{
    for (int i = 0; i < MAX_ORDER_NUM; i++) {
        if (!(indices & (1 << i))) {
            return i;
        }
    }
    return -1; // full
}

order_t *make_order(const char *market, const char *side, double price, double volume,
		const char *status)
{
	order_t *o;
	MALLOC(o, sizeof(order_t));
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

cancel_option_t *create_cancel_option(const char *side, const char *pairs,
		const char *excluded_pairs, const char *quote_currencies, int count,
		const char *order_by)
{
	cancel_option_t *c;
	MALLOC(c, sizeof(cancel_option_t));

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

order_status_t *create_order_status(const char *market, size_t states_count,
		long start_time, long end_time, int page, int limit, const char *order_by)
{
	order_status_t *os;
	MALLOC(os, sizeof(order_status_t));

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
	g_order_arr = (order_t **)malloc(sizeof(order_t *) * MAX_ORDER_NUM);
	if (!g_order_arr) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}
	g_orders = rb_create(comparison_str, free_key, free_data);
	if (!g_orders) {
		pr_err("rb_create() failed.");
		exit(EXIT_FAILURE);
	}
}

void insert_order(const char *uuid, const char *market, double volume,
				  double price, const char *side, const char *status)
{
	pthread_mutex_lock(&order_mtx);

	char *key = strdup(uuid);
	if (!key) {
		pr_err("strdup() failed.");
		LOG_ERR("strdup() failed.");
		exit(EXIT_FAILURE);
	}
	order_t *data;
	MALLOC(data, sizeof(data));
	data->uuid = key;
	strncpy(data->market, market, strlen(market));
	data->volume = volume;
	data->price = price;
	strncpy(data->side, side, strlen(side));
	strncpy(data->status, status, strlen(status));
	rb_insert(g_orders, (void *)key, (void *)data);

	int idx = find_empty_index();
    if (idx != -1) {
        g_order_arr[idx] = data;
        indices |= (1 << idx);
		if (atomic_load(&g_orders_empty)) {
			atomic_store(&g_orders_empty, false);
		}
    } else {
		atomic_store(&g_orders_full, true);
    }

	pthread_mutex_unlock(&order_mtx);
}


void find_order(const char *uuid, order_t **o)
{
	pthread_mutex_lock(&order_mtx);

	*o = rb_find(g_orders->root, g_orders->compare, (void *)uuid)->data;

	pthread_mutex_unlock(&order_mtx);
}

static void remove_order_arr(const char *uuid)
{
	pthread_mutex_lock(&order_mtx);

	int i;
	for (i = 0; i < MAX_ORDER_NUM; i+=1) {
		if (strcmp(g_order_arr[i]->uuid, uuid) == 0) {
			g_order_arr[i] = NULL;
			indices &= ~(1 << i);
			if (atomic_load(&g_orders_full)) {
				atomic_store(&g_orders_full, false);
			}
			break;
		}
	}
	for (i = 0; i < MAX_ORDER_NUM; i+=1) {
		if (g_order_arr[i] != NULL)
			break;
	}
	if (i == MAX_ORDER_NUM)
		atomic_store(&g_orders_empty, true);

	pthread_mutex_unlock(&order_mtx);
}

void remove_order(const char *uuid)
{
	pthread_mutex_lock(&order_mtx);

	remove_order_arr(uuid);
	rb_remove(g_orders, (void *)uuid);

	pthread_mutex_unlock(&order_mtx);
}

void ui_remove_order(int i)
{
	pthread_mutex_lock(&order_mtx);

	if (i >= 0 && i < MAX_ORDER_NUM) {
		rb_remove(g_orders, (void *)g_order_arr[i]->uuid);
		g_order_arr[i] = NULL;
		indices &= ~(1 << i);
		if (atomic_load(&g_orders_full)) {
			atomic_store(&g_orders_full, false);
		}
	}

	pthread_mutex_unlock(&order_mtx);
}


void update_order_status(const char *uuid, const char *status)
{
	pthread_mutex_lock(&order_mtx);

	order_t *o = NULL;
	find_order(uuid, &o);
	strcpy(o->status, status);

	pthread_mutex_unlock(&order_mtx);
}

void destroy_order_handler()
{
	pthread_mutex_lock(&order_mtx);

	if (g_order_arr) {
		free(g_order_arr);
	}

	rb_remove_tree(g_orders);
	g_orders = NULL;

	pthread_mutex_unlock(&order_mtx);

	pthread_mutex_destroy(&order_mtx);
}

void print_order(rb_node_t *root)
{
	if (!root) return;

	print_order(root->link[LEFT]);
	order_t *order = (order_t *)root->data;
	pr_out("Order: %s, %s, %.2f, %.2f, %s, %s\n",
			(char *)root->key, order->market, order->volume, order->price,
			order->side, order->status);
	print_order(root->link[RIGHT]);
}
