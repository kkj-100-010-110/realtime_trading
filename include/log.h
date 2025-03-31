#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <libwebsockets.h> 
#include <jansson.h>

#include "thread_queue.h"

#define LOG_ERR(fmt, ...) \
	do_log("[ERR][%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__);

#define LOG_REP(fmt, ...) \
	do_log("[REP][%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__);

#define MAX_FILE_SIZE 10 * 1024 * 1024  // 10MB
#define MAX_LOG_FILES 5

typedef struct {
	const char *fmt;
	va_list args;
} log_task_arg_t;

/* thread_queue task */
void logging_task(void *arg);

void init_log();
void do_log(const char *fmt, ...);
void destroy_log();

#endif//_LOG_H
