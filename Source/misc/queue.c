#include "queue.h"

#include "common.h"
#include "var_type.h"

#define MAX_QUEUE_SIZE 25

Queue_t *QUEUE_malloc(DESTROY_HANDLER fn)
{
	Queue_t *queue = (Queue_t *)malloc(sizeof(Queue_t));
	queue->size = 0;
	queue->head = NULL;
	queue->tail = NULL;
	queue->destroy = fn;

	return queue;
}

BOOL QUEUE_push(Queue_t *queue, void *data)
{
	if (queue->size == MAX_QUEUE_SIZE) {
		return FALSE;
	} else {
		Node_t *newNode = (Node_t *)malloc(sizeof(Node_t));
		if (newNode == NULL)
			return FALSE;

		newNode->data = data;
		newNode->prev = NULL;
		newNode->next = NULL;
		if (queue->head == NULL && queue->tail == NULL)
			queue->head = queue->tail = newNode;
		else {
			queue->tail->next = newNode;
			newNode->prev = queue->tail;
			queue->tail = newNode;
		}
		queue->size++;
		return TRUE;
	}
}

void QUEUE_pop(Queue_t *queue)
{
	if (queue->size > 0) {
		Node_t *removeNode = queue->head;
		if (removeNode == queue->tail) {
			queue->head = NULL;
			queue->tail = NULL;
		} else {
			queue->head = removeNode->next;
			queue->head->prev = NULL;
		}

		if (queue->destroy != NULL) {
			queue->destroy(removeNode->data);
		}
		free(removeNode->data);
		free(removeNode);
		queue->size--;
	}
}

void *QUEUE_front(Queue_t *queue)
{
	return queue->head->data;
}

void *QUEUE_back(Queue_t *queue)
{
	return queue->tail->data;
}

void *QUEUE_get(Queue_t *queue, int index)
{
	if (index < queue->size) {
		Node_t *current = queue->head;
		while (index--) {
			current = current->next;
		}
		return current->data;
	} else {
		return NULL;
	}
}

int QUEUE_size(Queue_t *queue)
{
	return queue->size;
}

BOOL QUEUE_isEmpty(Queue_t *queue)
{
	return (queue->size == 0) ? TRUE : FALSE;
}

BOOL QUEUE_isFull(Queue_t *queue)
{
	return (queue->size == MAX_QUEUE_SIZE) ? TRUE : FALSE;
}
