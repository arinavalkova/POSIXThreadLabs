#include <unistd.h>
#include <cstring>
#include "myMsgQueue.h"

#define DESTROY "MyMsgQueue resources was already destroyed\n"

MyMsgQueue::MyMsgQueue() {}

MyMsgQueue::~MyMsgQueue() {
    write(1, DESTROY, strlen(DESTROY));
}

int MyMsgQueue::put(char* msg) {
    return 0;
}

int MyMsgQueue::get(char *buffer, int buffer_len) {
    return 0;
}
