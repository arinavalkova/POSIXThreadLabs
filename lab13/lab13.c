#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t semaphore1;
sem_t semaphore2;

void printTenLines(char *line) {
    int i = 0;
    for (; i < 10; i++) {
        sem_wait(&semaphore1);
        printf("%s\n", line);
        sem_post(&semaphore2);
    }
}

void *printLinesInNewThread(void *parameters) {
    printTenLines("I am a new thread...");
}

int main(int argc, char **argv) {
    pthread_t thread;
    sem_init(&semaphore1, 0, 0);
    sem_init(&semaphore2, 0, 1);

    int createThreadStatus = pthread_create(&thread, NULL, printLinesInNewThread, NULL);

    if (createThreadStatus != 0) {
        printf("Main thread: can't create thread, status=%d", createThreadStatus);
        pthread_exit(NULL);
    }

    int i = 0;
    for (; i < 10; i++) {
        sem_wait(&semaphore2);
        printf("%s\n", "I am a main thread...");
        sem_post(&semaphore1);
    }

    pthread_exit(NULL);
}
