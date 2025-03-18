#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libwebsockets.h> 
#include <jansson.h>

#define MAX_FILE_SIZE 10 * 1024 * 1024  // 10MB
#define MAX_LOG_FILES 5

static void log_emit(int level, const char *msg);
void init_logging();
void log_err(const char *fmt, ...);
void rotate_log_file();
void terminate_logging();

#endif//_LOG_H
