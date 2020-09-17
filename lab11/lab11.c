#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

void printTenLines(char *line) {
    int i = 0;
    for (; i < 10; i++) {
        pthread_mutex_lock(&mutex1);
        printf("%s\n", line);
        pthread_mutex_unlock(&mutex2);
    }
}

void *printLinesInNewThread(void *parameters) {
    printTenLines("I am a new thread...");
}

int main(int argc, char **argv) {
    pthread_t thread;

    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);

    int createThreadStatus = pthread_create(&thread, NULL, printLinesInNewThread, NULL);

    if (createThreadStatus != 0) {
        printf("Main thread: can't create thread, status=%d", createThreadStatus);
        pthread_mutex_destroy(&mutex2);
        pthread_mutex_destroy(&mutex1);
        pthread_exit(NULL);
    }

    int i = 0;
    for (; i < 10; i++) {
        pthread_mutex_lock(&mutex2);
        printf("%s\n", "I am a main thread...");
        pthread_mutex_unlock(&mutex1);
    }

    pthread_mutex_destroy(&mutex2);
    pthread_mutex_destroy(&mutex1);

    pthread_exit(NULL);
}
