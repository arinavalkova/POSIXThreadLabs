#include <iostream>
#include <semaphore.h>
#include "queue.h"

class MyMsgQueue {
    struct queue *next, *prev;
    sem_t head_sem, tail_sem, queue_sem;
    bool destroying;
public:
    explicit MyMsgQueue();

    virtual ~MyMsgQueue();

    int put(char* msg);

    int get(char *buffer, int buffer_len);
};

