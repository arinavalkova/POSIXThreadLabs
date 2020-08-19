#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define num_steps 200000000

int countOfThreads;
double* partOfPi;

void* calculatePartOfPi(void* parameters)
{
    int rank = *(int*) parameters, i = rank;

    for (; i < num_steps; i += countOfThreads)
    {
        partOfPi[rank] += 1.0 / (i * 4.0 + 1.0);
        partOfPi[rank] -= 1.0 / (i * 4.0 + 3.0);
    }

    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    double pi = 0;
    pthread_t* thread;
    int* rank;
    int i = 0;

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
        printf("Can't allocate memory");
        exit(EXIT_FAILURE);
    }

    for(; i < countOfThreads; i++)
    {
        rank[i] = i;
        pthread_create(&thread[i], NULL, calculatePartOfPi, &rank[i]);
    }

    i = 0;
    for(; i < countOfThreads; i++)
    {
        pthread_join(thread[i], NULL);
        pi += partOfPi[i];
    }

    pi = pi * 4.0;
    printf("pi done - %.15g \n", pi);
    
    free(thread);
    free(partOfPi);
    free(rank);

    return (EXIT_SUCCESS);
}

