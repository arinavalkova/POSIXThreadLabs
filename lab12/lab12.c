#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#define pthread_mutex_lock_err_proc(x) {\
    int errorCode;\
    while ((errorCode = pthread_mutex_lock(x)) != 0) {\
        perror(errorCode);\
        if (errorCode != EAGAIN) {\
            pthread_exit((void*)-1);\
        }\
    }\
}
#define pthread_mutex_unlock_err_proc(x) {\
    int errorCode;\
    while ((errorCode = pthread_mutex_unlock(x)) != 0) {\
        perror(errorCode);\
        if (errorCode != EAGAIN) {\
            pthread_exit((void*)-1);\
        }\
    }\
}

pthread_cond_t cond;
pthread_mutex_t mutex;

void printTenLines(char *line) {
    pthread_mutex_lock_err_proc(&mutex);
    int i = 0, err;
    for (; i < 10; i++) {
        if (pthread_cond_signal(&cond) == -1) {
            fprintf(stderr, "pthread_cond_signal() failed. errno:%d\n", err);
            pthread_exit(NULL);
        }
        printf("%s\n", line);
        if (pthread_cond_wait(&cond, &mutex) == -1) {
            fprintf(stderr, "pthread_cond_wait() failed. errno:%d\n", err);
            pthread_exit(NULL);
        }
    }
    pthread_mutex_unlock_err_proc(&mutex);
}

void *printLinesInNewThread(void *parameters) {
    printTenLines("I am a new thread...");
}

int main(int argc, char **argv) {
    pthread_t thread;
    int mutex_stat = pthread_mutex_init(&mutex, NULL);
    if (mutex_stat != 0) {
        int err = errno;
        fprintf(stderr, "pthread_mutex_init() failed. errno:%d\n", err);
        fprintf(stderr, "Error : incorrect program execution \n");
        exit(EXIT_FAILURE);
    }

    int cond_stat = pthread_cond_init(&cond, NULL);
    if (cond_stat != 0) {
        int err = errno;
        fprintf(stderr, "pthread_cond_init() failed. errno:%d\n", err);
        fprintf(stderr, "Error : incorrect program execution \n");
        exit(EXIT_FAILURE);
    }

    int err = errno, errorCode;
    do {
        errorCode = pthread_create(&thread, NULL, printLinesInNewThread, NULL);
        if (errorCode == -1) {
            fprintf(stderr, "Error: pthread_create() for %s failed.  errno:%d\n", err);
        }
    } while (errorCode == -1 && err == EAGAIN);

    pthread_mutex_lock_err_proc(&mutex);
    int i = 0;
    for (; i < 10; i++) {
        if (pthread_cond_signal(&cond) == -1) {
            fprintf(stderr, "pthread_cond_signal() failed. errno:%d\n", err);
            pthread_exit(NULL);
        }
        printf("%s\n", "I am a main thread...");
        if (pthread_cond_wait(&cond, &mutex) == -1) {
            fprintf(stderr, "pthread_cond_wait() failed. errno:%d\n", err);
            pthread_exit(NULL);
        }
    }
    pthread_mutex_unlock_err_proc(&mutex);
    if (pthread_cond_signal(&cond) == -1) {
        fprintf(stderr, "pthread_cond_signal() failed. errno:%d\n", err);
        pthread_exit(NULL);
    }

    if (pthread_cond_destroy(&cond) == -1) {
        fprintf(stderr, "pthread_cond_destroy() failed. errno:%d\n", err);
    }
    if (pthread_mutex_destroy(&mutex) == -1) {
        fprintf(stderr, "pthread_mutex_destroy() failed. errno:%d\n", err);
    }
    pthread_exit(NULL);
}
