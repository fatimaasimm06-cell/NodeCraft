// HistoryLog.h
// A hand-written SINGLY linked list of LogEntry (PRD section 7: "Singly Linked
// List - storage for each structure's history log; entries are appended at the end").
#pragma once

#include "Common.h"

class HistoryLog {
public:
    HistoryLog();
    ~HistoryLog();

    // Rule of three: a log owns its nodes, so copying is not allowed.
    HistoryLog(const HistoryLog&) = delete;
    HistoryLog& operator=(const HistoryLog&) = delete;

    // Adds a new entry at the END in O(1) using the tail pointer.
    // The entry number is filled in automatically (1, 2, 3...).
    LogEntry append(const std::string& function, const std::string& argument,
                    bool success, const std::string& result);

    // Deletes every entry. Numbering starts again at 1.
    void clear();

    int size() const;

    // Copies the entries, oldest first, into a vector (used to hand data to JS).
    std::vector<LogEntry> toVector() const;

private:
    struct Node {
        LogEntry entry;
        Node* next;
    };

    Node* head_;
    Node* tail_;
    int size_;
    int nextNumber_;
};
