#include "json_handler.h"

char *ticker_json = NULL;
char *orderbook_json = NULL;
char *ticker_orderbook_json = NULL;
char *ticker_orderbook_trade = NULL;

void clean_extern_json()
{
	if (ticker_json) {
		free(ticker_json);
		ticker_json = NULL;
	}
	if (orderbook_json) {
		free(orderbook_json);
		orderbook_json = NULL;
	}
	if (ticker_orderbook_json) {
		free(ticker_orderbook_json);
		ticker_orderbook_json = NULL;
	}
	if (ticker_orderbook_trade) {
		free(ticker_orderbook_trade);
		ticker_orderbook_trade = NULL;
	}
}

void set_json_config()
{
	FILE *file = fopen("./config/markets.json", "r");
	if (!file) {
		pr_err("fopen failed.");
		exit(EXIT_FAILURE);
	}
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *buffer = (char *)malloc(length + 1);
	if (!buffer) {
		pr_err("malloc failed.");
		fclose(file);
		exit(EXIT_FAILURE);
	}

	size_t read_size = fread(buffer, 1, length, file);
	fclose(file);

	if (read_size != length) {
		pr_err("fread failed.");
		free(buffer);
		exit(EXIT_FAILURE);
	}
	buffer[length] = '\0';

	json_t *root;
	json_error_t error;
	root = json_loads(buffer, 0, &error);
	free(buffer);
	if (!root) {
		pr_err("json_loads() failed. %s", error.text);
		exit(EXIT_FAILURE);
	}

	json_t *markets = json_object_get(root, "markets");
	if (!json_is_array(markets)) {
		pr_err("markets not array");
		json_decref(root);
		exit(EXIT_FAILURE);
	}
	json_incref(markets);  // Increase ref count since we're using it multiple times

	// Create JSON requests
	json_t *ticker_request = json_array();
	json_array_append_new(ticker_request,
			json_pack("{s:s}", "ticket", "ticker_sub"));
	json_array_append_new(ticker_request,
		json_pack("{s:s, s:o}", "type", "ticker", "codes", markets));
	json_incref(markets);

	json_t *orderbook_request = json_array();
	json_array_append_new(orderbook_request,
			json_pack("{s:s}", "ticket", "orderbook_sub"));
	json_array_append_new(orderbook_request,
			json_pack("{s:s, s:o}", "type", "orderbook", "codes", markets));
	json_incref(markets);

	json_t *ticker_orderbook_request = json_array();
	json_array_append_new(ticker_orderbook_request,
			json_pack("{s:s}", "ticket", "ticker_orderbook_sub"));
	json_array_append_new(ticker_orderbook_request,
			json_pack("{s:s, s:o}", "type", "ticker", "codes", markets));
	json_incref(markets);
	json_array_append_new(ticker_orderbook_request,
			json_pack("{s:s, s:o}", "type", "orderbook", "codes", markets));
	json_incref(markets);

	json_t *ticker_orderbook_trade_request = json_array();
	json_array_append_new(ticker_orderbook_trade_request,
			json_pack("{s:s}", "ticket", "ticker_orderbook_trade_sub"));
	json_array_append_new(ticker_orderbook_trade_request,
			json_pack("{s:s, s:o}", "type", "ticker", "codes", markets));
	json_incref(markets);
	json_array_append_new(ticker_orderbook_trade_request,
			json_pack("{s:s, s:o}", "type", "orderbook", "codes", markets));
	json_incref(markets);
	json_array_append_new(ticker_orderbook_trade_request,
			json_pack("{s:s, s:o}", "type", "trade", "codes", markets));
	json_incref(markets);

	// Convert JSON objects to strings
	ticker_json = json_dumps(ticker_request, JSON_COMPACT);
	orderbook_json = json_dumps(orderbook_request, JSON_COMPACT);
	ticker_orderbook_json = json_dumps(ticker_orderbook_request, JSON_COMPACT);
	ticker_orderbook_trade = json_dumps(ticker_orderbook_trade_request, JSON_COMPACT);

	if (!ticker_json || !orderbook_json || !ticker_orderbook_json || !ticker_orderbook_trade) {
		pr_err("json_dumps failed");
		clean_extern_json(); // Free allocated strings
		exit(EXIT_FAILURE);
	}

	// Cleanup JSON objects
	json_decref(ticker_request);
	json_decref(orderbook_request);
	json_decref(ticker_orderbook_request);
	json_decref(ticker_orderbook_trade_request);
	json_decref(markets);
	json_decref(root);
}

/* WEBSOCKET JSON DATA */
void parse_websocket_data(const char *data, size_t len)
{
	json_t *root;
	json_error_t error;
	root = json_loadb(data, len, 0, &error);
	if (!root) {
		pr_err("json_loadb() failed: %s", error.text);
		return;
	}

	const char *type_str = json_string_value(json_object_get(root, "type"));
	if (!type_str) {
		pr_err("failed to get type field.");
		json_decref(root);
		return;
	}

    if (strcmp(type_str, "ticker") == 0) {
        parse_ticker_json(root);
    } else if (strcmp(type_str, "orderbook") == 0) {
        parse_orderbook_json(root);
    } else if (strcmp(type_str, "trade") == 0) {
        parse_trade_json(root);
    } else {
        pr_err("Unknown type: %s", type_str);
    }
	
	// reference count to zero, the value is destroyed and no longer used.
	json_decref(root);
}

void parse_ticker_json(json_t *root)
{
	// get info that you need from json data
    const char *code = json_string_value(json_object_get(root, "code"));
    double opening_price = json_real_value(json_object_get(root, "opening_price"));
    double high_price = json_real_value(json_object_get(root, "high_price"));
    double low_price = json_real_value(json_object_get(root, "low_price"));
    double trade_price = json_real_value(json_object_get(root, "trade_price"));
    double prev_closing_price = json_real_value(json_object_get(root, "prev_closing_price"));
    double acc_trade_volume = json_real_value(json_object_get(root, "acc_trade_volume"));
    double acc_trade_price = json_real_value(json_object_get(root, "acc_trade_price"));
    json_int_t timestamp = json_integer_value(json_object_get(root, "timestamp"));

#if PRINT
    printf("Code: %s\n", code);
    printf("Opening Price: %.2f\n", opening_price);
    printf("High Price: %.2f\n", high_price);
    printf("Low Price: %.2f\n", low_price);
    printf("Trade Price: %.2f\n", trade_price);
    printf("Previous Closing Price: %.2f\n", prev_closing_price);
    printf("Accumulated Trade Volume: %.2f\n", acc_trade_volume);
    printf("Accumulated Trade Price: %.2f\n", acc_trade_price);
    printf("Timestamp: %lld\n", (long long)timestamp);
#endif//PRINT
}

void parse_orderbook_json(json_t *root)
{
	// get info that you need from json data
    const char *code = json_string_value(json_object_get(root, "code"));
    json_int_t timestamp = json_integer_value(json_object_get(root, "timestamp"));
    double total_ask_size = json_real_value(json_object_get(root, "total_ask_size"));
    double total_bid_size = json_real_value(json_object_get(root, "total_bid_size"));

#if PRINT
    // orderbook unit
    json_t *orderbook_units = json_object_get(root, "orderbook_units");
    if (json_is_array(orderbook_units)) {
        size_t index;
        json_t *unit;
        json_array_foreach(orderbook_units, index, unit) {
            double ask_price = json_real_value(json_object_get(unit, "ask_price"));
            double bid_price = json_real_value(json_object_get(unit, "bid_price"));
            double ask_size = json_real_value(json_object_get(unit, "ask_size"));
            double bid_size = json_real_value(json_object_get(unit, "bid_size"));

            // ask & bit price unit
            printf("Unit %zu: Ask Price=%.2f, Ask Size=%.2f, Bid Price=%.2f, Bid Size=%.2f\n",
                   index, ask_price, ask_size, bid_price, bid_size);
        }
    }

    printf("Code: %s\n", code);
    printf("Timestamp: %lld\n", (long long)timestamp);
    printf("Total Ask Size: %.2f\n", total_ask_size);
    printf("Total Bid Size: %.2f\n", total_bid_size);
#endif//PRINT
}

void parse_trade_json(json_t *root)
{
	if (json_is_array(root)) {
		size_t index;
		json_t *value;
		json_array_foreach(root, index, value) {
			Transaction *txn;
			MALLOC(txn, sizeof(Transaction));
			const char *trade_date = json_string_value(json_object_get(root, "trade_date"));
			const char *trade_time = json_string_value(json_object_get(root, "trade_date"));
			const char *code = json_string_value(json_object_get(root, "code"));
			const char *ask_bid = json_string_value(json_object_get(root, "ask_bid"));
			double trade_price = json_real_value(json_object_get(root, "trade_price"));
			double trade_volume = json_real_value(json_object_get(root, "trade_volume"));
			strcpy(txn->date, trade_date);
			strcpy(txn->time, trade_time);
			strcpy(txn->code, code);
			strcpy(txn->side, ask_bid);
			txn->price = trade_price;
			txn->volume = trade_volume;
			enqueue_task(save_txn_task, (void *)txn);
			pr_trade("%s, %s Code=%s, Side=%s, Price=%.8f, Volume=%.8f",
					trade_date, trade_time, code, trade_price, trade_volume, ask_bid);
		}
	}

	// rest_api
}
/* WEBSOCKET JSON DATA */

/* REST API JSON DATA */
void parse_balance_json(const char *data)
{
	json_t *root;
	json_error_t error;

	root = json_loads(data, 0, &error);
	if (!root) {
		pr_err("json_loads() failed. %s", error.text);
		return;
	}

	if (!json_is_array(root)) {
		pr_err("Invalid JSON format.");
		json_decref(root);
		return;
	}

	size_t index;
	json_t *value;
	json_array_foreach(root, index, value) {
		json_t *currency = json_object_get(value, "currency");
		json_t *balance = json_object_get(value, "balance");
		json_t *locked = json_object_get(value, "locked");
		if (json_is_string(currency)
			&& json_is_string(balance)
			&& json_is_string(locked)) {
			pr_out("Currency: %s | Balance: %s | Locked: %s\n",
					json_string_value(currency),
					json_string_value(balance),
					json_string_value(locked));
		}
	}
	json_decref(root);
}

void parse_order_response_json(const char *data)
{
	json_error_t error;
    json_t *root = json_loads(data, 0, &error);
    if (!root) {
        pr_err("JSON parsing failed: %s", error.text);
        return;
    }

    const char *uuid = json_string_value(json_object_get(root, "uuid"));
    const char *market = json_string_value(json_object_get(root, "market"));
    const char *side = json_string_value(json_object_get(root, "side"));
    double volume = json_real_value(json_object_get(root, "volume"));
    double price = json_real_value(json_object_get(root, "price"));
    const char *state = json_string_value(json_object_get(root, "state"));

	insert_order(uuid, market, volume, price, side, state);

	pr_order("Market: %s, Side: %s, Volume: %.8f, Price: %.8f, State: %s, UUID: %s",
			 market, side, volume, price, state, uuid);

    json_decref(root);
}

void parse_cancel_response_json(const char *data)
{
	json_t *root;
    json_error_t error;

    root = json_loads(data, 0, &error);
    if (!root) {
        pr_err("JSON parsing failed: %s\n", error.line, error.text);
        return;
    }

    const char *uuid = json_string_value(json_object_get(root, "uuid"));
    const char *side = json_string_value(json_object_get(root, "side"));
    const char *state = json_string_value(json_object_get(root, "state"));
    const char *market = json_string_value(json_object_get(root, "market"));
    double price = json_real_value(json_object_get(root, "price"));
    double executed_volume = json_real_value(json_object_get(root, "executed_volume"));

	pr_order("Market: %s, Side: %s, Executed Volume: %.8f, Price: %.2f, State: %s, UUID: %s",
			 market, side, executed_volume, price, state, uuid);

    json_decref(root);
}


/* REST API JSON DATA */
