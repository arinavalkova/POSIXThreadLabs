#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define ERROR_CREATE_THREAD 1

void *printLinesInNewThread(void *parameters)
{
    while (1)
    {
        pthread_testcancel();
        printf("I am a new thread...\n");
    }

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    pthread_t thread;

    int createThreadStatus = pthread_create(&thread, NULL, printLinesInNewThread, NULL);
    if (createThreadStatus != 0)
    {
        printf("Main thread: can't create thread, status=%d", createThreadStatus);
        exit(ERROR_CREATE_THREAD);
    }

    sleep(2);

    printf("Before canceling...\n");
    int cancelThreadStatus = pthread_cancel(thread);
    if(cancelThreadStatus != 0)
    {
        printf("Main thread: can't cancel thread, status=%d", cancelThreadStatus);
        exit(ERROR_CREATE_THREAD);
    }

    int joiningThreadStatus = pthread_join(thread, NULL);
    if(joiningThreadStatus != 0)
    {
        printf("Main thread: can't join thread, status=%d", joiningThreadStatus);
        exit(ERROR_CREATE_THREAD);
    }

    printf("After canceling...\n");

    pthread_exit(NULL);
    exit(EXIT_SUCCESS);
}
