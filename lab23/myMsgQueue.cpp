#include <unistd.h>
#include <cstring>
#include "myMsgQueue.h"
#include "queue.h"

#define DESTROY "MyMsgQueue resources was already destroyed\n"

MyMsgQueue::MyMsgQueue(int value) : value(value) {}

MyMsgQueue::~MyMsgQueue() {
    write(1, DESTROY, strlen(DESTROY));
}

int MyMsgQueue::getValue() const {
    return value;
}
