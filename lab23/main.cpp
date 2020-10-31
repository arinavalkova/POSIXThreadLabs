#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include "myMsgQueue.h"

#define MSG_LEN 40

void* prod_func(void* args) {
    auto* myMsgQue = (MyMsgQueue*)(args);
    char msg_buffer[MSG_LEN];
    int msg_counter = 0;
    while (true) {
        msg_counter++;
        sprintf(msg_buffer, "MSG %d: from prod - %d", msg_counter, pthread_self());
        if (myMsgQue->put(msg_buffer) == 0) {
            return 0;
        }
    }
}

void* cons_func(void* args) {
    auto* myMsgQue = (MyMsgQueue*)(args);
    char msg_buffer[MSG_LEN];
    while (true) {
        if(myMsgQue->get(msg_buffer, MSG_LEN) == 0) {
            return 0;
        } else {
            printf("Gotten by %d cons: %s\n", pthread_self(), msg_buffer);
        }
    }
    return 0;
}

void pthread_create_err_proc(pthread_t *thread,
        const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg) {
    int ret;
    do {
        ret = pthread_create(thread, attr, start_routine, arg);
        if (ret != 0) {
            fprintf(stderr, "Error: pthread_create() is failed with %s. errno: %d", ret);
        }
    } while (ret != 0 && ret == EAGAIN);
}

void pthread_join_err_proc(pthread_t thread, void **retval) {
    int ret = pthread_join(thread, retval);
    if (ret != 0) {
        fprintf(stderr, "Error: pthread_join() failed with %s. errno:%d", ret);
    }
}

int main(int argc, char** argv) {
    int prod_count, cons_count;
    pthread_t *prod_pthread, *cons_pthread;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s producers consumers\n", argv[0]);
        return -1;
    }
    prod_count = atoi(argv[1]);
    cons_count = atoi(argv[2]);
    if (prod_count <= 0 || cons_count <= 0) {
        fprintf(stderr, "Usage: %s producers consumers\n", argv[0]);
        return -1;
    }

    prod_pthread = (pthread_t*) malloc(prod_count * sizeof(pthread_t));
    cons_pthread = (pthread_t*) malloc(cons_count * sizeof(pthread_t));
    if (prod_pthread == NULL || cons_pthread == NULL) {
        fprintf(stderr, "Error: malloc() is failed\n");
        return -1;
    }

    MyMsgQueue myMsgQueue;

    for (int i = 0; i < prod_count; i++) {
        pthread_create_err_proc(prod_pthread + i, NULL, prod_func, &myMsgQueue);
    }
    for (int i = 0; i < cons_count; i++) {
        pthread_create_err_proc(cons_pthread + i, NULL, cons_func, &myMsgQueue);
    }

    sleep(5);
    myMsgQueue.drop();

    for (int i = 0; i < prod_count; i++) {
        pthread_join_err_proc(*(prod_pthread + i), NULL);
    }
    for (int i = 0; i < cons_count; i++) {
        pthread_join_err_proc(*(cons_pthread + i), NULL);
    }

    printf("Threads already joined\n");
    free(cons_pthread);
    free(prod_pthread);

    return 0;
}
