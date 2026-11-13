// Stack.cpp
#include "Stack.h"

Stack::Stack() : top_(-1) {
    for (int i = 0; i < CAPACITY; i++) items_[i] = 0;
}

OpResult Stack::push(int value) {
    const std::string arg = toText(value);
    if (isFull()) {
        return fail("Push", arg, top_, value, "Stack overflow: the stack is full (8 items)");
    }

    int slot = top_ + 1;
    items_[slot] = value;
    record("insert", slot, value, "",
           "Place " + arg + " in slot " + toText(slot) + ", just above the old top");
    top_ = slot;
    record("movePointer", top_, value, "top",
           "top moves up to slot " + toText(top_) + " (value " + arg + ")");

    return finish("Push", arg, true, "Success");
}

OpResult Stack::pop() {
    if (isEmpty()) {
        return fail("Pop", "", -1, 0, "Stack underflow: the stack is empty");
    }

    int value = items_[top_];
    std::string v = toText(value);
    record("highlight", top_, value, "top",
           "Only the top item can be removed: " + v + " in slot " + toText(top_));
    record("remove", top_, value, "", "Remove " + v + " from the stack");
    top_--;
    if (top_ >= 0) {
        record("movePointer", top_, items_[top_], "top",
               "top moves down to slot " + toText(top_) + " (value " + toText(items_[top_]) + ")");
    } else {
        record("movePointer", -1, 0, "top", "The stack is now empty, so top is -1");
    }

    return finish("Pop", "", true, "Success (removed " + v + ")");
}

OpResult Stack::reset() {
    top_ = -1;
    return finishReset("The stack was reset. It is empty and the log starts again.");
}

int Stack::peek() const { return isEmpty() ? 0 : items_[top_]; }
int Stack::size() const { return top_ + 1; }
bool Stack::isEmpty() const { return top_ == -1; }
bool Stack::isFull() const { return top_ == CAPACITY - 1; }
int Stack::getTop() const { return top_; }
int Stack::capacity() const { return CAPACITY; }

std::vector<int> Stack::getItems() const {
    std::vector<int> out;
    for (int i = 0; i <= top_; i++) out.push_back(items_[i]);
    return out;
}
