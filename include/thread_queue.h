#ifndef _THREAD_QUEUE_H
#define _THREAD_QUEUE_H

#include "common.h"

#define MAX_QUEUE_SIZE 64
#define MAX_CPU sysconf(_SC_NPROCESSORS_ONLN)

typedef struct {
	void (*task_function)(void *);
	void *arg;
} Task;

typedef struct {
	Task *queue;
	int front;
	int rear;
	int count;
	pthread_mutex_t lock;
	pthread_cond_t not_empty;
	pthread_cond_t not_full;
	bool shutdown;
} TaskQueue;

void thread_queue_init();
void thread_queue_destroy();
bool enqueue_task(void (*task_function)(void *), void *arg);
bool dequeue_task(Task *t);
void *worker_thread(void *arg);

#endif//_THREAD_QUEUE_H
