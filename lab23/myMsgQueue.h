#include <iostream>
#include <semaphore.h>
#include "queue.h"

class MyMsgQueue {
    queue* q;
    sem_t first_sem, last_sem, queue_sem;
    bool destroying;
public:
    explicit MyMsgQueue();

    virtual ~MyMsgQueue();

    int put(char* msg);

    int get(char *buffer, int buffer_len);

    void drop();
};

