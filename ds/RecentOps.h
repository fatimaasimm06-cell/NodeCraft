// RecentOps.h
// The "Recent" strip (HL-9): a circular queue of the last 5 log entries.
// When it is full, adding a new entry overwrites the oldest one.
#pragma once

#include "Common.h"

class RecentOps {
public:
    static const int CAPACITY = 5;

    RecentOps();

    // Adds an entry. If all 5 slots are used, the oldest entry is overwritten.
    void add(const LogEntry& entry);

    void clear();
    int size() const;

    // Entries from oldest to newest.
    std::vector<LogEntry> toVector() const;

private:
    LogEntry items_[CAPACITY];
    int front_;  // index of the oldest entry
    int rear_;   // index of the newest entry
    int count_;
};
