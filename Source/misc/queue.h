#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "var_type.h"

typedef struct Node {
	void *data;
	struct Node *prev;
	struct Node *next;
} Node_t;

typedef void (*DESTROY_HANDLER)(void *);
typedef struct Queue {
	int size;
	struct Node *head;
	struct Node *tail;
	DESTROY_HANDLER destroy;
} Queue_t;

Queue_t *QUEUE_malloc(DESTROY_HANDLER fn);

BOOL QUEUE_push(Queue_t *queue, void *data);
void QUEUE_pop(Queue_t *queue);
void *QUEUE_front(Queue_t *queue);
void *QUEUE_back(Queue_t *queue);
void *QUEUE_get(Queue_t *queue, int index);
int QUEUE_size(Queue_t *queue);
BOOL QUEUE_isEmpty(Queue_t *queue);
BOOL QUEUE_isFull(Queue_t *queue);

#endif /* __QUEUE_H__ */
