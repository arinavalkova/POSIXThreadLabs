#include "queue.h"

struct queue_node {
    struct queue_node *next, *prev;
    void *val;
};

void queue_init(struct queue *queue) {
    queue->first = queue->last = NULL;
}

int queue_is_empty(struct queue *queue){
    return queue->first == NULL;
}

int queue_add(struct queue *queue, void *val) {
    struct queue_node *new_node = (struct queue_node *)malloc(sizeof(struct queue_node));
    if (new_node == NULL) return -1;

    new_node->val = val;
    new_node->next = NULL;
    new_node->prev = queue->last;

    if (queue->first == NULL) {
        queue->last = queue->first = new_node;
    } else {
        queue->last->next = new_node;
        queue->last = new_node;
    }
    return 0;
}

void* queue_peek(struct queue *queue) {
    if (queue->first == NULL)
        return NULL;
    else
        return queue->first->val;
}

void* queue_pop(struct queue *queue) {
    void *val;
    struct queue_node *node = queue->first;
    if (queue->first == NULL)
        return NULL;

    val = node->val;
    if (queue->first == queue->last) {
        queue->first = queue->last = NULL;
    } else {
        queue->first->next->prev = NULL;
        queue->first = queue->first->next;
    }
    free(node);
    return val;
}

void queue_clear(struct queue *queue, void (*free_element)(void*)) {
    while (queue->first != NULL) {
        struct queue_node *node = queue->first;
        queue->first = node->next;
        free_element(node->val);
        free(node);
    }
}
