#include "utils.h"

static void base64url_encode(const unsigned char *input, int len, char *output)
{
    char temp[1024];
    int encoded_len = EVP_EncodeBlock((unsigned char *)temp, input, len);

    int j = 0;
    for (int i = 0; i < encoded_len; ++i) {
        if (temp[i] == '+') output[j++] = '-';
        else if (temp[i] == '/') output[j++] = '_';
        else if (temp[i] == '=') continue;
        else output[j++] = temp[i];
    }
    output[j] = '\0';
}

void generate_hash_jwt(char *jwt, size_t jwt_size, const char *query_str)
{
    const char *api_key = getenv("UPBIT_API_KEY");
    const char *secret_key = getenv("UPBIT_SECRET_KEY");
    if (!api_key || !secret_key) {
        MY_LOG_ERR("no api & secret key.");
        return;
    }

    /* UUID */
    uuid_t binuuid;
    char nonce[UUID_BUFFER_SIZE];
    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, nonce);

    /* query_hash */
	unsigned char hash_raw[SHA512_DIGEST_LENGTH];
	SHA512((const unsigned char *)query_str, strlen(query_str), hash_raw);

	// inverting to hex
	char query_hash_hex[SHA512_DIGEST_LENGTH * 2 + 1];
	for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i)
		sprintf(query_hash_hex + (i * 2), "%02x", hash_raw[i]);
	query_hash_hex[SHA512_DIGEST_LENGTH * 2] = '\0';

    /* Header */
    const char *header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char header_b64[64];
	base64url_encode((const unsigned char *)header, strlen(header), header_b64);

    /* Payload */
    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"access_key\":\"%s\",\"nonce\":\"%s\",\"query_hash\":\"%s\","
    		 "\"query_hash_alg\":\"SHA512\"}",
             api_key, nonce, query_hash_hex);
	

    char payload_b64[512];
	base64url_encode((const unsigned char *)payload, strlen(payload), payload_b64);

    /* Signature */
    char signing_input[1024];
    snprintf(signing_input, sizeof(signing_input), "%s.%s", header_b64, payload_b64);

    // HMAC-SHA256 - SHA256 hash
    unsigned char hmac[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;
    HMAC(EVP_sha256(), secret_key, strlen(secret_key),
         (unsigned char *)signing_input, strlen(signing_input), hmac, &hmac_len);

	// Base64 encoding
    char signature_b64[512];
	base64url_encode(hmac, hmac_len, signature_b64);

	/* Result */
    snprintf(jwt, jwt_size, "%s.%s.%s", header_b64, payload_b64, signature_b64);
}

void generate_jwt(char *jwt, size_t jwt_size)
{
    const char *api_key = getenv("UPBIT_API_KEY");
    const char *secret_key = getenv("UPBIT_SECRET_KEY");
    if (!api_key || !secret_key) {
		MY_LOG_ERR("no api & secret key.");
		pr_err("no api & secret key.");
        return;
    }

	/* UUID */ 
    uuid_t binuuid;
    char nonce[37]; // uuid format - 128-bit
    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, nonce);

	/* JWT header */
	// JSON format - encryption algorithm & token type
    const char *header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
	// Base64 encoding
    char header_b64[64];
    EVP_EncodeBlock((unsigned char *)header_b64,
					(const unsigned char *)header,
					strlen(header));

    /* JWT payload */
    char payload[256];
	// JSON format - api key & uuid
    snprintf(payload, sizeof(payload),
			"{\"access_key\":\"%s\",\"nonce\":\"%s\"}", api_key, nonce);
	// Base64 encoding
    char payload_b64[256];
    EVP_EncodeBlock((unsigned char *)payload_b64,
			(const unsigned char *)payload, strlen(payload));

    /* Signatrure */
    char signing_input[512];
    snprintf(signing_input, sizeof(signing_input),
			"%s.%s", header_b64, payload_b64);
    // HMAC-SHA256 - SHA256 hash
    unsigned char hmac[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;
    HMAC(EVP_sha256(), secret_key, strlen(secret_key),
			(unsigned char *)signing_input, strlen(signing_input), hmac,
			&hmac_len);

	// Base64 encoding
    char signature_b64[256];
    EVP_EncodeBlock((unsigned char *)signature_b64, hmac, hmac_len);

	/* Result */
    snprintf(jwt, jwt_size, "%s.%s.%s", header_b64, payload_b64, signature_b64);
}

void generate_uuid(char *out)
{
	uuid_t uuid;
	uuid_generate_random(uuid);
	uuid_unparse(uuid, out);
}
