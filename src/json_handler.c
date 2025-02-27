#include "json_handler.h"

void parse_json(const char *data)
{
	json_t *root;
	json_error_t error;

	root = json_loads(data, 0, &error);
	if (!root) {
		pr_err("parse_json() failed. %s", error.text);
		return;
	}

	// get info that you need from json data
	json_t *code = json_object_get(root, "code");
	json_t *trade_price = json_object_get(root, "trade_price");
	json_t *acc_trade_volume_24h = json_object_get(root, "acc_trade_volume_24h");
	json_t *change_rate = json_object_get(root, "change_rate");
	json_t *highest_52_week_price = json_object_get(root, "highest_52_week_price");
	json_t *lowest_42_week_price = json_object_get(root, "lowest_42_week_price");
	
	if (json_is_string(code)) {
		pr_out("Code: %s", json_string_value(code));
	}
	if (json_is_number(trade_price)) {
		pr_out("Trade price: %.2f", json_number_value(trade_price));
	}
	if (json_is_number(acc_trade_volume_24h)) {
		pr_out("24H Trade volume: %.2f", json_number_value(acc_trade_volume_24h));
	}
	if (json_is_number(change_rate)) {
		pr_out("Change rate: %.6f", json_number_value(change_rate));
	}
	if (json_is_number(highest_52_week_price)) {
		pr_out("52 Week Highest price: %.2f", json_number_value(highest_52_week_price));
	}
	if (json_is_number(lowest_42_week_price)) {
		pr_out("52 Week Lowest price: %.2f", json_number_value(lowest_42_week_price));
	}

	/**/
	/**/

	// reference count to zero, the value is destroyed and no longer used.
	json_decref(root);
}
