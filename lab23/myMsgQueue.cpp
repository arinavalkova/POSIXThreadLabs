#include <cstring>
#include "myMsgQueue.h"

void sem_wait_err_proc(sem_t *sem) {
    int ret;
    do {
        ret = sem_wait(sem);
        if (ret != 0) {
            fprintf(stderr, "sem_wait() failed with %s\n", strerror(ret));
        }
    } while (ret != 0 && ret == EINTR);
}

MyMsgQueue::MyMsgQueue() {
    q = (queue*) malloc(sizeof(queue));
    q->first = NULL;
    q->last = NULL;
    sem_init(&first_sem, 0, 10);
    sem_init(&last_sem, 0, 0);
    sem_init(&queue_sem, 0, 1);
    destroying = false;
}

MyMsgQueue::~MyMsgQueue() {
    sem_destroy(&first_sem);
    sem_destroy(&last_sem);
    sem_destroy(&queue_sem);
    printf("MyMsgQueue resources was already destroyed\n");
}

void MyMsgQueue::drop() {
    printf("Starting dropping MyMsgQueue\n");
    destroying = true;
    sem_wait_err_proc(&queue_sem);
    sem_post(&first_sem);
    sem_post(&last_sem);

    list* l1 = q->first;
    while (l1) {
        list* l2;
        l2 = l1->next;
        free(l1);
        l1 = l2;
    }
    free(q);
    sem_post(&queue_sem);
    printf("Ending of dropping MyMsgQueue\n");
}

int MyMsgQueue::put(char* msg) {
    list* l;
    sem_wait_err_proc(&first_sem);
    sem_wait_err_proc(&queue_sem);
    if (destroying) {
        sem_post(&first_sem);
        sem_post(&queue_sem);
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

    sem_post(&queue_sem);
    sem_post(&last_sem);

    return strlen(l->value) + 1;
}

int MyMsgQueue::get(char *buffer, int buffer_len) {
    list* l;
    sem_wait_err_proc(&last_sem);
    sem_wait_err_proc(&queue_sem);
    if (destroying) {
        sem_post(&last_sem);
        sem_post(&queue_sem);
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
    sem_post(&queue_sem);

    strncpy(buffer, l->value, buffer_len - 1);
    buffer[buffer_len - 1] = 0;
    free(l);

    sem_post(&first_sem);
    return strlen(buffer) + 1;
}
