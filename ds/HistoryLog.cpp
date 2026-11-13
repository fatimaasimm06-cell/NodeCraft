// HistoryLog.cpp
#include "HistoryLog.h"

HistoryLog::HistoryLog() : head_(nullptr), tail_(nullptr), size_(0), nextNumber_(1) {}

HistoryLog::~HistoryLog() {
    clear();
}

LogEntry HistoryLog::append(const std::string& function, const std::string& argument,
                            bool success, const std::string& result) {
    Node* node = new Node;
    node->entry.number = nextNumber_++;
    node->entry.function = function;
    node->entry.argument = argument;
    node->entry.success = success;
    node->entry.result = result;
    node->next = nullptr;

    if (head_ == nullptr) {
        // First entry: it is both the head and the tail.
        head_ = node;
        tail_ = node;
    } else {
        // Link after the current tail, then move the tail forward.
        tail_->next = node;
        tail_ = node;
    }
    size_++;
    return node->entry;
}

void HistoryLog::clear() {
    Node* cur = head_;
    while (cur != nullptr) {
        Node* next = cur->next;  // remember the next node before freeing this one
        delete cur;
        cur = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
    nextNumber_ = 1;
}

int HistoryLog::size() const {
    return size_;
}

std::vector<LogEntry> HistoryLog::toVector() const {
    std::vector<LogEntry> out;
    out.reserve(size_);
    for (Node* cur = head_; cur != nullptr; cur = cur->next) {
        out.push_back(cur->entry);
    }
    return out;
}
