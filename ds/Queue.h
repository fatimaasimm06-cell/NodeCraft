// Queue.h
// Fixed-capacity LINEAR array queue (capacity 8). First In, First Out.
//
// front and rear only ever move to the right. Once rear reaches the last slot
// the queue is "full", even if slots on the left were freed by dequeues.
// This waste is intentional: it is the problem the circular queue solves (CQ-5).
#pragma once

#include "TrackedStructure.h"

class Queue : public TrackedStructure {
public:
    static const int CAPACITY = NC_CAPACITY;

    Queue();

    OpResult enqueue(int value);  // QU-1
    OpResult dequeue();           // QU-2
    OpResult reset();             // QU-5

    bool isEmpty() const;
    bool isFull() const;          // rear has reached slot 7
    int size() const;
    int capacity() const;
    int getFront() const;         // index of the front item, -1 if empty
    int getRear() const;          // index of the rear item, -1 if empty

    std::vector<QueueSlot> getSlots() const;  // all 8 slots
    std::vector<int> getItems() const;        // front -> rear

private:
    int items_[CAPACITY];
    int front_;  // next item to leave
    int rear_;   // last item that joined, -1 before the first enqueue
};
