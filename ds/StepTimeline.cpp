// StepTimeline.cpp
#include "StepTimeline.h"

StepTimeline::StepTimeline()
    : head_(nullptr), tail_(nullptr), cursor_(nullptr), size_(0), position_(0) {}

StepTimeline::~StepTimeline() {
    clear();
}

void StepTimeline::clear() {
    Node* cur = head_;
    while (cur != nullptr) {
        Node* next = cur->next;
        delete cur;
        cur = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
    cursor_ = nullptr;
    size_ = 0;
    position_ = 0;
}

void StepTimeline::load(const std::vector<Step>& steps) {
    clear();
    for (const Step& s : steps) {
        Node* node = new Node;
        node->step = s;
        node->prev = tail_;   // new node points back to the old tail
        node->next = nullptr;
        if (tail_ == nullptr) {
            head_ = node;
        } else {
            tail_->next = node;  // old tail points forward to the new node
        }
        tail_ = node;
        size_++;
    }
}

bool StepTimeline::hasNext() const {
    if (cursor_ == nullptr) {
        return head_ != nullptr;  // before the first step: is there a first step?
    }
    return cursor_->next != nullptr;
}

bool StepTimeline::hasPrev() const {
    return cursor_ != nullptr && cursor_->prev != nullptr;
}

Step StepTimeline::next() {
    if (!hasNext()) {
        return current();
    }
    cursor_ = (cursor_ == nullptr) ? head_ : cursor_->next;
    position_++;
    return cursor_->step;
}

Step StepTimeline::prev() {
    if (!hasPrev()) {
        return current();
    }
    cursor_ = cursor_->prev;
    position_--;
    return cursor_->step;
}

Step StepTimeline::current() const {
    if (cursor_ == nullptr) {
        return Step();  // empty step: action ""
    }
    return cursor_->step;
}

int StepTimeline::size() const {
    return size_;
}

int StepTimeline::position() const {
    return position_;
}
