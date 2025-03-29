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
#include <stdatomic.h>

#include "rb.h"
#include "log.h"

/* switch */
#define UI_ON 0
#define PRINT 0

/* print macro options */
#define INFO "\033[93m[INFO] \033[0m"
#define ERROR "\033[91m[ERROR] \033[0m"
#define OK "\033[92m[OK] \033[0m"
#define TRADE "\033[96m[TRADE] \033[0m"
#define ORDER "\033[94m[ORDER] \033[0m"
#define CANCEL "\033[95m[CANCEL] \033[0m"

#define pr_err(arg...) \
	flockfile(stderr); \
	fprintf(stderr, ERROR arg); \
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

/* RESOURCE */
typedef enum {
	RES_MEM,
	RES_LOG,
    RES_CURL,
    RES_WEBSOCKET,
    RES_THREAD_QUEUE,
    RES_ACCOUNT,
    RES_JSON,
    RES_UI
} resource_type_e;

typedef struct resource_node_s {
    void* resource;
    resource_type_e type;
    struct resource_node* prev;
} resource_node_t;

resource_node_t* g_resources = NULL;

#define TRACK_RESOURCE(res, type) \
    do { \
        resource_node_t* node = malloc(sizeof(resource_node_t)); \
        if (!node) { \
            pr_err("[ERR][%s:%d] malloc() failed.", __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
        node->resource = res; \
        node->type = type; \
        node->prev = g_resources; \
        g_resources = node; \
    } while (0)

#define UNTRACK_RESOURCE(res) \
    do { \
        resource_node_t **current = &g_resources; \
        while (*current) { \
            if ((*current)->resource == (res)) { \
                resource_node_t *tmp = *current; \
                *current = (*current)->prev; \
                free(tmp); \
                break; \
            } \
            current = &(*current)->prev; \
        } \
    } while (0)


#define MALLOC(p, size) \
    do { \
        p = malloc(size); \
        if (!p) { \
            pr_err("[ERR][%s:%d] malloc() failed.", __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
        memset(p, 0, size); \
        TRACK_RESOURCE(p, RES_MEM); \
    } while (0)

#define FREE(p) \
    do { \
        if (p) { \
            UNTRACK_RESOURCE(p); \
            free(p); \
            p = NULL; \
        } \
    } while (0)

#endif//_COMMON_H_
