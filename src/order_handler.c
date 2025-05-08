#include "order_handler.h"

/* for char states[2][7] in order_status_t */
const char *g_closed_states[2] = { "done", "cancel" };
const char *g_open_states[2] = { "wait", "watch" };

static pthread_mutex_t order_mtx = PTHREAD_MUTEX_INITIALIZER;

order_manager_t *g_my_orders = NULL;

order_t *make_order(const char *market, const char *side, double price, double volume,
		const char *ord_type, const char *time_in_force, const char *status)
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
	if (ord_type) {
		strcpy(o->ord_type, ord_type);
	}
	if (time_in_force) {
		strcpy(o->time_in_force, time_in_force);
	}
	if (status) {
		strcpy(o->status, status);
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
	else *c->side = '\0';

	if (pairs != NULL) strcpy(c->pairs, pairs);
	else *c->pairs = '\0';
	
	if (excluded_pairs != NULL) strcpy(c->excluded_pairs, excluded_pairs);
	else *c->excluded_pairs = '\0';
	
	if (quote_currencies != NULL) strcpy(c->quote_currencies, quote_currencies);
	else *c->quote_currencies = '\0';
	
	c->count = count;

	if (order_by != NULL) strcpy(c->order_by, order_by);
	else *c->order_by = '\0';
	
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
	g_my_orders = (order_manager_t *)malloc(sizeof(order_manager_t));
	if (!g_my_orders) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}

	g_my_orders->orders = (order_t **)malloc(sizeof(order_t *) * MAX_ORDER_NUM);
	if (!g_my_orders->orders) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < MAX_ORDER_NUM; i++) {
		g_my_orders->orders[i] = NULL;
	}

	g_my_orders->size = 0;
	g_my_orders->is_full = ATOMIC_VAR_INIT(false);
	g_my_orders->is_empty = ATOMIC_VAR_INIT(true);
	pthread_mutex_init(&g_my_orders->lock, NULL);
}

static void delete_orders()
{
	for (int i = 0; i < MAX_ORDER_NUM; i++) {
		if (g_my_orders->orders[i] != NULL) {
			free(g_my_orders->orders[i]);
			g_my_orders->orders[i] = NULL;
		}
	}
	free(g_my_orders->orders);
	g_my_orders->orders = NULL;
}

void destroy_order_handler()
{
	pthread_mutex_lock(&order_mtx);

	if (g_my_orders->orders) {
		delete_orders();
	}
	pthread_mutex_destroy(&g_my_orders->lock);
	free(g_my_orders);
	g_my_orders = NULL;

	pthread_mutex_unlock(&order_mtx);
	pthread_mutex_destroy(&order_mtx);
}

void insert_order(const char *uuid, const char *market, double volume, double price,
		const char *side, const char *ord_type, const char *status)
{
	pthread_mutex_lock(&order_mtx);
	pthread_mutex_lock(&g_my_orders->lock);

	if (g_my_orders->size == MAX_ORDER_NUM) {
	//if (atomic_load(&g_my_orders->is_full)) {
		pthread_mutex_unlock(&g_my_orders->lock);
		pthread_mutex_unlock(&order_mtx);
		return;
	} else {
		if (g_my_orders->size == 0) {
			atomic_store(&g_my_orders->is_empty, false);
		}

		order_t *data = (order_t *)malloc(sizeof(order_t));
		if (!data) {
			pthread_mutex_unlock(&g_my_orders->lock);
			pthread_mutex_unlock(&order_mtx);
			pr_err("malloc() failed.");
			MY_LOG_ERR("malloc() failed.");
			exit(EXIT_FAILURE);
		}

		strncpy(data->uuid, uuid, sizeof(data->uuid));
		strncpy(data->market, market, sizeof(data->market));
		data->volume = volume;
		data->price = price;
		strncpy(data->side, side, sizeof(data->side));
		strncpy(data->ord_type, ord_type, sizeof(data->ord_type));
		strncpy(data->status, status, sizeof(data->status));

		int idx;
		for (idx = 0; idx < MAX_ORDER_NUM; idx++) {
			if (g_my_orders->orders[idx] == NULL) {
				break;
			}
		}
		g_my_orders->orders[idx] = data;
		g_my_orders->size++;
		if (g_my_orders->size == MAX_ORDER_NUM) {
			atomic_store(&g_my_orders->is_full, true);
		}
	}

	pthread_mutex_unlock(&g_my_orders->lock);
	pthread_mutex_unlock(&order_mtx);
}

void remove_order(const char *uuid)
{
	pthread_mutex_lock(&order_mtx);
	pthread_mutex_lock(&g_my_orders->lock);
	if (g_my_orders->size == 0) {
	//if (atomic_load(&g_my_orders->is_empty)) {
		pthread_mutex_unlock(&g_my_orders->lock);
		pthread_mutex_unlock(&order_mtx);
		return;
	} else {
		if (g_my_orders->size == MAX_ORDER_NUM) {
			atomic_store(&g_my_orders->is_full, false);
		}
		for (int i = 0; i < MAX_ORDER_NUM; i++) {
			if (g_my_orders->orders[i] != NULL &&
					strcmp(g_my_orders->orders[i]->uuid, uuid) == 0) {
				free(g_my_orders->orders[i]);
				g_my_orders->orders[i] = NULL;
				g_my_orders->size--;
				break;
			}
		}
		if (g_my_orders->size == 0) {
			atomic_store(&g_my_orders->is_empty, true);
		}
	}
	pthread_mutex_unlock(&g_my_orders->lock);
	pthread_mutex_unlock(&order_mtx);
}

void get_uuid_from_orders(int idx, char **uuid)
{
	pthread_mutex_lock(&order_mtx);
	pthread_mutex_lock(&g_my_orders->lock);

	if (g_my_orders->orders[idx] != NULL) {
		strcpy(*uuid, g_my_orders->orders[idx]->uuid);
	} else {
		*uuid = NULL;
	}

	pthread_mutex_unlock(&g_my_orders->lock);
	pthread_mutex_unlock(&order_mtx);
}

void update_order_status(const char *uuid, const char *status)
{
	pthread_mutex_lock(&order_mtx);
	pthread_mutex_lock(&g_my_orders->lock);

	for (int i = 0; i < MAX_ORDER_NUM; i++) {
		if (g_my_orders->orders[i] != NULL &&
				strcmp(g_my_orders->orders[i]->uuid, uuid) == 0) {
			strcpy(g_my_orders->orders[i]->status, status);
		}
	}

	pthread_mutex_unlock(&g_my_orders->lock);
	pthread_mutex_unlock(&order_mtx);
}

void update_order_volume(const char *uuid, double remaining_volume)
{
	pthread_mutex_lock(&order_mtx);
	pthread_mutex_lock(&g_my_orders->lock);

	for (int i = 0; i < MAX_ORDER_NUM; i++) {
		if (strcmp(g_my_orders->orders[i]->uuid, uuid) == 0) {
			g_my_orders->orders[i]->volume = remaining_volume;
		}
	}

	pthread_mutex_unlock(&g_my_orders->lock);
	pthread_mutex_unlock(&order_mtx);
}

// for debug, if using this, data race occurs
//static void _print_order(rb_node_t *root)
//{
//	if (!root) return;
//
//	_print_order(root->link[LEFT]);
//	order_t *order = (order_t *)root->data;
//	pr_out("Order: %s, %s, %.2f, %.2f, %s, %s\n",
//			(char *)root->key, order->market, order->volume, order->price,
//			order->side, order->status);
//	_print_order(root->link[RIGHT]);
//}

void print_order()
{
	//_print_order(g_my_orders->orders->root);
}
