#ifndef _JWT_H
#define _JWT_H

#include "common.h"

#include <time.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <uuid/uuid.h>

#define JWT_BUFFER_SIZE 512

void generate_jwt(char *jwt, size_t jwt_size);

#endif//_JWT_H
