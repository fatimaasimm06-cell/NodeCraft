// CircularQueue.h
// Fixed-capacity CIRCULAR array queue (capacity 8).
// front and rear wrap around with (index + 1) % 8 (CQ-3), so slots freed by
// dequeue are reused (CQ-5). A separate count keeps "full" and "empty" clear.
#pragma once

#include "TrackedStructure.h"

class CircularQueue : public TrackedStructure {
public:
    static const int CAPACITY = NC_CAPACITY;

    CircularQueue();

    OpResult enqueue(int value);
    OpResult dequeue();
    OpResult reset();

    bool isEmpty() const;
    bool isFull() const;
    int size() const;
    int capacity() const;
    int getFront() const;  // -1 if empty
    int getRear() const;   // -1 if empty

    std::vector<QueueSlot> getSlots() const;  // all 8 slots
    std::vector<int> getItems() const;        // front -> rear

private:
    int items_[CAPACITY];
    bool used_[CAPACITY];  // has this slot ever held a value (since last reset)?
    int front_;
    int rear_;
    int count_;
};
