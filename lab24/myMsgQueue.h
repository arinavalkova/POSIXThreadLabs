#include <iostream>
#include <semaphore.h>
#include "queue.h"

class MyMsgQueue {
    queue* q;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool destroying;
public:
    explicit MyMsgQueue(pthread_mutex_t mutex, pthread_cond_t cond);

    virtual ~MyMsgQueue();

    int put(char* msg);

    int get(char *buffer, int buffer_len);

    void drop();
};

