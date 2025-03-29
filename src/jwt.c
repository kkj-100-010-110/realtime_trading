#include "jwt.h"

void generate_jwt(char *jwt, size_t jwt_size)
{
    const char *api_key = getenv("UPBIT_API_KEY");
    const char *secret_key = getenv("UPBIT_SECRET_KEY");
    if (!api_key || !secret_key) {
		LOG_ERR("no api & secret key");
		pr_err("no api & secret key");
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
			(const unsigned char *)payload,
			strlen(payload));

    /* Signatrure */
    char signing_input[512];
    snprintf(signing_input, sizeof(signing_input),
			"%s.%s", header_b64, payload_b64);
    // HMAC-SHA256 - SHA256 hash
    unsigned char hmac[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;
    HMAC(EVP_sha256(), secret_key, strlen(secret_key),
			(unsigned char *)signing_input, strlen(signing_input),
			hmac, &hmac_len);
	// Base64 encoding
    char signature_b64[256];
    EVP_EncodeBlock((unsigned char *)signature_b64, hmac, hmac_len);

	/* Result */
    snprintf(jwt, jwt_size, "%s.%s.%s", header_b64, payload_b64, signature_b64);
}
