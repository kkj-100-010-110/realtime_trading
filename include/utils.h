#ifndef _UTILS_H
#define _UTILS_H

#include "common.h"

#include <time.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/crypto.h>
#include <openssl/sha.h> //SHA512
#include <uuid/uuid.h>

/*******************************************************************************
 * Generate JWT																   *
 * Generate UUID for identifers												   *
 ******************************************************************************/

#define JWT_BUFFER_SIZE 512
#define UUID_BUFFER_SIZE 37

void generate_jwt(char *jwt, size_t jwt_size);
void generate_hash_jwt(char *jwt, size_t jwt_size, const char *query_str);
void generate_uuid(char *out);

#endif//_UTILS_H
