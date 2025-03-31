#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdlib.h>
#include <stdio.h>

typedef struct queue_s {
	struct qnode_s *head;
	struct qnode_s *tail;
	int size;
} queue_t;

struct qnode_s
{
	void *data;
	struct qnode_s* next;
};

queue_t *qcreate(void);
int qsize(queue_t *q);
void *qfront(queue_t *q);
void qpop(queue_t *q);
void qpush(queue_t *q, void *d);
void qclear(queue_t **q);
void qprint_int(queue_t *q);
void qprint_str(queue_t *q);

#endif//_QUEUE_H_
