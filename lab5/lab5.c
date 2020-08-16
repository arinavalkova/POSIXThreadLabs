#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void exitFunction(void* parameters)
{
    printf("I am cleanup function...\n");
}

void* printLinesInNewThread(void *parameters)
{
    pthread_cleanup_push(exitFunction, NULL);

    while (1)
    {
        pthread_testcancel();
        printf("I am a new thread...\n");
    }

    pthread_cleanup_pop(0);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    pthread_t thread;

    int createThreadStatus = pthread_create(&thread, NULL, printLinesInNewThread, NULL);
    if (createThreadStatus != 0)
    {
        printf("Main thread: can't create thread, status=%d", createThreadStatus);
        exit(EXIT_FAILURE);
    }

    sleep(2);

    printf("Before canceling...\n");
    int cancelThreadStatus = pthread_cancel(thread);
    if(cancelThreadStatus != 0)
    {
        printf("Main thread: can't cancel thread, status=%d", cancelThreadStatus);
        exit(EXIT_FAILURE);
    }

    int joiningThreadStatus = pthread_join(thread, NULL);
    if(joiningThreadStatus != 0)
    {
        printf("Main thread: can't join thread, status=%d", joiningThreadStatus);
        exit(EXIT_FAILURE);
    }

    printf("After canceling...\n");

    pthread_exit(NULL);
    exit(EXIT_SUCCESS);
}
