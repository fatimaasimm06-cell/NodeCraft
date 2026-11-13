// Queue.cpp
#include "Queue.h"

Queue::Queue() : front_(0), rear_(-1) {
    for (int i = 0; i < CAPACITY; i++) items_[i] = 0;
}

OpResult Queue::enqueue(int value) {
    const std::string arg = toText(value);
    if (isFull()) {
        std::string hint = "Queue is full: rear has reached the last slot";
        if (front_ > 0) {
            hint += " (slots 0 to " + toText(front_ - 1) +
                    " are free but wasted, a circular queue would reuse them)";
        }
        // The step message explains more, the log keeps the short error.
        record("error", rear_, value, "rear", hint);
        return finish("Enqueue", arg, false, "Error: Queue is full: rear has reached the last slot");
    }

    int slot = rear_ + 1;
    items_[slot] = value;
    record("insert", slot, value, "", "Place " + arg + " in slot " + toText(slot) + " at the rear");
    rear_ = slot;
    record("movePointer", rear_, value, "rear", "rear moves right to slot " + toText(rear_));
    if (size() == 1) {
        record("movePointer", front_, value, "front",
               "This is the only item, so front is also slot " + toText(front_));
    }
    return finish("Enqueue", arg, true, "Success");
}

OpResult Queue::dequeue() {
    if (isEmpty()) {
        return fail("Dequeue", "", -1, 0, "Queue is empty: there is nothing to dequeue");
    }

    int value = items_[front_];
    std::string v = toText(value);
    int oldFront = front_;
    record("highlight", oldFront, value, "front",
           "The front item leaves first: " + v + " in slot " + toText(oldFront));
    record("remove", oldFront, value, "", "Remove " + v + " from the queue");
    front_++;
    if (isEmpty()) {
        record("movePointer", front_ < CAPACITY ? front_ : -1, 0, "front",
               "The queue is now empty. Slot " + toText(oldFront) + " stays wasted");
    } else {
        record("movePointer", front_, items_[front_], "front",
               "front moves right to slot " + toText(front_) + ". Slot " + toText(oldFront) +
               " is now wasted, it cannot be used again");
    }
    return finish("Dequeue", "", true, "Success (removed " + v + ")");
}

OpResult Queue::reset() {
    front_ = 0;
    rear_ = -1;
    return finishReset("The queue was reset. All 8 slots are free again and the log starts again.");
}

bool Queue::isEmpty() const { return rear_ < front_; }
bool Queue::isFull() const { return rear_ == CAPACITY - 1; }
int Queue::size() const { return rear_ - front_ + 1; }
int Queue::capacity() const { return CAPACITY; }
int Queue::getFront() const { return isEmpty() ? -1 : front_; }
int Queue::getRear() const { return isEmpty() ? -1 : rear_; }

std::vector<QueueSlot> Queue::getSlots() const {
    std::vector<QueueSlot> out;
    for (int i = 0; i < CAPACITY; i++) {
        QueueSlot s;
        s.index = i;
        s.occupied = (i >= front_ && i <= rear_);
        s.value = s.occupied ? items_[i] : 0;
        s.wasted = (i < front_);
        s.used = (i <= rear_);
        out.push_back(s);
    }
    return out;
}

std::vector<int> Queue::getItems() const {
    std::vector<int> out;
    for (int i = front_; i <= rear_; i++) out.push_back(items_[i]);
    return out;
}
