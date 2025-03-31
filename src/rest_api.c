#include "rest_api.h"

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total_size = size * nmemb;
	char *response = (char *)userdata;
	strncat(response, (char *)ptr, total_size);
    return total_size;
}

/* task function format for thread queue */
void get_account_info_task(void *arg)
{
	(void)arg;
	get_account_info();
}

void place_order_task(void *arg)
{
	order_t *o = (order_t *)arg;
	if (strcmp(o->side, "ask") == 0) {
		place_sell_order(o->market, o->volume, o->price);
	} else if (strcmp(o->side, "bid") == 0) {
		place_buy_order(o->market, o->volume, o->price);
	}
	FREE(arg);
}

void market_price_order_task(void *arg)
{
	order_t *o = (order_t *)arg;
	market_order(o->market, o->side, o->volume);
	FREE(arg);
}

void get_order_status_task(void *arg)
{
	char *uuid = (char *)arg;
	get_order_status(uuid);
	FREE(arg);
}

void get_closed_orders_status_task(void *arg)
{
	order_status_t *os = (order_status_t *)arg;
	get_closed_orders_status(os->market, g_closed_states, os->states_count,
			os->start_time, os->end_time, os->limit, os->order_by);
	FREE(arg);
}

void cancel_order_task(void *arg)
{
	char *uuid = (char *)arg;
	cancel_order(uuid);
	FREE(arg);
}

void cancel_by_bulk_task(void *arg)
{
	cancel_option_t *c = (cancel_option_t *)arg;
	cancel_by_bulk(c->side, c->pairs, c->excluded_pairs, c->quote_currencies,
			c->count, c->order_by);
	FREE(arg);
}


/* functions */
void get_account_info()
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
	char *response = NULL;

    generate_jwt(jwt, sizeof(jwt)); // JWT

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

    // init & configure CURL
    curl = curl_easy_init();
    if (curl) {
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, API_URL_ACCOUNTS);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

		// print debugging info
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
			pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
		} else {
			parse_account_json(response);
		}
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
	}
	FREE(response);
}

void place_order(const char *market, const char *side, double volume, double price)
{
	CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
    char post_data[POST_BUFFER_SIZE];
    char *response = NULL;

    generate_jwt(jwt, sizeof(jwt));

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    // content-type header
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // JSON data
	snprintf(post_data, sizeof(post_data),
			"{\"market\": \"%s\", \"%s\": \"bid\", \"volume\": \"%.8f\", \"price\": \"%.8f\", \"ord_type\": \"limit\"}",
			market, side, volume, price);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

    // init & configure CURL
    curl = curl_easy_init();
    if (curl) {
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, API_URL_ORDERS);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        // proceed request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
			pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
		} else {
			parse_place_order_response(response);
		}

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
	}
	FREE(response);
}

void place_buy_order(const char *market, double volume, double price)
{
	place_order(market, "bid", volume, price);
}

void place_sell_order(const char *market, double volume, double price)
{
	place_order(market, "ask", volume, price);
}

void market_order(const char *market, const char *side, double volume)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
    char post_data[POST_BUFFER_SIZE];
    char *response = NULL;

    generate_jwt(jwt, sizeof(jwt));

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    // content-type header
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // JSON data
    if (strcmp(side, "bid") == 0) {
        snprintf(post_data, sizeof(post_data),
                 "{\"market\":\"%s\",\"side\":\"%s\",\"ord_type\":\"price\",\"price\":\"%.0f\"}",
                 market, side, volume);
    } else if (strcmp(side, "ask") == 0) {
        snprintf(post_data, sizeof(post_data),
                 "{\"market\":\"%s\",\"side\":\"%s\",\"ord_type\":\"market\",\"volume\":\"%.8f\"}",
                 market, side, volume);
    } else {
        pr_err("Invalid order side: %s", side);
        LOG_ERR("Invalid order side: %s", side);
        return;
    }

	MALLOC(response, RESPONSE_BUFFER_SIZE);

    curl = curl_easy_init();
    if (curl) {
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, API_URL_ORDERS);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
			pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else {
            printf("Market Order Response: %s\n", response);
			parse_market_order_response(response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
	}
	FREE(response);
}

void cancel_order(const char *uuid)
{
	CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
    char url[URL_BUFFER_SIZE];
    char *response = NULL;

    generate_jwt(jwt, sizeof(jwt));

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    // content-type header
    headers = curl_slist_append(headers, "Content-Type: application/json");

	// url + uuid
    snprintf(url, sizeof(url), "%s?uuid=%s", API_URL_ORDER, uuid);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

    curl = curl_easy_init();
    if (curl) {
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
			pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else {
			parse_cancel_order_response_json(response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
	}
	FREE(response);
}

void get_order_status(const char *uuid)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
    char url[URL_BUFFER_SIZE];
    char *response = NULL;

    generate_jwt(jwt, sizeof(jwt));

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    snprintf(url, sizeof(url), API_URL_ORDER "?uuid=%s", uuid);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

    curl = curl_easy_init();
    if (curl) {
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
			pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else {
			parse_get_order_status(response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
	FREE(response);
}

int check_order_status(const char *uuid)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
    char url[URL_BUFFER_SIZE];
    char *response = NULL;

    generate_jwt(jwt, sizeof(jwt));

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    snprintf(url, sizeof(url), API_URL_ORDER "?uuid=%s", uuid);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

    curl = curl_easy_init();
    if (curl) {
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
		} else {
			json_t *root;
			json_error_t error;

			root = json_loads(response, 0, &error);
			if (!root) {
				pr_err("json_loads() failed: %s", error.text);
				LOG_ERR("json_loads() failed: %s", error.text);
				return FAILED;
			}
			const char *state = json_string_value(json_object_get(root, "state"));

			if (strcmp(state, "wait") == 0) { // it needs to be improved when the program is bigger
				return PENDING;
			} else if (strcmp(state, "done") == 0 || strcmp(state, "cancel") == 0) {
				pr_cancel("uuid: %s, state: %s", uuid, state);
				remove_order(uuid);
			}
			json_decref(root);
		}
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}
	FREE(response);
	return COMPLETED;
}

void get_open_orders_status(const char *market, const char **states,
		size_t states_count, int page, int limit, const char *order_by)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
    char url[URL_BUFFER_SIZE];
    char *response = NULL;
    json_t *params;

    generate_jwt(jwt, sizeof(jwt));

    // authorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    // request parameters
    params = json_object();
    if (market) json_object_set_new(params, "market", json_string(market));

    if (states && states_count > 0) {
        json_t *states_array = json_array();
        for (size_t i = 0; i < states_count; i++) {
            json_array_append_new(states_array, json_string(states[i]));
        }
        json_object_set_new(params, "states[]", states_array);
    }
    if (page > 0) json_object_set_new(params, "page", json_integer(page));
    if (limit > 0) {
        limit = (limit > 100) ? 100 : limit;
        json_object_set_new(params, "limit", json_integer(limit));
    }
    if (order_by != NULL) json_object_set_new(params, "order_by", json_string(order_by));

    // URL + request parameters
    char *params_str = json_dumps(params, JSON_COMPACT);
    snprintf(url, sizeof(url), API_URL_ORDERS_OPEN "?%s", params_str);
    free(params_str);
    json_decref(params);

    MALLOC(response, RESPONSE_BUFFER_SIZE);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
			pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else {
			parse_get_open_orders_status(response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
	FREE(response);
}

void get_closed_orders_status(const char *market, const char **states,
		size_t states_count, long start_time, long end_time,
		int limit, const char *order_by)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
    char url[URL_BUFFER_SIZE];
	char *response = NULL;
	json_t *params;

    generate_jwt(jwt, sizeof(jwt));

    // authorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    // request parameters
    params = json_object();
	if (market) json_object_set_new(params, "market", json_string(market));

	if (states && states_count > 0) {
		json_t *states_array = json_array();
		for (size_t i = 0; i < states_count; i++) {
			json_array_append_new(states_array, json_string(states[i]));
		}
		json_object_set_new(params, "states[]", states_array);
	} else {
		json_t *default_states = json_array();
		json_array_append_new(default_states, json_string("done"));
		json_array_append_new(default_states, json_string("cancel"));
		json_object_set_new(params, "states[]", default_states);
	}

	// ISO-8601 or Timestamp, using timestamp here
	if (start_time) json_object_set_new(params, "start_time", json_integer(start_time));
    if (end_time) json_object_set_new(params, "end_time", json_integer(end_time));

    if (limit > 0) {
        limit = (limit > 1000) ? 1000 : limit;
        json_object_set_new(params, "limit", json_integer(limit));
    }
    if (order_by) json_object_set_new(params, "order_by", json_string(order_by));

    // URL + request parameters
    char *params_str = json_dumps(params, JSON_COMPACT);
    snprintf(url, sizeof(url), API_URL_ORDERS_CLOSED "?%s", params_str);
    free(params_str);
    json_decref(params);

    MALLOC(response, RESPONSE_BUFFER_SIZE);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
			pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
		} else {
			parse_get_closed_orders_status(response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    FREE(response);
}

void cancel_by_bulk(const char *cancel_side, const char *pairs,
                    const char *excluded_pairs, const char *quote_currencies,
                    int count, const char *order_by)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
    char url[URL_BUFFER_SIZE];
    char *response = NULL;
    json_t *params;
    char *params_str;

    // cannot take pairs and quote_currencies together
    if (pairs != NULL && quote_currencies != NULL) {
		pr_err("cancel_by_bulk() failed.");
		LOG_ERR("cancel_by_bulk() failed.");
        return;
    }

    generate_jwt(jwt, sizeof(jwt));

    // authorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    // create query parameter
    params = json_object();
    if (cancel_side != NULL) { // cancel_side
        json_object_set_new(params, "cancel_side", json_string(cancel_side));
    } else {
        json_object_set_new(params, "cancel_side", json_string("all")); // 기본값
    }
    if (pairs != NULL) { // pairs or quote_currencies
        json_object_set_new(params, "pairs", json_string(pairs));
    } else if (quote_currencies != NULL) {
        json_object_set_new(params, "quote_currencies", json_string(quote_currencies));
    }
    if (excluded_pairs != NULL) { // excluded_pairs
        json_object_set_new(params, "excluded_pairs", json_string(excluded_pairs));
    }
    if (count > 0) { // count
        json_object_set_new(params, "count", json_integer(count));
    } else {
        json_object_set_new(params, "count", json_integer(20)); // default
    }
    if (order_by != NULL) { // order_by
        json_object_set_new(params, "order_by", json_string(order_by));
    } else {
        json_object_set_new(params, "order_by", json_string("desc")); // default
    }

    params_str = json_dumps(params, JSON_COMPACT);
    json_decref(params);

    // URL
    snprintf(url, sizeof(url), API_URL_ORDERS_OPEN "?%s", params_str);
    free(params_str);

    MALLOC(response, RESPONSE_BUFFER_SIZE);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            pr_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            LOG_ERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else {
            parse_cancel_by_bulk_response(response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    FREE(response);
}
