#include "order_handler.h"

RB_tree *orders = NULL;
pthread_mutex_t orders_mutex;

void init_order_handler()
{
	orders = rb_create(comparison_s, free_key, free_data);
	pthread_mutex_init(&orders_mutex, NULL);
}

void insert_order(const char *uuid, const char *market, double volume,
				  double price, const char *side, const char *status)
{
	pthread_mutex_lock(orders_mutex);

	char *key = strdup(uuid);
	if (!key) {
		pr_err("strdup failed: %s:%d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
	Order *data;
	MALLOC(data, sizeof(data));
	strncpy(data->market, market, sizeof(market));
	data->volume = volume;
	data->price = price;
	strncpy(data->side, side, strlen(side));
	strncpy(data->status, status, strlen(status));
	rb_insert(orders, (void *)key, (void *)data);

	pthread_mutex_unlock(orders_mutex);
}

void find_order(const char *uuid, Order **o)
{
	pthread_mutex_lock(orders_mutex);

	*o = rb_find(orders->root, orders->compare, (void *)uuid);

	pthread_mutex_unlock(orders_mutex);
}

void remove_order(const char *uuid)
{
	pthread_mutex_lock(orders_mutex);

	rb_remove(orders, (void *)uuid);

	pthread_mutex_unlock(orders_mutex);
}

void update_order_status(const char *uuid, const char *status)
{
	pthread_mutex_lock(orders_mutex);

	Order *o = NULL;
	find_order(uuid, &o);
	strcpy(o->status, status);

	pthread_mutex_unlock(orders_mutex);
}

void destroy_order_handler()
{
	pthread_mutex_lock(orders_mutex);

	rb_remove_tree(orders);

	pthread_mutex_unlock(orders_mutex);

	pthread_mutex_destroy(&orders_mutex);
}

void print_order(RB_node *root)
{
	if (!root) return;

	print_order(root->link[LEFT]);
	Order *order = (Order *)root->data;
	printf("%s, %s, %.2f, %.2f, %s, %s\n",
			(char *)root->key, order->market, order->volume, order->price,
			order->side, order->status);
	print_order(root->link[RIGHT]);
}
