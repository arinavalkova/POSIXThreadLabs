#ifndef QUEUE_BUFFER_H_
#define QUEUE_BUFFER_H_

#include "queue.h"

#if defined(MULTITHREAD) || defined (THREADPOOL)
#define QUEUE_BUFFER_INITIALIZER { NULL, NULL, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER }
#else
#define QUEUE_BUFFER_INITIALIZER { NULL, NULL, 0, 0 }
#endif


struct queuebuffer_node {
    struct queuebuffer_node *next;
    int data_len;
    char line[];
};

struct queuebuffer {
    struct queuebuffer_node *first, *last;
    int finished;
    int offset;
#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
};


int queuebuffer_init(struct queuebuffer *queuebuffer) {
#if defined(MULTITHREAD) || defined (THREADPOOL)
    int res;
    if ((res = pthread_mutex_init(&queuebuffer->mutex, NULL)) != 0)
        return res;
    if ((res = pthread_cond_init(&queuebuffer->cond, NULL)) != 0)
        return res;
#endif
    queuebuffer->first = NULL;
    queuebuffer->last = NULL;
    queuebuffer->finished = 0;
    queuebuffer->offset = 0;
    return 0;
}

int queuebuffer_add_bytes(struct queuebuffer *queuebuffer, char *line, int len) {
    struct queuebuffer_node *node = (struct queuebuffer_node *) malloc(
            sizeof(struct queuebuffer_node) + sizeof(char) * len);
    if (node == NULL) return -1;

    memcpy(node->line, line, len);
    node->data_len = len;
    node->next = NULL;

#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_mutex_lock(&queuebuffer->mutex);
#endif
    if (queuebuffer->first == NULL) {
        queuebuffer->last = node;
        queuebuffer->first = node;
    } else {
        queuebuffer->last->next = node;
        queuebuffer->last = node;
    }
#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_cond_broadcast(&queuebuffer->cond);
    pthread_mutex_unlock(&queuebuffer->mutex);
#endif
    return 0;
}

void queuebuffer_get_bytes(struct queuebuffer *queuebuffer, char **byte_dest, int *len) {
    struct queuebuffer_node *node;
#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_mutex_lock(&queuebuffer->mutex);
    while (queuebuffer->first == NULL && !queuebuffer->finished) {
        pthread_cond_wait(&queuebuffer->cond, &queuebuffer->mutex);
    }
#endif
    node = queuebuffer->first;
    if (node == NULL) {
        *byte_dest = NULL;
        *len = -1;
    } else {
        *byte_dest = node->line + queuebuffer->offset;
        *len = node->data_len - queuebuffer->offset;
    }
#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_mutex_unlock(&queuebuffer->mutex);
#endif
}

int queuebuffer_add_handled_bytes(struct queuebuffer *queuebuffer, int bytes_count) {
    struct queuebuffer_node *node = queuebuffer->first;
    if (node == NULL) return -1;

    queuebuffer->offset += bytes_count;
    if (queuebuffer->offset > node->data_len) return -1;

    if (queuebuffer->offset == node->data_len) {
#if defined(MULTITHREAD) || defined (THREADPOOL)
        pthread_mutex_lock(&queuebuffer->mutex);
#endif
        queuebuffer->first = node->next;
#if defined(MULTITHREAD) || defined (THREADPOOL)
        pthread_mutex_unlock(&queuebuffer->mutex);
#endif
        queuebuffer->offset = 0;
        free(node);
    }
    return 0;
}

int queuebuffer_is_empty(struct queuebuffer *queuebuffer) {
#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_mutex_lock(&queuebuffer->mutex);
#endif
    int res = queuebuffer->first == NULL;
#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_mutex_unlock(&queuebuffer->mutex);
#endif
    return res;
}

void queuebuffer_clear(struct queuebuffer *queuebuffer) {
    while (queuebuffer->first != NULL) {
        struct queuebuffer_node *node = queuebuffer->first;
        queuebuffer->first = node->next;
        free(node);
    }
}

void queuebuffer_finish(struct queuebuffer *queuebuffer) {
#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_mutex_lock(&queuebuffer->mutex);
#endif
    queuebuffer->finished = 1;
#if defined(MULTITHREAD) || defined (THREADPOOL)
    pthread_cond_broadcast(&queuebuffer->cond);
    pthread_mutex_unlock(&queuebuffer->mutex);
#endif
}

#endif //QUEUE_BUFFER_H_
