#include "pthread.h"
#include "stdlib.h"
#include "stdio.h"
#include "errno.h"
#define NUMBER_OF_LINES 50
#define LINE_MAIN "-------------------"
#define LINE_SECONDARY "##################"

#define pthread_mutex_lock_err_proc(x) {\
    while ((errorCode = pthread_mutex_lock(x)) != 0) {\
        perror(errorCode);\
        if (errorCode != EAGAIN) {\
            pthread_exit((void*)-1);\
        }\
    }\
}
#define pthread_mutex_unlock_err_proc(x) {\
    while ((errorCode = pthread_mutex_unlock(x)) != 0) {\
        perror(errorCode);\
        if (errorCode != EAGAIN) {\
            pthread_exit((void*)-1);\
        }\
    }\
}

pthread_mutex_t mutex[3] = { PTHREAD_MUTEX_INITIALIZER };

void *threadFunction(void *arg) {
    int i, errorCode;
    pthread_mutex_lock_err_proc(&mutex[2]);
    for (i = 0; i < NUMBER_OF_LINES; i++) {
        pthread_mutex_lock_err_proc(&mutex[0]);
        pthread_mutex_unlock_err_proc(&mutex[2]);
        pthread_mutex_lock_err_proc(&mutex[1]);
        pthread_mutex_unlock_err_proc(&mutex[0]);
        printf("%s\n", LINE_SECONDARY);
        pthread_mutex_lock_err_proc(&mutex[2]);
        pthread_mutex_unlock_err_proc(&mutex[1]);
    }
}

int main(int argc, char *argv[]) {
    int errorCode;
    pthread_t thread;
    pthread_mutex_lock_err_proc(&mutex[0]);
    pthread_mutex_lock_err_proc(&mutex[1]);

    int err = errno;
    do {
        errorCode = pthread_create(&thread, NULL, threadFunction, NULL);
        if (errorCode == -1) {
            fprintf(stderr, "Error: pthread_create() for %s failed.  errno:%d\n", err);
        }
    } while (errorCode == -1 && err == EAGAIN);

    int i;
    while (pthread_mutex_trylock(&mutex[2]) == 0) {
        pthread_mutex_unlock_err_proc(&mutex[2]);
    }

    for (i = 0; i < NUMBER_OF_LINES; i++) {
        pthread_mutex_unlock_err_proc(&mutex[0]);
        printf("%s\n", LINE_MAIN);
        pthread_mutex_lock_err_proc(&mutex[2]);
        pthread_mutex_unlock_err_proc(&mutex[1]);
        pthread_mutex_lock_err_proc(&mutex[0]);
        pthread_mutex_unlock_err_proc(&mutex[2]);
        pthread_mutex_lock_err_proc(&mutex[1]);
    }
    pthread_exit(NULL);
}
