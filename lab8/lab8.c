#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#define num_global_iteration 1000000

int countOfThreads;
int stopCalculating = 0;
int maxGlobalIteration = 0;
double* partOfPi;

pthread_mutex_t mutex;

void* calculatePartOfPi(void* parameters)
{
    int rank = *(int*) parameters;
    int currentGlobalIteration = 0;

    while(1)
    {
        int i = rank + currentGlobalIteration * num_global_iteration;
        for (; i < (currentGlobalIteration + 1) * num_global_iteration; i += countOfThreads)
        {
            partOfPi[rank] += 1.0 / (i * 4.0 + 1.0);
            partOfPi[rank] -= 1.0 / (i * 4.0 + 3.0);
        }

        pthread_mutex_lock(&mutex);

        if(stopCalculating && currentGlobalIteration == maxGlobalIteration)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        currentGlobalIteration++;

        if(currentGlobalIteration > maxGlobalIteration)
            maxGlobalIteration = currentGlobalIteration;

        pthread_mutex_unlock(&mutex);
    }

    return EXIT_SUCCESS;
}

void stopCalculateParts(int parameters)
{
    printf("Caught interrupt signal... \n");
    printf("Starting preparing pi...\n");

    stopCalculating = 1;
}

int main(int argc, char** argv)
{
    double pi = 0;
    int i;
    pthread_t* thread;
    int* rank;

    signal(SIGINT, stopCalculateParts);

    if(argc < 2)
    {
        printf("Usage: %s countOfThreads\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    countOfThreads = atoi(argv[1]);
    if(countOfThreads < 1)
    {
        printf("Bad count of thread\n");
        exit(EXIT_FAILURE);
    }

    thread = (pthread_t*) malloc(countOfThreads * sizeof(pthread_t));
    partOfPi = (double*) calloc(countOfThreads, sizeof(double));
    rank = (int*) malloc(countOfThreads * sizeof(int));

    if(thread == NULL || partOfPi == NULL || rank == NULL)
    {
        printf("Can't allocate memory\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_mutex_init(&mutex,NULL) != 0)
    {
        printf("Can't init mutex\n");
        exit(EXIT_FAILURE);
    }

    printf("pi is calculating...\n");

    for(i = 0; i < countOfThreads; i++)
    {
        rank[i] = i;
        pthread_create(&thread[i], NULL, calculatePartOfPi, &rank[i]);
    }

    for(i = 0; i < countOfThreads; i++)
    {
        pthread_join(thread[i], NULL);
        pi += partOfPi[i];
    }

    pi = pi * 4.0;
    printf("pi done - %.15g \n", pi);

    pthread_mutex_destroy(&mutex);

    free(thread);
    free(partOfPi);
    free(rank);

    return (EXIT_SUCCESS);
}

