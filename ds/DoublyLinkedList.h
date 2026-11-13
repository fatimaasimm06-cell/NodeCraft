// DoublyLinkedList.h
// Hand-written DOUBLY linked list with head AND tail pointers.
// Every node has prev and next pointers, so the animation shows both links
// changing (DL-3). Delete at end is O(1) thanks to the tail pointer.
#pragma once

#include "TrackedStructure.h"

class DoublyLinkedList : public TrackedStructure {
public:
    DoublyLinkedList();
    ~DoublyLinkedList() override;

    OpResult insertAtStart(int value);
    OpResult insertAtEnd(int value);
    OpResult insertAtPosition(int position, int value);  // valid 1..size+1
    OpResult deleteAtStart();
    OpResult deleteAtEnd();
    OpResult deleteAtPosition(int position);             // valid 1..size
    OpResult search(int value);
    OpResult reset();

    std::vector<int> getValues() const;  // head -> tail
    int size() const;
    bool isEmpty() const;

private:
    struct Node {
        int value;
        Node* prev;
        Node* next;
    };

    Node* head_;
    Node* tail_;
    int size_;

    void freeAll();
    Node* walkTo(int targetIndex, const std::string& why);

    void doInsertAtStart(int value);
    void doInsertAtEnd(int value);
    int doDeleteAtStart();
    int doDeleteAtEnd();
};
