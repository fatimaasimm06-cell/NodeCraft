// CircularQueue.cpp
#include "CircularQueue.h"

CircularQueue::CircularQueue() : front_(0), rear_(CAPACITY - 1), count_(0) {
    for (int i = 0; i < CAPACITY; i++) {
        items_[i] = 0;
        used_[i] = false;
    }
}

OpResult CircularQueue::enqueue(int value) {
    const std::string arg = toText(value);
    if (isFull()) {
        return fail("Enqueue", arg, rear_, value, "Circular queue is full: all 8 slots are in use");
    }

    int oldRear = rear_;
    int slot = (rear_ + 1) % CAPACITY;   // wrap from 7 back to 0
    bool wrapped = (count_ > 0 && oldRear == CAPACITY - 1);
    bool reused = used_[slot];

    std::string msg;
    if (wrapped && reused) {
        msg = "Rear wraps around to slot 0 and reuses a freed slot";
    } else if (reused) {
        msg = "rear moves to slot " + toText(slot) + " and reuses a slot that was freed earlier";
    } else if (count_ == 0) {
        msg = "The queue is empty, so rear starts at slot " + toText(slot);
    } else {
        msg = "rear moves to slot " + toText(slot) + " using (rear + 1) % 8";
    }

    rear_ = slot;
    record("movePointer", rear_, value, "rear", msg);
    items_[rear_] = value;
    used_[rear_] = true;
    count_++;
    record("insert", rear_, value, "", "Place " + arg + " in slot " + toText(rear_));
    if (count_ == 1) {
        record("movePointer", front_, value, "front",
               "This is the only item, so front is also slot " + toText(front_));
    }
    return finish("Enqueue", arg, true, "Success");
}

OpResult CircularQueue::dequeue() {
    if (isEmpty()) {
        return fail("Dequeue", "", -1, 0, "Circular queue is empty: there is nothing to dequeue");
    }

    int value = items_[front_];
    std::string v = toText(value);
    int oldFront = front_;
    record("highlight", oldFront, value, "front",
           "The front item leaves first: " + v + " in slot " + toText(oldFront));
    record("remove", oldFront, value, "",
           "Remove " + v + ". Slot " + toText(oldFront) + " is free and can be reused later");
    front_ = (front_ + 1) % CAPACITY;
    count_--;

    if (count_ == 0) {
        record("movePointer", -1, 0, "front", "The circular queue is now empty");
    } else if (front_ == 0) {
        record("movePointer", front_, items_[front_], "front", "front wraps around to slot 0");
    } else {
        record("movePointer", front_, items_[front_], "front",
               "front moves to slot " + toText(front_) + " using (front + 1) % 8");
    }
    return finish("Dequeue", "", true, "Success (removed " + v + ")");
}

OpResult CircularQueue::reset() {
    front_ = 0;
    rear_ = CAPACITY - 1;
    count_ = 0;
    for (int i = 0; i < CAPACITY; i++) used_[i] = false;
    return finishReset("The circular queue was reset. It is empty and the log starts again.");
}

bool CircularQueue::isEmpty() const { return count_ == 0; }
bool CircularQueue::isFull() const { return count_ == CAPACITY; }
int CircularQueue::size() const { return count_; }
int CircularQueue::capacity() const { return CAPACITY; }
int CircularQueue::getFront() const { return isEmpty() ? -1 : front_; }
int CircularQueue::getRear() const { return isEmpty() ? -1 : rear_; }

std::vector<QueueSlot> CircularQueue::getSlots() const {
    std::vector<QueueSlot> out(CAPACITY);
    for (int i = 0; i < CAPACITY; i++) {
        out[i].index = i;
        out[i].used = used_[i];
    }
    // Walk from front for count_ items, wrapping around.
    for (int k = 0; k < count_; k++) {
        int i = (front_ + k) % CAPACITY;
        out[i].occupied = true;
        out[i].value = items_[i];
    }
    return out;
}

std::vector<int> CircularQueue::getItems() const {
    std::vector<int> out;
    for (int k = 0; k < count_; k++) out.push_back(items_[(front_ + k) % CAPACITY]);
    return out;
}
