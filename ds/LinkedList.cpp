// LinkedList.cpp
#include "LinkedList.h"

namespace {
const char* EMPTY_DELETE = "The list is empty: there is nothing to delete";

std::string positionError(int maxPos) {
    return "Invalid position: choose between 1 and " + toText(maxPos);
}
}  // namespace

SinglyLinkedList::SinglyLinkedList() : head_(nullptr), size_(0) {}

SinglyLinkedList::~SinglyLinkedList() {
    freeAll();
}

void SinglyLinkedList::freeAll() {
    Node* cur = head_;
    while (cur != nullptr) {
        Node* next = cur->next;
        delete cur;
        cur = next;
    }
    head_ = nullptr;
    size_ = 0;
}

SinglyLinkedList::Node* SinglyLinkedList::walkTo(int targetIndex, const std::string& why) {
    Node* cur = head_;
    record("movePointer", 0, cur->value, "cur", "cur starts at the head (position 1, value " +
                                                    toText(cur->value) + ")");
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

void SinglyLinkedList::doInsertAtStart(int value) {
    std::string v = toText(value);
    Node* node = new Node{value, head_};
    record("insert", 0, value, "", "Create a new node holding " + v);
    if (head_ != nullptr) {
        record("link", 0, head_->value, "next",
               "The new node's next points to the old head (" + toText(head_->value) + ")");
    } else {
        record("link", 0, 0, "next", "The list was empty, so the new node's next is NULL");
    }
    head_ = node;
    size_++;
    record("movePointer", 0, value, "head", "head now points to the new node (" + v + ")");
}

void SinglyLinkedList::doInsertAtEnd(int value) {
    if (head_ == nullptr) {
        doInsertAtStart(value);
        return;
    }
    std::string v = toText(value);
    Node* last = walkTo(size_ - 1, "cur.next is NULL, so cur is the last node");
    Node* node = new Node{value, nullptr};
    record("insert", size_, value, "", "Create a new node holding " + v + " (its next is NULL)");
    last->next = node;
    record("link", size_ - 1, value, "next",
           "The last node's next now points to the new node (" + v + ")");
    size_++;
}

OpResult SinglyLinkedList::insertAtStart(int value) {
    doInsertAtStart(value);
    return finish("Insert at Start", toText(value), true, "Success");
}

OpResult SinglyLinkedList::insertAtEnd(int value) {
    doInsertAtEnd(value);
    return finish("Insert at End", toText(value), true, "Success");
}

OpResult SinglyLinkedList::insertAtPosition(int position, int value) {
    const std::string arg = "pos " + toText(position) + ", value " + toText(value);
    if (position < 1 || position > size_ + 1) {
        return fail("Insert at Middle", arg, -1, value, positionError(size_ + 1));
    }

    if (position == 1) {
        doInsertAtStart(value);
    } else {
        // Walk to the node just BEFORE the new position.
        int beforeIndex = position - 2;
        Node* cur = walkTo(beforeIndex, "cur stops at position " + toText(position - 1) +
                                            ", the node just before the new one");
        std::string v = toText(value);
        Node* node = new Node{value, cur->next};
        record("insert", position - 1, value, "", "Create a new node holding " + v);
        // Order matters: first connect the new node, then change cur.next.
        if (node->next != nullptr) {
            record("link", position - 1, node->next->value, "next",
                   "First, the new node's next points to cur.next (" + toText(node->next->value) +
                       ")");
        } else {
            record("link", position - 1, 0, "next", "First, the new node's next is NULL");
        }
        cur->next = node;
        record("link", beforeIndex, value, "next",
               "Then cur.next points to the new node, so nothing is lost");
        size_++;
    }
    return finish("Insert at Middle", arg, true, "Success");
}

// ---------- delete ----------

int SinglyLinkedList::doDeleteAtStart() {
    Node* old = head_;
    int value = old->value;
    record("highlight", 0, value, "head", "The head node (" + toText(value) + ") will be removed");
    head_ = old->next;
    if (head_ != nullptr) {
        record("movePointer", 1, head_->value, "head",
               "head moves to the next node (" + toText(head_->value) + ")");
    } else {
        record("movePointer", -1, 0, "head", "That was the only node, so head becomes NULL");
    }
    record("unlink", 0, head_ ? head_->value : 0, "next", "The old head is disconnected");
    record("remove", 0, value, "", "Delete the old node and free its memory");
    delete old;
    size_--;
    return value;
}

int SinglyLinkedList::doDeleteAtEnd() {
    if (size_ == 1) {
        return doDeleteAtStart();
    }
    Node* cur = walkTo(size_ - 2, "cur.next.next is NULL, so cur is the second-to-last node");
    Node* last = cur->next;
    int value = last->value;
    record("highlight", size_ - 1, value, "", "The last node (" + toText(value) + ") will be removed");
    cur->next = nullptr;
    record("unlink", size_ - 2, value, "next", "cur.next is set to NULL, cur is the new last node");
    record("remove", size_ - 1, value, "", "Delete the old last node and free its memory");
    delete last;
    size_--;
    return value;
}

OpResult SinglyLinkedList::deleteAtStart() {
    if (head_ == nullptr) return fail("Delete at Start", "", -1, 0, EMPTY_DELETE);
    int removed = doDeleteAtStart();
    return finish("Delete at Start", "", true, "Success (removed " + toText(removed) + ")");
}

OpResult SinglyLinkedList::deleteAtEnd() {
    if (head_ == nullptr) return fail("Delete at End", "", -1, 0, EMPTY_DELETE);
    int removed = doDeleteAtEnd();
    return finish("Delete at End", "", true, "Success (removed " + toText(removed) + ")");
}

OpResult SinglyLinkedList::deleteAtPosition(int position) {
    const std::string arg = "pos " + toText(position);
    if (head_ == nullptr) return fail("Delete at Position", arg, -1, 0, EMPTY_DELETE);
    if (position < 1 || position > size_) {
        return fail("Delete at Position", arg, -1, 0, positionError(size_));
    }

    int removed;
    if (position == 1) {
        removed = doDeleteAtStart();
    } else {
        int beforeIndex = position - 2;
        Node* cur = walkTo(beforeIndex, "cur stops at position " + toText(position - 1) +
                                            ", the node just before the one to delete");
        Node* target = cur->next;
        removed = target->value;
        record("highlight", position - 1, removed, "",
               "The node at position " + toText(position) + " (" + toText(removed) + ") will be removed");
        cur->next = target->next;
        if (cur->next != nullptr) {
            record("link", beforeIndex, cur->next->value, "next",
                   "cur.next skips over it and points to " + toText(cur->next->value));
        } else {
            record("link", beforeIndex, 0, "next", "cur.next skips over it and becomes NULL");
        }
        record("remove", position - 1, removed, "", "Delete the node and free its memory");
        delete target;
        size_--;
    }
    return finish("Delete at Position", arg, true, "Success (removed " + toText(removed) + ")");
}

// ---------- search ----------

OpResult SinglyLinkedList::search(int value) {
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

OpResult SinglyLinkedList::reset() {
    freeAll();
    return finishReset("The list was reset. Every node was freed and the log starts again.");
}

std::vector<int> SinglyLinkedList::getValues() const {
    std::vector<int> out;
    for (Node* cur = head_; cur != nullptr; cur = cur->next) out.push_back(cur->value);
    return out;
}

int SinglyLinkedList::size() const { return size_; }
bool SinglyLinkedList::isEmpty() const { return size_ == 0; }
