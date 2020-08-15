#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ERROR_CREATE_THREAD 1

void printTenLines(char *line);

void* printLinesInNewThread(void* parameters)
{
    printTenLines("I am a new thread...");

    return EXIT_SUCCESS;
}

void printTenLines(char *line)
{
    for(int i = 0; i < 10; i++)
    {
        printf("%s\n", line);
    }
}

int main(int argc, char** argv)
{
    pthread_t thread;

    int createThreadStatus = pthread_create(&thread, NULL, printLinesInNewThread, NULL);
    if(createThreadStatus != 0)
    {
        printf("Main thread: can't create thread, status=%d", createThreadStatus);
        exit(ERROR_CREATE_THREAD);
    }

    printTenLines("I am a main thread...");

    pthread_exit(NULL);
    exit(EXIT_SUCCESS);
}
