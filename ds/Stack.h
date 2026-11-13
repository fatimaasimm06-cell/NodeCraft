// Stack.h
// Fixed-capacity array stack (capacity 8). Last In, First Out.
#pragma once

#include "TrackedStructure.h"

class Stack : public TrackedStructure {
public:
    static const int CAPACITY = NC_CAPACITY;

    Stack();

    OpResult push(int value);  // ST-1
    OpResult pop();            // ST-2
    OpResult reset();          // ST-6, HL-6

    int peek() const;          // top value, or 0 if the stack is empty (not logged)
    int size() const;
    bool isEmpty() const;
    bool isFull() const;
    int getTop() const;        // index of the top slot, -1 if empty
    int capacity() const;

    std::vector<int> getItems() const;  // bottom -> top

private:
    int items_[CAPACITY];
    int top_;  // index of the top item, -1 when empty
};
