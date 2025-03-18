#include "rest_api.h"

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total_size = size * nmemb;
	char *response = (char *)userdata;
	strncat(response, (char *)ptr, total_size);
    return total_size;
}

void get_account_info_task(void *arg)
{
	(void)arg;
	get_account_info();
}

void get_account_info()
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[512];
    char auth_header[1024];
	char *response = NULL;

    generate_jwt(jwt, sizeof(jwt)); // JWT

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

	MALLOC(response, BUFFER_SIZE);

    // init & configure CURL
    curl = curl_easy_init();
    if (curl) {
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, API_URL_ACCOUNT);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

		// print debugging info
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
			pr_err("get_account_info() failed: %s", curl_easy_strerror(res));
		} else {
			parse_balance_json(response);
		}
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
	}
	free(response);
}

void place_order_task(void *arg)
{
}

void place_order(const char *market, const char *side, double volume, double price)
{
	CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[512];
    char auth_header[1024];
    char *response = NULL;
    char post_data[1024];

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

	MALLOC(response, BUFFER_SIZE);

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
            pr_err("place_buy_order() failed: %s", curl_easy_strerror(res));
		} else {
			parse_order_response_json(response);
		}

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
	}
	free(response);
}

void place_buy_order(const char *market, double volume, double price)
{
	place_order(market, "bid", volume, price);
}

void place_sell_order(const char *market, double volume, double price)
{
	place_order(market, "ask", volume, price);
}

void cancel_order(const char *uuid)
{
	CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[512];
    char auth_header[1024];
    char *response = NULL;
    char url[1024];

    generate_jwt(jwt, sizeof(jwt));

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    // content-type header
    headers = curl_slist_append(headers, "Content-Type: application/json");

	// url + uuid
    snprintf(url, sizeof(url), "%s?uuid=%s", API_URL_ORDER, uuid);

	MALLOC(response, BUFFER_SIZE);

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
            pr_err("cancel_order() failed: %s", curl_easy_strerror(res));
        } else {
            pr_cancel("Cancel Order Response: %s", response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
	}
	free(response);
}

void market_order(const char *market, const char *side, double amount)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[512];
    char auth_header[1024];
    char post_data[256];
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
                 market, side, amount);
    } else if (strcmp(side, "ask") == 0) {
        snprintf(post_data, sizeof(post_data),
                 "{\"market\":\"%s\",\"side\":\"%s\",\"ord_type\":\"market\",\"volume\":\"%.8f\"}",
                 market, side, amount);
    } else {
        pr_err("Invalid order side: %s", side);
        return;
    }

	MALLOC(response, BUFFER_SIZE);

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
        } else {
            printf("Market Order Response: %s\n", response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
	}
	free(response);
}

void get_order_status(const char *uuid)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[512];
    char auth_header[1024];
    char url[512];
    char *response = NULL;

    generate_jwt(jwt, sizeof(jwt));

	// autorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    snprintf(url, sizeof(url), "https://api.upbit.com/v1/order?uuid=%s", uuid);

	MALLOC(response, BUFFER_SIZE);

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
        } else {
            printf("Order Status Response: %s\n", response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
	free(response);
}

void cancel_by_bulk(const char *cancel_side, const char *pairs,
					const char *excluded_pairs, const char *quote_currencies,
					int count, const char *order_by)
{
	CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    char jwt[512];
    char auth_header[1024];
    char url[1024];
    char body[BUFFER_SIZE];
    char *response = NULL;

    generate_jwt(jwt, sizeof(jwt));

    // authorization header
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt);
    headers = curl_slist_append(headers, auth_header);

    // content-type header
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // URL
    snprintf(url, sizeof(url), "https://api.upbit.com/v1/orders");

    // create request body
    snprintf(body, sizeof(body), "{");
    if (cancel_side != NULL) {
        strcat(body, "\"cancel_side\":\"");
        strcat(body, cancel_side);
        strcat(body, "\",");
    }
    if (pairs != NULL) {
        strcat(body, "\"pairs\":\"");
        strcat(body, pairs);
        strcat(body, "\",");
    }
    if (excluded_pairs != NULL) {
        strcat(body, "\"excluded_pairs\":\"");
        strcat(body, excluded_pairs);
        strcat(body, "\",");
    }
    if (quote_currencies != NULL) {
        strcat(body, "\"quote_currencies\":\"");
        strcat(body, quote_currencies);
        strcat(body, "\",");
    }
    if (count > 0) {
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "\"count\":%d,", count);
        strcat(body, count_str);
    }
    if (order_by != NULL) {
        strcat(body, "\"order_by\":\"");
        strcat(body, order_by);
        strcat(body, "\",");
    }
    // remove last comma
    if (body[strlen(body) - 1] == ',') {
        body[strlen(body) - 1] = '\0';
    }
    strcat(body, "}");

    MALLOC(response, BUFFER_SIZE);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body); // request body
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            pr_err("cancel_orders_bulk() failed: %s", curl_easy_strerror(res));
        } else {
            pr_cancel("Cancel Orders Bulk Response: %s", response);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    free(response);
}
