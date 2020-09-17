#include <stdio.h>
#include <pthread.h>

pthread_cond_t cond;
pthread_mutex_t mutex;

void printTenLines(char *line) {
    pthread_mutex_lock(&mutex);
    int i = 0;
    for (; i < 10; i++) {
        pthread_cond_signal(&cond);
        printf("%s\n", line);
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);
}

void *printLinesInNewThread(void *parameters) {
    printTenLines("I am a new thread...");
}

int main(int argc, char **argv) {
    pthread_t thread;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    int createThreadStatus = pthread_create(&thread, NULL, printLinesInNewThread, NULL);

    if (createThreadStatus != 0) {
        printf("Main thread: can't create thread, status=%d", createThreadStatus);
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&mutex);
    int i = 0;
    for (; i < 10; i++) {
        pthread_cond_signal(&cond);
        printf("%s\n", "I am a main thread...");
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
}
