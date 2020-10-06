#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

sem_t semaphore1;
sem_t semaphore2;

int sem_wait_err_proc(sem_t *sem) {
    int stat = sem_wait(sem);
    if (stat == -1) {
        int err = errno;
        fprintf(stderr, "sem_wait() failed. errno:%d\n", err);
        fprintf(stderr, "Error : incorrect program execution \n");
    }
    return stat;
}

int sem_post_err_proc(sem_t *sem) {
    int stat = sem_post(sem);
    if (stat == -1) {
        int err = errno;
        fprintf(stderr, "sem_post() failed. errno:%d\n", err);
        fprintf(stderr, "Error : incorrect program execution \n");
    }
    return stat;
}

void printTenLines(char *line) {
    int i = 0;
    for (; i < 10; i++) {
        if(sem_wait_err_proc(&semaphore1) == -1) {
            pthread_exit(NULL);
        }
        printf("%s\n", line);
        if(sem_post_err_proc(&semaphore2) == -1) {
            pthread_exit(NULL);
        }
    }
}

void sem_destroy_err_proc(sem_t *sem) {
    if (sem_destroy(sem) == -1) {
        int err = errno;
        fprintf(stderr, "sem_destroy() failed. errno:%d\n", err);
    }
}

void *printLinesInNewThread(void *parameters) {
    printTenLines("I am a new thread...");
}

int main(int argc, char **argv) {
    pthread_t thread;
    int stat1 = sem_init(&semaphore1, 0, 0);
    int stat2 = sem_init(&semaphore2, 0, 1);

    if (stat1 == -1 || stat2 == -1) {
        int err = errno;
        fprintf(stderr, "sem_init() failed. errno:%d\n", err);
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

    int i = 0;
    for (; i < 10; i++) {
        if(sem_wait_err_proc(&semaphore2) == -1) {
            pthread_exit(NULL);
        }
        printf("%s\n", "I am a main thread...");
        if(sem_post_err_proc(&semaphore1) == -1) {
            pthread_exit(NULL);
        }
    }

    if(pthread_join(thread, NULL) != 0) {
        int err = errno;
        fprintf(stderr, "sem_init() failed. errno:%d\n", err);
    }

    sem_destroy_err_proc(&semaphore1);
    sem_destroy_err_proc(&semaphore2);
    pthread_exit(NULL);
}
