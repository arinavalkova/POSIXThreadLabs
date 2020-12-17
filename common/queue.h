#ifndef QUEUE_ZHIGALOV
#define QUEUE_ZHIGALOV

#include <stdlib.h>

struct queue_node;

#define QUEUE_INITIALIZER { NULL, NULL }

struct queue {
    struct queue_node *first, *last;
};

int queue_is_empty(struct queue *queue);
void queue_init(struct queue *queue);
int queue_add(struct queue *queue, void *val);
void* queue_peek(struct queue *queue);
void* queue_pop(struct queue *queue);
void queue_clear(struct queue *queue, void (*free_element)(void*));

#endif //QUEUE_ZHIGALOV
