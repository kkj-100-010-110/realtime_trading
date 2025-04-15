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
	place_order(o->market, o->side, o->volume, o->price, o->ord_type,
			o->time_in_force);
	FREE(arg);
}

void get_order_status_task(void *arg)
{
	char *uuid = (char *)arg;
	get_order_status(uuid);
	FREE(arg);
}

void get_open_orders_status_task(void *arg)
{
	order_status_t *os = (order_status_t *)arg;
	get_open_orders_status(os->market, g_open_states, os->states_count,
			os->page, os->limit, os->order_by);
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


/* request functions */
// GET
void get_account_info()
{
    CURL *curl = NULL;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[JWT_BUFFER_SIZE];
    char auth_header[HEADER_BUFFER_SIZE];
	char *response = NULL;
	long http_code = 0;

	// get curl handle from curl pool
	if (!get_curl_pool_handle(&curl)) {
		MY_LOG_ERR("get_curl_pool_handle() failed.");
		pr_err("get_curl_pool_handle() failed.");
		return;
	}

    generate_jwt(jwt, sizeof(jwt)); // JWT

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

	// response buffer
	MALLOC(response, RESPONSE_BUFFER_SIZE);

	// set curl options
	curl_easy_setopt(curl, CURLOPT_URL, API_URL_ACCOUNTS);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

	// proceed the request
	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
        // HTTP status code check
		if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code) == CURLE_OK) {
			MY_LOG_ERR("curl_easy_perform() failed: %s (res: %d, http code: %d)",
					curl_easy_strerror(res), res, http_code);
			pr_err("curl_easy_perform() failed: %s (res: %d, http code: %d)",
					curl_easy_strerror(res), res, http_code);
		} else {
			MY_LOG_ERR("curl_easy_perform() failed: %s (code: %d)",
					curl_easy_strerror(res), res);
			pr_err("curl_easy_perform() failed: %s (code: %d)",
					curl_easy_strerror(res), res);
		}
	} else {
		parse_account_json(response);
	}

	// clean up resourses and release this handle
	curl_slist_free_all(headers);
	FREE(response);
	release_curl_pool_handle(curl);
}

void get_order_status(const char *uuid)
{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;
	char jwt[JWT_BUFFER_SIZE];
	char auth_header[HEADER_BUFFER_SIZE];
	char url[URL_BUFFER_SIZE];
	char query_str[QUERY_BUFFER_SIZE];
	char *response = NULL;
	long http_code = 0;

	if (!get_curl_pool_handle(&curl)) {
		MY_LOG_ERR("get_curl_pool_handle() failed.");
		pr_err("get_curl_pool_handle() failed.");
		return;
	}

	snprintf(query_str, sizeof(query_str), "uuid=%s", uuid);

	generate_hash_jwt(jwt, sizeof(jwt), query_str);

	// autorization header
	snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
	headers = curl_slist_append(headers, auth_header);

	snprintf(url, sizeof(url), API_URL_ORDER "?%s", query_str);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (res == CURLE_OK && (http_code == 200 || http_code == 201)) {
		parse_get_order_status(response);
	} else {
		MY_LOG_ERR("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
		pr_err("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
	}
	curl_slist_free_all(headers);
	FREE(response);
	release_curl_pool_handle(curl);
}

int check_order_status(const char *uuid)
{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;
	char jwt[JWT_BUFFER_SIZE];
	char auth_header[HEADER_BUFFER_SIZE];
	char url[URL_BUFFER_SIZE];
	char query_str[QUERY_BUFFER_SIZE];
	char *response = NULL;
	long http_code = 0;
	int retry_state = -1;

	if (!get_curl_pool_handle(&curl)) {
		MY_LOG_ERR("get_curl_pool_handle() failed.");
		pr_err("get_curl_pool_handle() failed.");
		return retry_state;
	}

	snprintf(query_str, sizeof(query_str), "uuid=%s", uuid);

	generate_hash_jwt(jwt, sizeof(jwt), query_str);

	// autorization header
	snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
	headers = curl_slist_append(headers, auth_header);

	snprintf(url, sizeof(url), API_URL_ORDER "?%s", query_str);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (res == CURLE_OK && (http_code == 200 || http_code == 201)) {
		json_t *root;
		json_error_t error;

		root = json_loads(response, 0, &error);
		if (!root) {
			pr_err("json_loads() failed: %s", error.text);
			MY_LOG_ERR("json_loads() failed: %s", error.text);
			retry_state = FAILED;
		}
		const char *state = json_string_value(json_object_get(root, "state"));

		if (strcmp(state, "wait") == 0) {
			retry_state = PENDING;
		} else if (strcmp(state, "done") == 0 || strcmp(state, "cancel") == 0) {
			pr_cancel("uuid: %s, state: %s", uuid, state);
			remove_order(uuid);
			retry_state = COMPLETED;
		}
		json_decref(root);
	} else {
		MY_LOG_ERR("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
		pr_err("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
	}
	curl_slist_free_all(headers);
	FREE(response);
	release_curl_pool_handle(curl);
	return retry_state;
}

void get_open_orders_status(const char *market, const char **states,
		size_t states_count, int page, int limit, const char *order_by)
{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;
	char jwt[JWT_BUFFER_SIZE];
	char auth_header[HEADER_BUFFER_SIZE];
	char url[URL_BUFFER_SIZE];
	char query_str[QUERY_BUFFER_SIZE];
	char *response = NULL;
	long http_code = 0;
	int offset = 0;

	if (!get_curl_pool_handle(&curl)) {
		MY_LOG_ERR("get_curl_pool_handle() failed.");
		pr_err("get_curl_pool_handle() failed.");
		return;
	}

	// request parameters
	if (!IS_EMPTY_STR(market)) {
		offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"market=%s", market);
	}

	if (states && states_count > 0) {
		for (size_t i = 0; i < states_count; i++) {
			offset += snprintf(query_str + offset, sizeof(query_str) - offset,
					"&states[]=%s", states[i]);
		}
	}

	if (page > 0) {
		offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"&page=%d", page);
	}

	if (limit > 0) {
		limit = (limit > 100) ? 100 : limit;
		offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"&limit=%d", limit);
	}

	if (!IS_EMPTY_STR(order_by)) {
		offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"&order_by=%s", order_by);
	}

	pr_out("query_str: %s", query_str);

	generate_hash_jwt(jwt, sizeof(jwt), query_str);

	// authorization header
	snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
	headers = curl_slist_append(headers, auth_header);

	snprintf(url, sizeof(url), API_URL_ORDERS_OPEN "?%s", query_str);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	if (res == CURLE_OK && (http_code == 200 || http_code == 201)) {
		parse_get_open_orders_status(response);
	} else {
		MY_LOG_ERR("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
		pr_err("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
	}
	curl_slist_free_all(headers);
	FREE(response);
	release_curl_pool_handle(curl);
}

void get_closed_orders_status(const char *market, const char **states,
		size_t states_count, long start_time, long end_time,
		int limit, const char *order_by)
{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;
	char jwt[JWT_BUFFER_SIZE];
	char auth_header[HEADER_BUFFER_SIZE];
	char url[URL_BUFFER_SIZE];
	char query_str[QUERY_BUFFER_SIZE];
	char *response = NULL;
	long http_code = 0;
	int offset = 0;

	if (!get_curl_pool_handle(&curl)) {
		MY_LOG_ERR("get_curl_pool_handle() failed.");
		pr_err("get_curl_pool_handle() failed.");
		return;
	}

	if (!IS_EMPTY_STR(market)) {
		offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"market=%s", market);
	}

	if (states && states_count > 0) {
        for (size_t i = 0; i < states_count; i++) {
            offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                               "&states[]=%s", states[i]);
        }
    } else {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"&states[]=done");
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"&states[]=cancel");
    }

	// ISO-8601 or timestamp, using timestamp here
	if (start_time > 0) {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&start_time=%ld", start_time);
    }

    if (end_time > 0) {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&end_time=%ld", end_time);
    }

	if (limit > 0) {
		limit = (limit > 100) ? 100 : limit;
		offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"&limit=%d", limit);
	}

	if (!IS_EMPTY_STR(order_by)) {
		offset += snprintf(query_str + offset, sizeof(query_str) - offset,
				"&order_by=%s", order_by);
	}

	pr_out("query_str: %s", query_str);

	generate_hash_jwt(jwt, sizeof(jwt), query_str);

	// authorization header
	snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
	headers = curl_slist_append(headers, auth_header);

	snprintf(url, sizeof(url), API_URL_ORDERS_CLOSED "?%s", query_str);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (res == CURLE_OK && (http_code == 200 || http_code == 201)) {
		parse_get_closed_orders_status(response);
	} else {
		MY_LOG_ERR("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
		pr_err("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
	}
	curl_slist_free_all(headers);
	FREE(response);
	release_curl_pool_handle(curl);
}

// POST
static void clean_up_resources(char **response, struct curl_slist **headers, CURL **curl)
{
	FREE(*response);
	curl_slist_free_all(*headers);
	release_curl_pool_handle(*curl);
	*response = NULL;
	*headers = NULL;
}

void place_order(const char *market, const char *side, double volume,
		double price, const char *ord_type, const char *time_in_force)
{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;
	char identifier[UUID_BUFFER_SIZE];
	char jwt[1024];
	char auth_header[HEADER_BUFFER_SIZE];
	char post_data[POST_BUFFER_SIZE];
	char query_str[QUERY_BUFFER_SIZE];
	char *response = NULL;
	long http_code = 0;
	int attempt = 0;
	bool success = false;
	long retry_after = 0;

	generate_uuid(identifier); // idempotency

	for (attempt = 0; attempt < RETRY && !success; attempt++) {
		if (!get_curl_pool_handle(&curl)) {
			MY_LOG_ERR("get_curl_pool_handle() failed.");
			pr_err("get_curl_pool_handle() failed.");
			continue;
		}

		if (strcmp(ord_type, "price") == 0) {
			snprintf(post_data, sizeof(post_data), PRICE_ORDER_TEMPLATE(
						"%s", "%s", "%s", "%.8f", "%s"),
						identifier, market, ord_type, price, side);
			snprintf(query_str, sizeof(query_str), QUERY_PRICE_ORDER(
						"%s", "%s", "%s", "%.8f", "%s"),
						identifier, market, ord_type, price, side);
		} else if (strcmp(ord_type, "market") == 0) {
			snprintf(post_data, sizeof(post_data), MARKET_ORDER_TEMPLATE(
						"%s", "%s", "%s", "%s", "%.8f"),
					identifier, market, ord_type, side, volume);
			snprintf(query_str, sizeof(query_str), QUERY_MARKET_ORDER(
						"%s", "%s", "%s", "%s", "%.8f"),
						identifier, market, ord_type, side, volume);
		} else { // ordering limit or best price can add time_in_force
			if (IS_EMPTY_STR(time_in_force)) {
				snprintf(post_data, sizeof(post_data), LIMIT_ORDER_NO_OP_TEMPLATE(
							"%s", "%s", "%s", "%.8f", "%s", "%.8f"),
							identifier, market, ord_type, price, side,
							volume);
				snprintf(query_str, sizeof(query_str), QUERY_LIMIT_NO_OP_ORDER(
							"%s", "%s", "%s", "%.8f", "%s", "%.8f"),
							identifier, market, ord_type, price, side, volume);
			} else {
				if (strcmp(ord_type, "limit") == 0) {
					snprintf(post_data, sizeof(post_data), LIMIT_ORDER_TEMPLATE(
								"%s", "%s", "%s", "%.8f", "%s", "%s", "%.8f"),
								identifier, market, ord_type, price, side,
								time_in_force, volume);
					snprintf(query_str, sizeof(query_str), QUERY_LIMIT_ORDER(
								"%s", "%s", "%s", "%.8f", "%s", "%s", "%.8f"),
								identifier, market, ord_type, price, side,
								time_in_force, volume);
				} else if (strcmp(ord_type, "best") == 0
						&& strcmp(side, "bid") == 0) {
					snprintf(post_data, sizeof(post_data),
							BEST_BID_ORDER_TEMPLATE(
								"%s", "%s", "%s", "%.8f", "%s", "%s"),
								identifier, market, ord_type, price, side,
								time_in_force);
					snprintf(query_str, sizeof(query_str),
							QUERY_BEST_BID_ORDER(
								"%s", "%s", "%s", "%.8f", "%s", "%s"),
								identifier, market, ord_type, price, side,
								time_in_force);
				} else if (strcmp(ord_type, "best") == 0
						&& strcmp(side, "ask") == 0) {
					snprintf(post_data, sizeof(post_data),
							BEST_ASK_ORDER_TEMPLATE(
								"%s", "%s", "%s", "%s", "%s", "%.8f"),
								identifier, market, ord_type, side,
								time_in_force, volume);
					snprintf(query_str, sizeof(query_str),
							QUERY_BEST_ASK_ORDER(
								"%s", "%s", "%s", "%s", "%s", "%.8f"),
								identifier, market, ord_type, side,
								time_in_force, volume);
				}
			}
		}
		pr_out("query_str: %s", query_str);
		pr_out("post_data: %s", post_data);

		generate_hash_jwt(jwt, sizeof(jwt), query_str);

		// autorization header
		snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
		headers = curl_slist_append(headers, auth_header);

		// content-type header
		headers = curl_slist_append(headers, "Content-Type: application/json");

		MALLOC(response, RESPONSE_BUFFER_SIZE);

		// set curl options
		curl_easy_setopt(curl, CURLOPT_URL, API_URL_ORDERS);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

		// proceed request & retry
		res = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		if (res == CURLE_OK && (http_code == 200 || http_code == 201)) {
			parse_place_order_response(response);
			success = true;
			break;
		}

		MY_LOG_ERR("place order attempt: %d failed: %s (res: %d, http code: %d)",
				attempt + 1, curl_easy_strerror(res), res, http_code);
		pr_err("place order attempt: %d failed: %s (res: %d, http code: %d)",
				attempt + 1, curl_easy_strerror(res), res, http_code);

		if (http_code == 429) { // Retry-After or 5 sec
			curl_easy_getinfo(curl, CURLINFO_RETRY_AFTER, &retry_after);
			retry_after = retry_after > 0 ? retry_after : 5;
		}

		// clean up resources before sleep() or usleep().
		clean_up_resources(&response, &headers, &curl);

		if (http_code == 429) {
			sleep(retry_after);
		} else if (DO_RETRY(res, http_code) && attempt < RETRY - 1) {
			usleep(RETRY_DELAY_MS * 1000 * (1 << attempt));
		}

	}
	if (!success) {
		MY_LOG_ERR("place_order() failed: max retries");
		pr_err("place_order() failed: max retries");
	} else {
		clean_up_resources(&response, &headers, &curl);
	}
}

// CANCEL
void cancel_order(const char *uuid)
{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;
	char identifier[UUID_BUFFER_SIZE];
	char jwt[JWT_BUFFER_SIZE];
	char auth_header[HEADER_BUFFER_SIZE];
	char url[URL_BUFFER_SIZE];
	char query_str[QUERY_BUFFER_SIZE];
	char *response = NULL;
	long http_code = 0;
	int attempt = 0;
	bool success = false;
	long retry_after = 0;

	generate_uuid(identifier); // idempotency

	for (attempt = 0; attempt < RETRY && !success; attempt++) {
		if (!get_curl_pool_handle(&curl)) {
			MY_LOG_ERR("get_curl_pool_handle() failed.");
			pr_err("get_curl_pool_handle() failed.");
			continue;
		}

		snprintf(query_str, sizeof(query_str), "uuid=%s&identifier=%s", uuid,
				identifier);

		generate_hash_jwt(jwt, sizeof(jwt), query_str);

		// autorization header
		snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
		headers = curl_slist_append(headers, auth_header);

		// content-type header
		headers = curl_slist_append(headers, "Content-Type: application/json");

		// url + uuid + identifier(for idempotency)
		//snprintf(url, sizeof(url), "%s?uuid=%s", API_URL_ORDER, uuid); // no retry
		snprintf(url, sizeof(url), API_URL_ORDER "?%s", query_str);

		MALLOC(response, RESPONSE_BUFFER_SIZE);

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

		res = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		if (res == CURLE_OK && (http_code == 200 || http_code == 201)) {
			parse_cancel_order_response_json(response);
			success = true;
			break;
		}

		MY_LOG_ERR("cancel order attempt: %d failed: %s (res: %d, http code: %d)",
				attempt + 1, curl_easy_strerror(res), res, http_code);
		pr_err("cancel order attempt: %d failed: %s (res: %d, http code: %d)",
				attempt + 1, curl_easy_strerror(res), res, http_code);

		if (http_code == 429) { // Retry-After or 5 sec
			curl_easy_getinfo(curl, CURLINFO_RETRY_AFTER, &retry_after);
			retry_after = retry_after > 0 ? retry_after : 5;
		}

		// clean up resources before sleep() or usleep().
		clean_up_resources(&response, &headers, &curl);

		if (http_code == 429) {
			sleep(retry_after);
		} else if (DO_RETRY(res, http_code) && attempt < RETRY - 1) {
			usleep(RETRY_DELAY_MS * 1000 * (1 << attempt));
		}

	}
	if (!success) {
		MY_LOG_ERR("cancel_order() failed: max retries");
		pr_err("cancel_order() failed: max retries");
	} else {
		clean_up_resources(&response, &headers, &curl);
	}
}

void cancel_by_bulk(const char *cancel_side, const char *pairs,
                    const char *excluded_pairs, const char *quote_currencies,
                    int count, const char *order_by)
{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;
	char jwt[JWT_BUFFER_SIZE];
	char auth_header[HEADER_BUFFER_SIZE];
	char url[URL_BUFFER_SIZE];
	char query_str[QUERY_BUFFER_SIZE];
	char *response = NULL;
	long http_code = 0;
	int offset = 0;

	if (!get_curl_pool_handle(&curl)) {
		MY_LOG_ERR("get_curl_pool_handle() failed.");
		pr_err("get_curl_pool_handle() failed.");
		return;
	}

	// cannot take pairs and quote_currencies together
	if (!IS_EMPTY_STR(pairs) && !IS_EMPTY_STR(quote_currencies)) {
		MY_LOG_ERR("cancel_by_bulk() failed.");
		pr_err("cancel_by_bulk() failed.");
		return;
	}

	// query parameter
	if (!IS_EMPTY_STR(cancel_side)) {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "cancel_side=%s", cancel_side);
    } else {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "cancel_side=all");
    }

	if (!IS_EMPTY_STR(pairs)) {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&pairs=%s", pairs);
    } else if (!IS_EMPTY_STR(quote_currencies)) {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&quote_currencies=%s", quote_currencies);
    }

	if (!IS_EMPTY_STR(excluded_pairs)) {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&excluded_pairs=%s", excluded_pairs);
    }

    if (count > 0) {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&count=%d", count);
    } else {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&count=20");
    }

    if (!IS_EMPTY_STR(order_by)) {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&order_by=%s", order_by);
    } else {
        offset += snprintf(query_str + offset, sizeof(query_str) - offset,
                           "&order_by=desc");
    }

	pr_out("query_str: %s", query_str);

	generate_hash_jwt(jwt, sizeof(jwt), query_str);

	// authorization header
	snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
	headers = curl_slist_append(headers, auth_header);

	// URL
	snprintf(url, sizeof(url), API_URL_ORDERS_OPEN "?%s", query_str);

	MALLOC(response, RESPONSE_BUFFER_SIZE);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); // 1L(active)

	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (res == CURLE_OK && (http_code == 200 || http_code == 201)) {
		parse_cancel_by_bulk_response(response);
	} else {
		MY_LOG_ERR("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
		pr_err("curl_easy_perform() failed: %s (code: %d, http code: %d)",
				curl_easy_strerror(res), res, http_code);
	}
	curl_slist_free_all(headers);
	FREE(response);
	release_curl_pool_handle(curl);
}
