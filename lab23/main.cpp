#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "myMsgQueue.h"

#define END "Threads already joined\n"

void* prod_func(void* args) {
    auto* myMsgQue = (MyMsgQueue*)(args);
    return 0;
}

void* cons_func(void* args) {
    auto* myMsgQue = (MyMsgQueue*)(args);
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
        exit(-1);
    }
    prod_count = atoi(argv[1]);
    cons_count = atoi(argv[2]);
    if (prod_count <= 0 || cons_count <= 0) {
        fprintf(stderr, "Usage: %s producers consumers\n", argv[0]);
        exit(-1);
    }

    prod_pthread = (pthread_t*) malloc(prod_count * sizeof(pthread_t));
    cons_pthread = (pthread_t*) malloc(cons_count * sizeof(pthread_t));
    if (prod_pthread == NULL || cons_pthread == NULL) {
        fprintf(stderr, "Error: malloc() is failed\n");
        exit(-1);
    }

    MyMsgQueue myMsgQueue(5);

    for (int i = 0; i < prod_count; i++) {
        pthread_create_err_proc(prod_pthread + i, NULL, prod_func, &myMsgQueue);
    }
    for (int i = 0; i < cons_count; i++) {
        pthread_create_err_proc(cons_pthread + i, NULL, cons_func, &myMsgQueue);
    }

    for (int i = 0; i < prod_count; i++) {
        pthread_join_err_proc(*(prod_pthread + i), NULL);
    }
    for (int i = 0; i < cons_count; i++) {
        pthread_join_err_proc(*(cons_pthread + i), NULL);
    }
    write(1, END, strlen(END));
    return 0;
}
