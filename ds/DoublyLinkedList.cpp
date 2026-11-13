// DoublyLinkedList.cpp
#include "DoublyLinkedList.h"

namespace {
const char* EMPTY_DELETE = "The list is empty: there is nothing to delete";

std::string positionError(int maxPos) {
    return "Invalid position: choose between 1 and " + toText(maxPos);
}
}  // namespace

DoublyLinkedList::DoublyLinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

DoublyLinkedList::~DoublyLinkedList() {
    freeAll();
}

void DoublyLinkedList::freeAll() {
    Node* cur = head_;
    while (cur != nullptr) {
        Node* next = cur->next;
        delete cur;
        cur = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
}

DoublyLinkedList::Node* DoublyLinkedList::walkTo(int targetIndex, const std::string& why) {
    Node* cur = head_;
    record("movePointer", 0, cur->value, "cur",
           "cur starts at the head (position 1, value " + toText(cur->value) + ")");
    for (int i = 1; i <= targetIndex; i++) {
        cur = cur->next;
        record("movePointer", i, cur->value, "cur",
               "Follow next: cur moves to position " + toText(i + 1) + " (value " +
                   toText(cur->value) + ")");
    }
    record("highlight", targetIndex, cur->value, "cur", why);
    return cur;
}

// ---------- insert ----------

void DoublyLinkedList::doInsertAtStart(int value) {
    std::string v = toText(value);
    Node* node = new Node{value, nullptr, head_};
    record("insert", 0, value, "", "Create a new node holding " + v + " (its prev is NULL)");
    if (head_ != nullptr) {
        record("link", 0, head_->value, "next",
               "The new node's next points to the old head (" + toText(head_->value) + ")");
        head_->prev = node;
        record("link", 1, value, "prev", "The old head's prev points back to the new node");
    } else {
        record("link", 0, 0, "next", "The list was empty, so the new node's next is NULL");
        tail_ = node;
    }
    head_ = node;
    size_++;
    record("movePointer", 0, value, "head", "head now points to the new node (" + v + ")");
    if (size_ == 1) {
        record("movePointer", 0, value, "tail", "It is the only node, so tail points to it too");
    }
}

void DoublyLinkedList::doInsertAtEnd(int value) {
    if (tail_ == nullptr) {
        doInsertAtStart(value);
        return;
    }
    std::string v = toText(value);
    record("highlight", size_ - 1, tail_->value, "tail",
           "tail already points at the last node, so no walking is needed");
    Node* node = new Node{value, tail_, nullptr};
    record("insert", size_, value, "", "Create a new node holding " + v + " (its next is NULL)");
    record("link", size_, tail_->value, "prev",
           "The new node's prev points back to the old tail (" + toText(tail_->value) + ")");
    tail_->next = node;
    record("link", size_ - 1, value, "next", "The old tail's next points to the new node");
    tail_ = node;
    size_++;
    record("movePointer", size_ - 1, value, "tail", "tail moves to the new node (" + v + ")");
}

OpResult DoublyLinkedList::insertAtStart(int value) {
    doInsertAtStart(value);
    return finish("Insert at Start", toText(value), true, "Success");
}

OpResult DoublyLinkedList::insertAtEnd(int value) {
    doInsertAtEnd(value);
    return finish("Insert at End", toText(value), true, "Success");
}

OpResult DoublyLinkedList::insertAtPosition(int position, int value) {
    const std::string arg = "pos " + toText(position) + ", value " + toText(value);
    if (position < 1 || position > size_ + 1) {
        return fail("Insert at Middle", arg, -1, value, positionError(size_ + 1));
    }

    if (position == 1) {
        doInsertAtStart(value);
    } else if (position == size_ + 1) {
        doInsertAtEnd(value);
    } else {
        int beforeIndex = position - 2;
        Node* cur = walkTo(beforeIndex, "cur stops at position " + toText(position - 1) +
                                            ", the node just before the new one");
        Node* after = cur->next;  // never NULL here, because position <= size
        std::string v = toText(value);
        Node* node = new Node{value, cur, after};
        int newIndex = position - 1;
        record("insert", newIndex, value, "", "Create a new node holding " + v);
        record("link", newIndex, after->value, "next",
               "1) The new node's next points to " + toText(after->value));
        record("link", newIndex, cur->value, "prev",
               "2) The new node's prev points back to " + toText(cur->value));
        after->prev = node;
        record("link", newIndex + 1, value, "prev",
               "3) " + toText(after->value) + "'s prev now points to the new node");
        cur->next = node;
        record("link", beforeIndex, value, "next",
               "4) " + toText(cur->value) + "'s next now points to the new node");
        size_++;
    }
    return finish("Insert at Middle", arg, true, "Success");
}

// ---------- delete ----------

int DoublyLinkedList::doDeleteAtStart() {
    Node* old = head_;
    int value = old->value;
    record("highlight", 0, value, "head", "The head node (" + toText(value) + ") will be removed");
    head_ = old->next;
    if (head_ != nullptr) {
        record("movePointer", 1, head_->value, "head",
               "head moves to the next node (" + toText(head_->value) + ")");
        head_->prev = nullptr;
        record("unlink", 1, value, "prev", "The new head's prev is set to NULL");
    } else {
        tail_ = nullptr;
        record("movePointer", -1, 0, "head", "That was the only node, so head becomes NULL");
        record("movePointer", -1, 0, "tail", "tail becomes NULL too");
    }
    record("remove", 0, value, "", "Delete the old node and free its memory");
    delete old;
    size_--;
    return value;
}

int DoublyLinkedList::doDeleteAtEnd() {
    if (size_ == 1) {
        return doDeleteAtStart();
    }
    Node* old = tail_;
    int value = old->value;
    record("highlight", size_ - 1, value, "tail",
           "tail points straight at the last node (" + toText(value) + "), no walking needed");
    tail_ = old->prev;
    record("movePointer", size_ - 2, tail_->value, "tail",
           "tail moves back to " + toText(tail_->value) + " using the prev pointer");
    tail_->next = nullptr;
    record("unlink", size_ - 2, value, "next", "The new tail's next is set to NULL");
    record("remove", size_ - 1, value, "", "Delete the old last node and free its memory");
    delete old;
    size_--;
    return value;
}

OpResult DoublyLinkedList::deleteAtStart() {
    if (head_ == nullptr) return fail("Delete at Start", "", -1, 0, EMPTY_DELETE);
    int removed = doDeleteAtStart();
    return finish("Delete at Start", "", true, "Success (removed " + toText(removed) + ")");
}

OpResult DoublyLinkedList::deleteAtEnd() {
    if (head_ == nullptr) return fail("Delete at End", "", -1, 0, EMPTY_DELETE);
    int removed = doDeleteAtEnd();
    return finish("Delete at End", "", true, "Success (removed " + toText(removed) + ")");
}

OpResult DoublyLinkedList::deleteAtPosition(int position) {
    const std::string arg = "pos " + toText(position);
    if (head_ == nullptr) return fail("Delete at Position", arg, -1, 0, EMPTY_DELETE);
    if (position < 1 || position > size_) {
        return fail("Delete at Position", arg, -1, 0, positionError(size_));
    }

    int removed;
    if (position == 1) {
        removed = doDeleteAtStart();
    } else if (position == size_) {
        removed = doDeleteAtEnd();
    } else {
        // In a doubly list we can stop ON the node to delete, because it knows its prev.
        int index = position - 1;
        Node* target = walkTo(index, "cur is on the node to delete (position " +
                                         toText(position) + ")");
        removed = target->value;
        Node* before = target->prev;
        Node* after = target->next;
        before->next = after;
        record("link", index - 1, after->value, "next",
               "1) " + toText(before->value) + "'s next skips over it to " + toText(after->value));
        after->prev = before;
        record("link", index + 1, before->value, "prev",
               "2) " + toText(after->value) + "'s prev skips back over it to " + toText(before->value));
        record("remove", index, removed, "", "Delete the node and free its memory");
        delete target;
        size_--;
    }
    return finish("Delete at Position", arg, true, "Success (removed " + toText(removed) + ")");
}

// ---------- search ----------

OpResult DoublyLinkedList::search(int value) {
    const std::string v = toText(value);
    int index = 0;
    for (Node* cur = head_; cur != nullptr; cur = cur->next, index++) {
        if (cur->value == value) {
            record("movePointer", index, cur->value, "cur",
                   "cur is at position " + toText(index + 1) + ": " + toText(cur->value) + " = " + v);
            record("found", index, value, "cur", "Found " + v + " at position " + toText(index + 1));
            return finish("Search", v, true, "Found at position " + toText(index + 1));
        }
        record("movePointer", index, cur->value, "cur",
               "cur is at position " + toText(index + 1) + ": " + toText(cur->value) + " is not " + v);
    }
    record("notFound", -1, value, "cur",
           head_ == nullptr ? "The list is empty, so " + v + " is not in it"
                            : "cur reached NULL: " + v + " is not in the list");
    return finish("Search", v, false, "Not found: " + v + " is not in the list");
}

OpResult DoublyLinkedList::reset() {
    freeAll();
    return finishReset("The list was reset. Every node was freed and the log starts again.");
}

std::vector<int> DoublyLinkedList::getValues() const {
    std::vector<int> out;
    for (Node* cur = head_; cur != nullptr; cur = cur->next) out.push_back(cur->value);
    return out;
}

int DoublyLinkedList::size() const { return size_; }
bool DoublyLinkedList::isEmpty() const { return size_ == 0; }
