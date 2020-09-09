#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void printTenLines(char *line)
{
    int i = 0;
    for(; i < 10; i++)
    {
        printf("%s\n", line);
    }
}

void* printLinesInNewThread(void* parameters)
{
    printTenLines("I am a new thread...");
}

int main(int argc, char** argv)
{
    pthread_t thread;
    int createThreadStatus, joiningThreadStatus;

    createThreadStatus = pthread_create(&thread, NULL, printLinesInNewThread, NULL);
    if(createThreadStatus != 0)
    {
        printf("Main thread: can't create thread, status=%d", createThreadStatus);
        exit(EXIT_FAILURE);
    }

    joiningThreadStatus = pthread_join(thread, NULL);
    if(joiningThreadStatus != 0)
    {
        printf("Main thread: can't join thread, status=%d", joiningThreadStatus);
        exit(EXIT_FAILURE);
    }

    printTenLines("I am a main thread...");

    pthread_exit(NULL);
}
