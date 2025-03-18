#include "../include/common.h"
#include <jansson.h>
#include "order_handler.h"
#include "symbol_handler.h"

typedef struct {
	char market[16];
    double volume;
    double price;
} order;

int main()
{
	FILE *file = fopen("test.json", "r");
	if (!file) {
		pr_err("fopen failed.");
		exit(EXIT_FAILURE);
	}
	fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

	char *buffer = (char *)malloc(length + 1);
    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

    json_t *root;
    json_error_t error;
    root = json_loads(buffer, 0, &error);
    free(buffer);
	if (!root) {
		pr_err("json_loads() failed. %s", error.text);
        exit(EXIT_FAILURE);
    }

    json_t *transaction = json_object_get(root, "data");
    if (!json_is_array(transaction)) {
        pr_err("transaction not array");
        json_decref(root);
        exit(EXIT_FAILURE);
    }

	RB_tree *t;
	t = rb_create(comparison_s, free_key, free_data);

    if (json_is_array(transaction)) {
        size_t index;
        json_t *unit;
        json_array_foreach(transaction, index, unit) {
            const char *uuid = json_string_value(json_object_get(unit, "uuid"));
            const char *market = json_string_value(json_object_get(unit, "market"));
            double trade_price = json_real_value(json_object_get(unit,
						"trade_price"));
            double trade_volume = json_real_value(json_object_get(unit,
						"trade_volume"));
            long timestamp = json_integer_value(json_object_get(unit, "timestamp"));

            printf("uuid: %s, market: %s, trade_price=%.2f, trade_volume=%.2f, timestamp: %ld\n",
                   uuid, market, trade_price, trade_volume, timestamp);
			char *key = strdup(uuid);
			order *data;
			MALLOC(data, sizeof(Order));
			strncpy(data->market, market, strlen(market));
			data->price = trade_price;
			data->volume = trade_volume;
			rb_insert(t, (void *)key, (void *)data);
        }
    }
	rb_print(t, 6);

	return 0;
}
