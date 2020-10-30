#include <iostream>

class MyMsgQueue {
    int value;
public:
    explicit MyMsgQueue(int value);

    virtual ~MyMsgQueue();

    int getValue() const;
};

