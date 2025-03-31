#include "thread_queue.h"

static task_queue_t *tq = NULL;
static pthread_t *threads = NULL;

void init_thread_queue()
{
	tq = (task_queue_t *)malloc(sizeof(task_queue_t));
	if (!tq) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}

	tq->queue = (task_t *)malloc(sizeof(task_t) * MAX_QUEUE_SIZE);
	if (!tq->queue) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}

	tq->front = 0;
	tq->rear = 0;
	tq->count = 0;
	pthread_mutex_init(&tq->lock, NULL);
	pthread_cond_init(&tq->not_empty, NULL);
	pthread_cond_init(&tq->not_full, NULL);
	tq->shutdown = false;

	// thread pool
	threads = malloc(sizeof(pthread_t) * MAX_CPU);
	if (!threads) {
		pr_err("malloc() failed.");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < MAX_CPU; i+=1) {
		pthread_create(&threads[i], NULL, worker_thread, NULL);
	}
}

void destroy_thread_queue()
{
	if (!tq) {
		return;
	}
	pthread_mutex_lock(&tq->lock);
	tq->shutdown = true;
	pthread_cond_broadcast(&tq->not_empty);
	pthread_cond_broadcast(&tq->not_full);
	pthread_mutex_unlock(&tq->lock);

	while (tq->count > 0) {
        task_t t;
        dequeue_task(&t);
        if (t.task_function == process_retry_task) {
            free(t.arg);  // release retry_task_t
        }
    }

	for (int i = 0; i < MAX_CPU; i+=1) {
		pthread_join(threads[i], NULL);
	}

	pthread_mutex_destroy(&tq->lock);
	pthread_cond_destroy(&tq->not_empty);
	pthread_cond_destroy(&tq->not_full);
	free(tq->queue);
	free(threads);
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
   from dequeue_task(task_t *t), if t is "task_t *", it should take deep copy
 */
//void task_deep_copy_txn(task_t *dest, const task_t *src) {
//    dest->func = src->func;
//    if (src->arg != NULL) {
//        dest->arg = malloc(sizeof(Transaction));
//        memcpy(dest->arg, src->arg, sizeof(Transaction));
//    } else {
//        dest->arg = NULL;
//    }
//}

bool dequeue_task(task_t *t)
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
		task_t t;
		if (!dequeue_task(&t)) {
			break;
		}
		t.task_function(t.arg);
	}
	return NULL;
}

void enqueue_retry_task(const char *uuid, int initial_backoff_ms)
{
    retry_task_t *task;
	MALLOC(task, sizeof(retry_task_t));
    strncpy(task->uuid, uuid, sizeof(task->uuid));
    task->retry_count = 0;
    task->next_retry_time = time(NULL) + (initial_backoff_ms / 1000);

    // push retry task into the thread queue
    enqueue_task(process_retry_task, task);
}

void process_retry_task(void *arg)
{
	retry_task_t *task = (retry_task_t *)arg;
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
			pr_err("Max retries exceeded for order %s", task->uuid);
			FREE(arg);
		}
	} else { // COMPLETED
		remove_order(task->uuid);
		FREE(arg);
	}
}

