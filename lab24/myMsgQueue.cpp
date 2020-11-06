#include <cstring>
#include "myMsgQueue.h"

MyMsgQueue::MyMsgQueue(pthread_mutex_t mutex, pthread_cond_t cond) {
    this->mutex = mutex;
    this->cond = cond;
    q = (queue*) malloc(sizeof(queue));
    q->first = NULL;
    q->last = NULL;
    count = 0;
    destroying = false;
}

MyMsgQueue::~MyMsgQueue() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    printf("MyMsgQueue resources was already destroyed\n");
}

void MyMsgQueue::drop() {
    printf("Starting dropping MyMsgQueue\n");
    destroying = true;

    pthread_cond_broadcast(&cond);
    pthread_mutex_lock(&mutex);

    list* l1 = q->first;
    while (l1) {
        list* l2;
        l2 = l1->next;
        free(l1);
        l1 = l2;
    }
    free(q);

    pthread_mutex_unlock(&mutex);
    printf("Ending of dropping MyMsgQueue\n");
}

int MyMsgQueue::put(char* msg) {
    list* l;

    pthread_mutex_lock(&mutex);
    if (count == 10 && !destroying) {
        pthread_cond_wait(&cond, &mutex);
    }

    if (destroying) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    l = (list*) malloc(sizeof(list));
    if (l == NULL) {
        drop();
        return 0;
    }

    strncpy(l->value, msg, sizeof(l->value) - 1);
    l->value[sizeof(l->value) - 1] = 0;
    l->prev = NULL;

    l->next = q->first;
    if (q->first != NULL) {
        q->first->prev = l;
        q->first = l;
    } else {
        q->first = l;
        q->last = l;
    }

    count++;
    if (count > 0) {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);

    return strlen(l->value) + 1;
}

int MyMsgQueue::get(char *buffer, int buffer_len) {
    list* l;
    pthread_mutex_lock(&mutex);
    if (count == 0 && !destroying) {
        pthread_cond_wait(&cond, &mutex);
    }

    if (destroying) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    l = q->last;
    if (q->first != l) {
        q->last = l->prev;
        q->last->next = NULL;
    } else {
        q->first = NULL;
        q->last = NULL;
    }

    count--;
    if (count < 10) {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);

    strncpy(buffer, l->value, buffer_len - 1);
    buffer[buffer_len - 1] = 0;
    free(l);

    return strlen(buffer) + 1;
}
