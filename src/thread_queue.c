#include "thread_queue.h"

static TaskQueue *tq = NULL;
static pthread_t *threads = NULL;

void thread_queue_init()
{
	MALLOC(tq, sizeof(TaskQueue));
	MALLOC(tq->queue, sizeof(Task) * MAX_QUEUE_SIZE);
	tq->front = 0;
	tq->rear = 0;
	tq->count = 0;
	pthread_mutex_init(&tq->lock, NULL);
	pthread_cond_init(&tq->not_empty, NULL);
	pthread_cond_init(&tq->not_full, NULL);
	tq->shutdown = false;

	// thread pool
	MALLOC(threads, sizeof(pthread_t) * MAX_CPU);
	for (int i = 0; i < MAX_CPU; i+=1) {
		pthread_create(&threads[i], NULL, worker_thread, NULL);
	}
}

void thread_queue_destroy()
{
	if (!tq) {
		return;
	}
	pthread_mutex_lock(&tq->lock);
	tq->shutdown = true;
	pthread_cond_broadcast(&tq->not_empty);
	pthread_mutex_unlock(&tq->lock);

	while (tq->count > 0) {
        Task t;
        dequeue_task(&t);
        if (t.task_function == process_retry_task) {
            free(t.arg);  // RetryTask 메모리 해제
        }
    }

	for (int i = 0; i < MAX_CPU; i+=1) {
		pthread_join(threads[i], NULL);
	}
	free(threads);
	pthread_mutex_destroy(&tq->lock);
	pthread_cond_destroy(&tq->not_empty);
	pthread_cond_destroy(&tq->not_full);
	free(tq->queue);
	free(tq);
}

bool enqueue_task(void (*task_function)(void *), void *arg)
{
	pthread_mutex_lock(&tq->lock);

	while (tq->count == MAX_QUEUE_SIZE && !tq->shutdown) {
        pthread_cond_wait(&tq->not_full, &tq->lock);
    }
	if (tq->shutdown) {
        pthread_mutex_unlock(&tq->lock);
        return false;
    }

	tq->queue[tq->rear].task_function = task_function;
	tq->queue[tq->rear].arg = arg;
	tq->rear = (tq->rear + 1) % MAX_QUEUE_SIZE;
	tq->count++;

	pthread_cond_signal(&tq->not_empty);
    pthread_mutex_unlock(&tq->lock);

	return true;
}

/*
   from dequeue_task(Task *t), if t is "Task *", it should take deep copy
 */
//void task_deep_copy_txn(Task *dest, const Task *src) {
//    dest->func = src->func;
//    if (src->arg != NULL) {
//        dest->arg = malloc(sizeof(Transaction));
//        memcpy(dest->arg, src->arg, sizeof(Transaction));
//    } else {
//        dest->arg = NULL;
//    }
//}

bool dequeue_task(Task *t)
{
	pthread_mutex_lock(&tq->lock);

	while (tq->count == 0 && !tq->shutdown) {
		pthread_cond_wait(&tq->not_empty, &tq->lock);
	}
	if (tq->shutdown && tq->count == 0) {
		pthread_mutex_unlock(&tq->lock);
		return false;
	}

	*t = tq->queue[tq->front];
	tq->front = (tq->front + 1) % MAX_QUEUE_SIZE;
	tq->count--;

	pthread_cond_signal(&tq->not_full);
	pthread_mutex_unlock(&tq->lock);
	return true;
}

void *worker_thread(void *arg)
{
	while (1) {
		Task t;
		if (!dequeue_task(&t)) {
			break;
		}
		t.task_function(t.arg);
	}
	return NULL;
}

void enqueue_retry_task(const char *uuid, int initial_backoff_ms)
{
    RetryTask *task = malloc(sizeof(RetryTask));
    strncpy(task->uuid, uuid, sizeof(task->uuid));
    task->retry_count = 0;
    task->next_retry_time = time(NULL) + (initial_backoff_ms / 1000);

    // push retry task into the thread queue
    enqueue_task(process_retry_task, task);
}

void process_retry_task(void *arg)
{
	RetryTask *task = (RetryTask *)arg;
	time_t now = time(NULL);

	// if it's not the retry time, push it into a queue again
	if (now < task->next_retry_time) {
		enqueue_task(process_retry_task, task);
		return;
	}

	// check the order state("wait", "done" and "cancel")
	int state = check_order_status(task->uuid); // FAILED, PENDING and COMEPLETED

	// when state is FAILED or PENDING, retry again. 
	if (state == 0 || state == -1) {
		if (task->retry_count++ < MAX_RETRY) {
			int backoff_ms = INITIAL_BACKOFF_MS * (1 << task->retry_count);
			task->next_retry_time = now + (backoff_ms / 1000);
			enqueue_task(process_retry_task, task);
		} else { // has all retries done but it failed. 
			LOG_ERR("Max retries exceeded for order %s", task->uuid);
			free(task);
		}
	} else { // COMPLETED
		remove_order(task->uuid);
		free(task);
	}
}

