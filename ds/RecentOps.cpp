// RecentOps.cpp
#include "RecentOps.h"

RecentOps::RecentOps() : front_(0), rear_(CAPACITY - 1), count_(0) {}

void RecentOps::add(const LogEntry& entry) {
    // Rear always moves forward one slot, wrapping from 4 back to 0.
    rear_ = (rear_ + 1) % CAPACITY;
    items_[rear_] = entry;

    if (count_ < CAPACITY) {
        count_++;
    } else {
        // The buffer was full, so we just overwrote the oldest entry.
        // The oldest is now the next slot after it.
        front_ = (front_ + 1) % CAPACITY;
    }
}

void RecentOps::clear() {
    front_ = 0;
    rear_ = CAPACITY - 1;
    count_ = 0;
}

int RecentOps::size() const {
    return count_;
}

std::vector<LogEntry> RecentOps::toVector() const {
    std::vector<LogEntry> out;
    out.reserve(count_);
    for (int k = 0; k < count_; k++) {
        out.push_back(items_[(front_ + k) % CAPACITY]);
    }
    return out;
}
