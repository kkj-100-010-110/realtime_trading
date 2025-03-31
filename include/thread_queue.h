#ifndef _THREAD_QUEUE_H
#define _THREAD_QUEUE_H

#include "common.h"
#include "order_handler.h"
#include "rest_api.h"

#define MAX_QUEUE_SIZE 64
#define MAX_CPU sysconf(_SC_NPROCESSORS_ONLN)
#define INITIAL_BACKOFF_MS 2000
#define MAX_RETRY 3

typedef struct {
	void (*task_function)(void *);
	void *arg;
} task_t;

typedef struct {
	task_t *queue;
	int front;
	int rear;
	int count;
	pthread_mutex_t lock;
	pthread_cond_t not_empty;
	pthread_cond_t not_full;
	bool shutdown;
} task_queue_t;

typedef enum retry_state_e {
	FAILED = -1,
	PENDING = 0,
	COMPLETED = 1
} retry_state_t;

typedef struct {
	char uuid[37];
	int retry_count;
	time_t next_retry_time; // time(timestamp)
} retry_task_t;

void init_thread_queue();
void destroy_thread_queue();
bool enqueue_task(void (*task_function)(void *), void *arg);
bool dequeue_task(task_t *t);
void *worker_thread(void *arg);

/* RETRY */
void enqueue_retry_task(const char *uuid, int initial_backoff_ms);
void process_retry_task(void *arg);

#endif//_THREAD_QUEUE_H
