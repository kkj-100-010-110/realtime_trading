#include "json_handler.h"

char *ticker_json = NULL;
char *orderbook_json = NULL;
char *ticker_orderbook_json = NULL;
char *ticker_orderbook_trade = NULL;

void clear_extern_json()
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

bool set_json_config()
{
	FILE *file = fopen("./config/markets.json", "r");
	if (!file) {
		pr_err("fopen() failed.");
		return false;
	}
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *buffer = (char *)malloc(length + 1);
	if (!buffer) {
		pr_err("malloc() failed.");
		fclose(file);
		return false;
	}

	size_t read_size = fread(buffer, 1, length, file);
	fclose(file);
	if (read_size != length) {
		pr_err("fread() failed.");
		free(buffer);
		return false;
	}
	buffer[length] = '\0';

	json_t *root;
	json_error_t error;
	root = json_loads(buffer, 0, &error);
	free(buffer);
	if (!root) {
		pr_err("json_loads() failed. %s", error.text);
		return false;
	}

	json_t *markets = json_object_get(root, "markets");
	if (!json_is_array(markets)) {
		pr_err("json_is_array() failed.");
		json_decref(root);
		return false;
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
		pr_err("json_dumps() failed");
		clear_extern_json(); // Free allocated strings
		return false;
	}

	// Cleanup JSON objects
	json_decref(ticker_request);
	json_decref(orderbook_request);
	json_decref(ticker_orderbook_request);
	json_decref(ticker_orderbook_trade_request);
	json_decref(markets);
	json_decref(root);
	return true;
}

/* WEBSOCKET RESPONSE JSON DATA PARSE BEGIN */
void parse_websocket_data(const char *data, size_t len)
{
	json_t *root;
	json_error_t error;
	root = json_loadb(data, len, 0, &error);
	if (!root) {
		LOG_ERR("json_loadb() failed: %s", error.text);
		pr_err("json_loadb() failed: %s", error.text);
		return;
	}

	const char *type_str = json_string_value(json_object_get(root, "type"));
	if (!type_str) {
		LOG_ERR("json_string_value() failed.");
		pr_err("json_string_value() failed.");
		json_decref(root);
		return;
	}

    if (strcmp(type_str, "ticker") == 0) {
        parse_ticker_json(root);
    } else if (strcmp(type_str, "orderbook") == 0) {
        parse_orderbook_json(root);
    } else if (strcmp(type_str, "trade") == 0) {
        parse_trade_json(root);
	}
	
	// reference count to zero, the value is destroyed and no longer used.
	json_decref(root);
}

void parse_ticker_json(json_t *root)
{
	// get info that you need from json data
    const char *code = json_string_value(json_object_get(root, "code"));
	int idx = get_code_index(code);
	if (idx == -1) {
		LOG_ERR("unknown code: %s", code);
		pr_err("unknown code: %s", code);
		return;
	}
	tickers[idx].code = codes[idx];
	tickers[idx].trade_price = json_real_value(json_object_get(root, "trade_price"));
	strcpy(tickers[idx].change, json_string_value(json_object_get(root, "change")));
	tickers[idx].signed_change_price = json_real_value(json_object_get(root, "signed_change_price"));
	tickers[idx].signed_change_rate = json_real_value(json_object_get(root, "signed_change_rate"));
	tickers[idx].high_price = json_real_value(json_object_get(root, "high_price"));
	tickers[idx].low_price = json_real_value(json_object_get(root, "low_price"));
    tickers[idx].opening_price = json_real_value(json_object_get(root, "opening_price"));
    tickers[idx].prev_closing_price = json_real_value(json_object_get(root, "prev_closing_price"));
    tickers[idx].trade_volume = json_real_value(json_object_get(root, "trade_volume"));
    tickers[idx].acc_trade_volume_24h = json_real_value(json_object_get(root, "acc_trade_volume_24h"));
    tickers[idx].acc_trade_price_24h = json_real_value(json_object_get(root, "acc_trade_price_24h"));
	tickers[idx].highest_52_week_price = json_real_value(json_object_get(root, "highest_52_week_price"));
	tickers[idx].lowest_52_week_price = json_real_value(json_object_get(root, "lowest_52_week_price"));
	strcpy(tickers[idx].market_state, json_string_value(json_object_get(root, "market_state")));
}

void parse_orderbook_json(json_t *root)
{
	// get info that you need from json data
    const char *code = json_string_value(json_object_get(root, "code"));
	int idx = get_code_index(code);
	if (idx == -1) {
		LOG_ERR("unknown code: %s");
		pr_err("unknown code: %s", code);
		return;
	}
	orderbooks[idx].code = codes[idx];
    orderbooks[idx].timestamp = json_integer_value(json_object_get(root, "timestamp"));
    orderbooks[idx].total_bid_size = json_real_value(json_object_get(root, "total_bid_size"));
    orderbooks[idx].total_ask_size = json_real_value(json_object_get(root, "total_ask_size"));

    // orderbook units
    json_t *orderbook_units = json_object_get(root, "orderbook_units");
	if (json_is_array(orderbook_units)) {
		json_t *first_unit = json_array_get(orderbook_units, 0);
		orderbooks[idx].best_bid_price = json_real_value(json_object_get(first_unit, "bid_price"));
		orderbooks[idx].best_bid_size = json_real_value(json_object_get(first_unit, "bid_size"));
		orderbooks[idx].best_ask_price = json_real_value(json_object_get(first_unit, "ask_price"));
		orderbooks[idx].best_ask_size = json_real_value(json_object_get(first_unit, "ask_size"));
		orderbooks[idx].spread = orderbooks[idx].best_ask_price - orderbooks[idx].best_bid_price;
		orderbooks[idx].bid_ask_ratio = orderbooks[idx].best_bid_size / orderbooks[idx].best_ask_size;
	}

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
            pr_out("Unit %zu: Ask Price=%.2f, Ask Size=%.2f, Bid Price=%.2f, Bid Size=%.2f\n",
                   index, ask_price, ask_size, bid_price, bid_size);
        }
    }

    pr_out("Code: %s", code);
    pr_out("Timestamp: %lld", (long long)timestamp);
    pr_out("Total Ask Size: %.2f", total_ask_size);
    pr_out("Total Bid Size: %.2f", total_bid_size);
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
			long trade_timestamp = json_integer_value(json_object_get(root, "trade_timestamp"));
			strcpy(txn->date, trade_date);
			strcpy(txn->time, trade_time);
			strcpy(txn->code, code);
			strcpy(txn->side, ask_bid);
			txn->price = trade_price;
			txn->volume = trade_volume;

			pr_trade("%s, %s Code=%s, Side=%s, Price=%.8f, Volume=%.8f",
					trade_date, trade_time, code, trade_price, trade_volume, ask_bid);

			OrderStatus *os = create_order_status(code, 1, trade_timestamp, 0,
					0, 5, NULL);
			enqueue_task(get_closed_orders_status_task, (void *)os); 
			enqueue_task(save_txn_task, (void *)txn);
		}
	}
}
/* WEBSOCKET RESPONSE JSON DATA PARSE END */

/* REST API RESPONSE JSON DATA PARSE BEGIN */
void parse_account_json(const char *data)
{
	json_t *root;
	json_error_t error;

	root = json_loads(data, 0, &error);
	if (!root) {
		LOG_ERR("json_loads() failed: %s", error.text);
		pr_err("json_loads() failed: %s", error.text);
		return;
	}

	if (!json_is_array(root)) {
		LOG_ERR("json_is_array() failed.");
		pr_err("json_is_array() failed.");
		json_decref(root);
		return;
	}

	size_t index;
	json_t *value;
	json_array_foreach(root, index, value) {
		json_t *currency = json_object_get(value, "currency");
		json_t *balance = json_object_get(value, "balance");
		json_t *locked = json_object_get(value, "locked");
		if (json_is_string(currency) && json_is_string(balance) && json_is_string(locked)) {
			const char *c = json_string_value(currency);
			const char *b = json_string_value(balance);
			const char *l = json_string_value(locked);
			int idx = get_index(c);
			strcpy(account[idx].currency, c);
			account[idx].balance = atof(b);
			account[idx].locked = atof(l);
			pr_out("Currency: %s | Balance: %.2f | Locked: %.2f\n",
					account[idx].currency, account[idx].balance, account[idx].locked);
		}
	}
	json_decref(root);
}

void parse_place_order_response(const char *data)
{
	json_error_t error;
    json_t *root = json_loads(data, 0, &error);
    if (!root) {
		LOG_ERR("json_loads() failed: %s", error.text);
		pr_err("json_loads() failed: %s", error.text);
        return;
    }

    const char *uuid = json_string_value(json_object_get(root, "uuid"));
    const char *market = json_string_value(json_object_get(root, "market"));
    const char *side = json_string_value(json_object_get(root, "side"));
    double volume = json_real_value(json_object_get(root, "volume"));
    double price = json_real_value(json_object_get(root, "price"));
    const char *state = json_string_value(json_object_get(root, "state"));

	// account update

	// order update
	insert_order(uuid, market, volume, price, side, state);

	pr_order("Market: %s, Side: %s, Volume: %.8f, Price: %.8f, State: %s, UUID: %s",
			 market, side, volume, price, state, uuid);

    json_decref(root);
}

void parse_market_order_response(const char *data)
{
	json_error_t error;
    json_t *root = json_loads(data, 0, &error);
    if (!root) {
		LOG_ERR("json_loads() failed: %s", error.text);
		pr_err("json_loads() failed: %s", error.text);
        return;
    }

    const char *uuid = json_string_value(json_object_get(root, "uuid"));
    const char *market = json_string_value(json_object_get(root, "market"));
    const char *side = json_string_value(json_object_get(root, "side"));
    double volume = json_real_value(json_object_get(root, "volume"));
    double price = json_real_value(json_object_get(root, "price"));
    const char *state = json_string_value(json_object_get(root, "state"));

	// account update

	// order update
	insert_order(uuid, market, volume, price, side, state);

	pr_order("Market: %s, Side: %s, Volume: %.8f, Price: %.8f, State: %s, UUID: %s",
			 market, side, volume, price, state, uuid);

    json_decref(root);
}

void parse_cancel_order_response_json(const char *data)
{
	json_t *root;
    json_error_t error;

    root = json_loads(data, 0, &error);
    if (!root) {
		LOG_ERR("json_loads() failed: %s", error.text);
		pr_err("json_loads() failed: %s", error.text);
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

	if (strcmp(state, "wait") == 0) {
		update_order_status(uuid, state);
		enqueue_retry_task(uuid, INITIAL_BACKOFF_MS);
	} else if (strcmp(state, "done") == 0 || strcmp(state, "cancel") == 0) {
		pr_cancel("uuid: %s, state: %s", uuid, state);
		remove_order(uuid);
	}

    json_decref(root);
}

void parse_get_order_status(const char *data)
{
	json_t *root;
	json_error_t error;

	root = json_loads(data, 0, &error);
    if (!root) {
		LOG_ERR("json_loads() failed: %s", error.text);
		pr_err("json_loads() failed: %s", error.text);
        return;
    }

	const char *uuid = json_string_value(json_object_get(root, "uuid"));
    const char *state = json_string_value(json_object_get(root, "state"));

	pr_out("order: %s, state: %s", uuid, state);
	update_order_status(uuid, state);
	if (strcmp(state, "done") == 0 || strcmp(state, "cancel") == 0) {
		pr_cancel("uuid: %s, state: %s", uuid, state);
		remove_order(uuid);
	}
	json_decref(root);
}

void parse_get_open_orders_status(const char * data)
{
	json_t *root;
	json_error_t error;

	root = json_loads(data, 0, &error);
	if (!root) {
		LOG_ERR("json_loads() failed: %s", error.text);
		pr_err("json_loads() failed: %s", error.text);
		return;
	}

	size_t index;
	json_t *value;
	json_array_foreach(root, index, value) {
		const char *uuid = json_string_value(json_object_get(value, "uuid"));
		const char *state = json_string_value(json_object_get(value, "state"));
		const char *market = json_string_value(json_object_get(value, "market"));
		const char *side = json_string_value(json_object_get(value, "side"));
		const char *ord_type = json_string_value(json_object_get(value, "ord_type"));
		double price = json_real_value(json_object_get(value, "price"));
		double volume = json_real_value(json_object_get(value, "volume"));
		double executed_volume = json_real_value(json_object_get(value, "executed_volume"));

		pr_out("Open Order: uuid=%s, state=%s, market=%s, side=%s, type=%s, price=%.2f, volume=%.8f, executed_volume=%.8f",
				uuid, state, market, side, ord_type, price, volume, executed_volume);
	}
	json_decref(root);
}

void parse_get_closed_orders_status(const char *data)
{
	json_t *root;
	json_error_t error;

	root = json_loads(data, 0, &error);
    if (!root) {
		LOG_ERR("json_loads() failed: %s", error.text);
		pr_err("json_loads() failed: %s", error.text);
        return;
    }

	size_t index;
	json_t *value;
	json_array_foreach(root, index, value) {
		const char *uuid = json_string_value(json_object_get(value, "uuid"));
		const char *state = json_string_value(json_object_get(value, "state"));
		const char *market = json_string_value(json_object_get(value, "market"));
		const char *side = json_string_value(json_object_get(value, "side"));
		const char *ord_type = json_string_value(json_object_get(value, "ord_type"));
		double price = json_real_value(json_object_get(value, "price"));
		double volume = json_real_value(json_object_get(value, "volume"));
		double executed_volume = json_real_value(json_object_get(value, "executed_volume"));
		remove_order(uuid);
		pr_out("Closed Order: uuid=%s, state=%s, market=%s, side=%s, type=%s, price=%.2f, volume=%.8f, executed_volume=%.8f",
				uuid, state, market, side, ord_type, price, volume, executed_volume);
	}
	json_decref(root);
}

void parse_cancel_by_bulk_response(const char *data)
{
    json_error_t error;
    json_t *root = json_loads(data, 0, &error);

    if (!root) {
		LOG_ERR("json_loads() failed: %s", error.text);
		pr_err("json_loads() failed: %s", error.text);
        return;
    }

    // success
    json_t *success = json_object_get(root, "success");
    if (success) {
        int success_count = json_integer_value(json_object_get(success, "count"));
        json_t *success_orders = json_object_get(success, "orders");
        if (json_is_array(success_orders)) {
            size_t index;
            json_t *value;
            json_array_foreach(success_orders, index, value) {
                const char *uuid = json_string_value(json_object_get(value, "uuid"));

				// test
                const char *market = json_string_value(json_object_get(value, "market"));
                pr_cancel("Successfully canceled order: uuid=%s, market=%s", uuid, market);

				remove_order(uuid);

				//test
				print_order(orders->root);
            }
        }
    }

    // failed
    json_t *failed = json_object_get(root, "failed");
    if (failed) {
        int failed_count = json_integer_value(json_object_get(failed, "count"));
        json_t *failed_orders = json_object_get(failed, "orders");
        if (json_is_array(failed_orders)) {
            size_t index;
            json_t *value;
			json_array_foreach(failed_orders, index, value) {
				const char *uuid = json_string_value(json_object_get(value, "uuid"));
				const char *market = json_string_value(json_object_get(value, "market"));
				pr_err("failed to cancel order: uuid=%s, market=%s", uuid, market);
				LOG_ERR("failed to cancel order: uuid=%s, market=%s", uuid, market);
				enqueue_retry_task(uuid, INITIAL_BACKOFF_MS);
			}
        }
    }

    json_decref(root);
}
/* REST API RESPONSE JSON DATA PARSE END */
