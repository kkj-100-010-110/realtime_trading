#include "queue.h"

queue_t *qcreate(void)
{
	queue_t *new_q = (queue_t*)malloc(sizeof(queue_t));
	if (!new_q) {
		fprintf(stderr, "malloc failed in qcreate()\n");
		return NULL;
	}

	new_q->head = NULL;
	new_q->tail = NULL;
	new_q->size = 0;
	
	return new_q;
}

int qsize(queue_t *q)
{
	if (!q) {
		fprintf(stderr, "Error: queue_t is null in qsize()\n");
		return -1;
	}
	return q->size;
}

void *qfront(queue_t *q)
{
	if (!q) {
		fprintf(stderr, "Error: queue_t is null in qfront()\n");
		return NULL;
	}
	return q->head->data;
}

void qpop(queue_t *q)
{
	if (!q) {
		fprintf(stderr, "Error: queue_t is null in qpop()\n");
		return;
	}
	if (!q->head) {
		fprintf(stderr, "Error: queue_t is empty in qpop()\n");
		return;
	}
	struct qnode_s *tmp = q->head;
	q->head = q->head->next;
	q->size--;
	free(tmp);
	tmp = NULL;
}

void qpush(queue_t *q, void *d)
{
	struct qnode_s *new_q = (struct qnode_s*)malloc(sizeof(struct qnode_s));
	if (!new_q) {
		fprintf(stderr, "malloc failed in qpush()\n");
		return;
	}
	new_q->data = d;
	new_q->next = NULL;
	if (!q->head) {
		q->head = new_q;
		q->tail = new_q;
	} else {
		q->tail->next = new_q;
		q->tail = q->tail->next;
	}
	q->size++;
}

void qclear(queue_t **q)
{
	if (!*q) {
		printf("empty\n");
		return;
	}
	while ((*q)->size)
		qpop(*q);
	free(*q);
	*q = NULL;
}

void qprint_int(queue_t *q)
{
	if (!q) {
		fprintf(stderr, "error: queue is null\n");
		return ;
	}
	struct qnode_s *tmp = q->head;
	while (tmp)
	{
		printf("%d ", *(int*)tmp->data);
		tmp = tmp->next;
	}
}

void qprint_str(queue_t *q)
{
	if (!q) {
		fprintf(stderr, "error: queue is null\n");
		return ;
	}
	struct qnode_s *tmp = q->head;
	while (tmp)
	{
		printf("%s\n", (char*)tmp->data);
		tmp = tmp->next;
	}
}
