#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>

#include "rb.h"

#define INFO "\033[93m[INFO]\033[0m"
#define ERR "\033[91m[ERR]\033[0m"
#define OK "\033[92m[OK]\033[0m"

#define pr_err(arg...) \
	flockfile(stderr); \
	fprintf(stderr, ERR arg); \
	fputc('\n', stderr); \
	funlockfile(stderr);

#define pr_out(arg...) \
	flockfile(stdout); \
	fprintf(stdout, INFO arg); \
	fputc('\n', stdout); \
	funlockfile(stdout);

#define pr_ok(arg...) \
	flockfile(stdout); \
	fprintf(stdout, OK arg); \
	fputc('\n', stdout); \
	funlockfile(stdout);

/* print transactions */

#define TRADE "\033[96m[TRADE]\033[0m"
#define ORDER "\033[94m[ORDER]\033[0m"
#define CANCEL "\033[95m[CANCEL]\033[0m"

#define pr_trade(arg...) \
	flockfile(stdout); \
	fprintf(stdout, TRADE arg); \
	fputc('\n', stdout); \
	funlockfile(stdout);

#define pr_order(arg...) \
	flockfile(stdout); \
	fprintf(stdout, ORDER arg); \
	fputc('\n', stdout); \
	funlockfile(stdout);

#define pr_cancel(arg...) \
	flockfile(stdout); \
	fprintf(stdout, CANCEL arg); \
	fputc('\n', stdout); \
	funlockfile(stdout);

#define MALLOC(p, size) \
    do { \
		p = malloc(size); \
		memset(p, 0, size); \
        if (!p) { \
            pr_err("malloc failed: %s:%d\n", __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#endif//_COMMON_H_
