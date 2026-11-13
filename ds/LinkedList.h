// LinkedList.h
// Hand-written SINGLY linked list. Each node has a value and a next pointer.
// Positions given by the user are 1-based (position 1 = head).
// Step indexes are 0-based node indexes for drawing.
#pragma once

#include "TrackedStructure.h"

class SinglyLinkedList : public TrackedStructure {
public:
    SinglyLinkedList();
    ~SinglyLinkedList() override;

    OpResult insertAtStart(int value);                 // SL-2
    OpResult insertAtEnd(int value);                   // SL-3
    OpResult insertAtPosition(int position, int value);// SL-4, valid 1..size+1
    OpResult deleteAtStart();                          // SL-5
    OpResult deleteAtEnd();
    OpResult deleteAtPosition(int position);           // valid 1..size
    OpResult search(int value);                        // SL-6
    OpResult reset();

    std::vector<int> getValues() const;  // head -> last
    int size() const;
    bool isEmpty() const;

private:
    struct Node {
        int value;
        Node* next;
    };

    Node* head_;
    int size_;

    void freeAll();
    // Records "cur" moving from the head to node `targetIndex` and returns that node.
    Node* walkTo(int targetIndex, const std::string& why);

    // Shared bodies so "Insert at Middle" with position 1 behaves like insert at start, etc.
    void doInsertAtStart(int value);
    void doInsertAtEnd(int value);
    int doDeleteAtStart();
    int doDeleteAtEnd();
};
