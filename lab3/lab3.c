#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define COUNT_OF_THREADS 4
#define MAX_COUNT_OF_LINES 5

const char* linesForPrintInThreads[][MAX_COUNT_OF_LINES] =
        {
                {"line1 in thread1...", "line2 in thread2...", "line3 in thread3...", "line4 in thread4...", NULL},
                {"line1 in thread2...", "line2 in thread2...", NULL},
                {"line1 in thread3...", NULL},
                {"line1 in thread4...", "line2 in thread4...", "line3 in thread4...", NULL}
        };

void* printLinesInNewThread(void* parameters)
{
    char** arrayOfLines;

    for(arrayOfLines = (char**)parameters; *arrayOfLines != NULL; arrayOfLines++)
    {
        printf("%s\n", *arrayOfLines);
    }

    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    pthread_t thread[COUNT_OF_THREADS];
    int createThreadStatus, i;


    for(i = 0; i < COUNT_OF_THREADS; i++)
    {
        createThreadStatus = pthread_create(&thread[i], NULL, printLinesInNewThread, linesForPrintInThreads[i]);
        if(createThreadStatus != 0)
        {
            printf("Main thread: can't create %d thread, status=%d", i, createThreadStatus);
            exit(EXIT_FAILURE);
        }
    }

    pthread_exit(NULL);
}
