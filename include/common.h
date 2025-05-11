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
#include <locale.h>

#include "rb.h"
#include "log.h"

/* SWITCH */
#define UI_ON 1
//#define UI_ON 0
//#define PRINT 1
#define PRINT 0

/* UTILITY MACROS */
#define IS_EMPTY_STR(str) (((str) == NULL) || ((str)[0] == '\0'))
#define EMPTY_ARR(arr) ((arr)[0] == '\0')
#define SAFE_STRCPY(dest, src, size) \
	do { \
		if ((dest) && (src) && (size) > 0) { \
			size_t len = strlen(src); \
			size_t copy_len = (len < (size - 1)) ? len : (size - 1); \
			memcpy((dest), (src), copy_len); \
			(dest)[copy_len] = '\0'; \
		} else { \
			fprintf(stderr, "[ERR][%s:%d] invalid args(dest=%p, src=%p, size=%zu)\n", \
					__FILE__, __LINE__, (void*)(dest), (void*)(src), (size_t)(size)); \
		} \
	} while (0)

/* PRINT MACRO OPTIONS */
#define INFO "\033[93m[INFO] \033[0m"
#define ERROR "\033[91m[ERROR] \033[0m"
#define OK "\033[92m[OK] \033[0m"
#define TRADE "\033[96m[TRADE] \033[0m"
#define ORDER "\033[94m[ORDER] \033[0m"
#define CANCEL "\033[95m[CANCEL] \033[0m"

#if PRINT
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

#define pr_track(res, input_type) \
	flockfile(stdout); \
	fprintf(stdout, "[TRACK][%s:%d] %p (type: %d)", __FILE__, __LINE__, res, input_type); \
	fputc('\n', stdout); \
	funlockfile(stdout);

#define pr_untrack(res) \
	flockfile(stdout); \
	fprintf(stdout, "[UNTRACK][%s:%d] %p", __FILE__, __LINE__, res); \
	fputc('\n', stdout); \
	funlockfile(stdout);

#define pr_malloc(p, size) \
	flockfile(stdout); \
	fprintf(stdout, "[%s:%d] malloc: %p (%zu bytes)\n", __FILE__, __LINE__, p, size); \
	fputc('\n', stdout); \
	funlockfile(stdout);

#define pr_free(p) \
	flockfile(stdout); \
	fprintf(stdout, "[%s:%d] free: %p\n", __FILE__, __LINE__, p); \
	fputc('\n', stdout); \
	funlockfile(stdout);
#else
#define pr_err(arg...)
#define pr_out(arg...)
#define pr_ok(arg...)
#define pr_trade(arg...)
#define pr_order(arg...)
#define pr_cancel(arg...)
#define pr_track(res, input_type)
#define pr_untrack(res)
#define pr_malloc(p, size)
#define pr_free(p)
#endif

/* RESOURCE MANAGEMENT */
/******************************************************************************
 * fucntions beginning with init_ & destroy_ do not use this macro            *
 * only for temporary objects									              *
 ******************************************************************************/

typedef enum resource_type_e {
	RES_MEM,
	RES_LOG,
	RES_SIG,
	RES_TXN,
	RES_JSON_CONFIG,
	RES_ORDER_HANDLER,
	RES_SYM_INFO,
	RES_ACCOUNT,
	RES_CURL,
    RES_THREAD_QUEUE,
    RES_UI,
    RES_WEBSOCKET,
} resource_type_t;

typedef struct resource_node_s {
    void* resource;
    resource_type_t type;
    struct resource_node_s* next;
} resource_node_t;

extern resource_node_t* g_resources;
extern pthread_mutex_t g_resources_mtx;

#define TRACK_RESOURCE(res, input_type) \
    do { \
        resource_node_t* node = malloc(sizeof(resource_node_t)); \
        if (!node) { \
            pr_err("malloc() failed."); \
            exit(EXIT_FAILURE); \
        } \
		pr_track(res, input_type); \
        node->resource = res; \
        node->type = input_type; \
		pthread_mutex_lock(&g_resources_mtx); \
        node->next = g_resources; \
        g_resources = node; \
		pthread_mutex_unlock(&g_resources_mtx); \
    } while (0)

#define UNTRACK_RESOURCE(res) \
    do { \
		pr_untrack(res); \
		pthread_mutex_lock(&g_resources_mtx); \
        resource_node_t **current = &g_resources; \
        while (*current) { \
            if ((*current)->resource == (res)) { \
                resource_node_t *tmp = *current; \
                *current = (*current)->next; \
                free(tmp); \
                break; \
            } \
            current = &(*current)->next; \
        } \
		pthread_mutex_unlock(&g_resources_mtx); \
    } while (0)

#define MALLOC(p, size) \
    do { \
        p = malloc(size); \
        if (!p) { \
            pr_err("malloc() failed."); \
            exit(EXIT_FAILURE); \
        } \
        memset(p, 0, size); \
		pr_malloc(p, size); \
        TRACK_RESOURCE(p, RES_MEM); \
    } while (0)

#define FREE(p) \
    do { \
		if (p) { \
			pr_free(p); \
			UNTRACK_RESOURCE(p); \
            free(p); \
            p = NULL; \
        } \
    } while (0)

#endif//_COMMON_H_
