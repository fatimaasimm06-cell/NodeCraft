// test_doubly_linked_list.cpp
#include "check.h"
#include "../ds/DoublyLinkedList.h"

static int countPointer(const OpResult& r, const std::string& action, const std::string& ptr) {
    int n = 0;
    for (const Step& s : r.steps) if (s.action == action && s.pointer == ptr) n++;
    return n;
}

void testDoublyList() {
    DoublyLinkedList l;

    OpResult first = l.insertAtStart(20);
    CHECK(first.success);
    OpResult s = l.insertAtStart(10);
    // Both links are shown: new.next -> old head, old head.prev -> new.
    CHECK_EQ(countPointer(s, "link", "next"), 1);
    CHECK_EQ(countPointer(s, "link", "prev"), 1);

    OpResult e = l.insertAtEnd(40);
    CHECK_EQ(join(l.getValues()), std::string("10 20 40"));
    CHECK_EQ(countPointer(e, "movePointer", "cur"), 0);  // O(1) via tail, no walking
    CHECK_EQ(countPointer(e, "link", "prev"), 1);
    CHECK_EQ(countPointer(e, "link", "next"), 1);
    CHECK_EQ(countPointer(e, "movePointer", "tail"), 1);

    OpResult m = l.insertAtPosition(3, 30);
    CHECK_EQ(join(l.getValues()), std::string("10 20 30 40"));
    CHECK_EQ(countPointer(m, "link", "next"), 2);
    CHECK_EQ(countPointer(m, "link", "prev"), 2);

    CHECK(l.insertAtPosition(1, 5).success);
    CHECK(l.insertAtPosition(6, 50).success);
    CHECK_EQ(join(l.getValues()), std::string("5 10 20 30 40 50"));

    CHECK(!l.insertAtPosition(0, 1).success);
    CHECK(!l.insertAtPosition(8, 1).success);
    CHECK_EQ(l.deleteAtPosition(7).message, std::string("Error: Invalid position: choose between 1 and 6"));

    CHECK_EQ(l.search(30).message, std::string("Found at position 4"));
    CHECK(!l.search(31).success);

    OpResult de = l.deleteAtEnd();
    CHECK_EQ(de.message, std::string("Success (removed 50)"));
    CHECK_EQ(countPointer(de, "movePointer", "cur"), 0);  // O(1) via tail
    CHECK_EQ(countPointer(de, "unlink", "next"), 1);

    OpResult dm = l.deleteAtPosition(3);  // removes 20, updates both neighbours
    CHECK_EQ(dm.message, std::string("Success (removed 20)"));
    CHECK_EQ(countPointer(dm, "link", "next"), 1);
    CHECK_EQ(countPointer(dm, "link", "prev"), 1);
    CHECK_EQ(join(l.getValues()), std::string("5 10 30 40"));

    OpResult ds = l.deleteAtStart();
    CHECK_EQ(ds.message, std::string("Success (removed 5)"));
    CHECK_EQ(countPointer(ds, "unlink", "prev"), 1);

    CHECK(l.deleteAtPosition(3).success);  // last -> via tail
    CHECK(l.deleteAtPosition(1).success);
    CHECK_EQ(join(l.getValues()), std::string("30"));

    // After the list empties, tail must be NULL too: insert at end must still work.
    CHECK(l.deleteAtEnd().success);
    CHECK(l.isEmpty());
    CHECK(!l.deleteAtStart().success);
    CHECK(!l.deleteAtEnd().success);
    CHECK(l.insertAtEnd(1).success);
    CHECK(l.insertAtEnd(2).success);
    CHECK(l.insertAtStart(0).success);
    CHECK_EQ(join(l.getValues()), std::string("0 1 2"));
    CHECK(l.deleteAtStart().success);
    CHECK(l.deleteAtStart().success);
    CHECK(l.deleteAtStart().success);
    CHECK(l.insertAtEnd(9).success);  // tail was reset correctly
    CHECK_EQ(join(l.getValues()), std::string("9"));

    l.reset();
    CHECK(l.isEmpty());
    l.insertAtEnd(1);
    l.insertAtEnd(2);  // destructor must free these
}
