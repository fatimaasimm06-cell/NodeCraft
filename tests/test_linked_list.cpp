// test_linked_list.cpp - SinglyLinkedList
#include "check.h"
#include "../ds/LinkedList.h"

static int countAction(const OpResult& r, const std::string& action) {
    int n = 0;
    for (const Step& s : r.steps) if (s.action == action) n++;
    return n;
}

void testSinglyList() {
    SinglyLinkedList l;

    // Insert at start: insert, link (new -> old head), THEN head moves.
    OpResult a = l.insertAtStart(20);
    CHECK(a.success);
    l.insertAtStart(10);
    CHECK_EQ(join(l.getValues()), std::string("10 20"));
    OpResult s = l.insertAtStart(5);
    CHECK_EQ(s.steps[0].action, std::string("insert"));
    CHECK_EQ(s.steps[1].action, std::string("link"));
    CHECK_EQ(s.steps[1].value, 10);
    CHECK_EQ(s.steps[2].action, std::string("movePointer"));
    CHECK_EQ(s.steps[2].pointer, std::string("head"));

    // Insert at end: cur walks node by node, then links.
    OpResult e = l.insertAtEnd(40);
    CHECK_EQ(join(l.getValues()), std::string("5 10 20 40"));
    CHECK_EQ(countAction(e, "movePointer"), 3);  // positions 1, 2, 3
    CHECK_EQ(e.steps.back().action, std::string("link"));
    CHECK_EQ(e.entry.function, std::string("Insert at End"));

    // Insert at middle: new node is linked first, then cur.next.
    OpResult m = l.insertAtPosition(4, 30);
    CHECK(m.success);
    CHECK_EQ(join(l.getValues()), std::string("5 10 20 30 40"));
    CHECK_EQ(m.entry.function, std::string("Insert at Middle"));
    CHECK_EQ(m.entry.argument, std::string("pos 4, value 30"));
    int n = (int)m.steps.size();
    CHECK_EQ(m.steps[n - 2].action, std::string("link"));
    CHECK_EQ(m.steps[n - 2].index, 3);  // new node -> 40
    CHECK_EQ(m.steps[n - 2].value, 40);
    CHECK_EQ(m.steps[n - 1].index, 2);  // then 20 -> new node
    CHECK_EQ(m.steps[n - 1].value, 30);

    CHECK(l.insertAtPosition(1, 1).success);
    CHECK(l.insertAtPosition(7, 50).success);  // size + 1 = end
    CHECK_EQ(join(l.getValues()), std::string("1 5 10 20 30 40 50"));

    // Invalid positions.
    OpResult bad = l.insertAtPosition(0, 9);
    CHECK(!bad.success);
    CHECK_EQ(bad.message, std::string("Error: Invalid position: choose between 1 and 8"));
    CHECK(!l.insertAtPosition(9, 9).success);
    OpResult badDel = l.deleteAtPosition(8);
    CHECK_EQ(badDel.message, std::string("Error: Invalid position: choose between 1 and 7"));
    CHECK(!l.deleteAtPosition(-3).success);
    CHECK_EQ(l.size(), 7);

    // Search.
    OpResult f = l.search(20);
    CHECK(f.success);
    CHECK_EQ(f.message, std::string("Found at position 4"));
    CHECK_EQ(f.steps.back().action, std::string("found"));
    CHECK_EQ(countAction(f, "movePointer"), 4);
    OpResult nf = l.search(99);
    CHECK(!nf.success);
    CHECK_EQ(nf.steps.back().action, std::string("notFound"));
    CHECK_EQ(countAction(nf, "movePointer"), 7);

    // Deletes.
    OpResult ds = l.deleteAtStart();
    CHECK_EQ(ds.message, std::string("Success (removed 1)"));
    OpResult de = l.deleteAtEnd();
    CHECK_EQ(de.message, std::string("Success (removed 50)"));
    CHECK_EQ(de.steps.back().action, std::string("remove"));
    OpResult dp = l.deleteAtPosition(3);
    CHECK_EQ(dp.message, std::string("Success (removed 20)"));
    CHECK_EQ(join(l.getValues()), std::string("5 10 30 40"));
    CHECK(l.deleteAtPosition(1).success);
    CHECK(l.deleteAtPosition(3).success);  // last
    CHECK_EQ(join(l.getValues()), std::string("10 30"));

    // Delete until empty.
    CHECK(l.deleteAtEnd().success);
    CHECK(l.deleteAtEnd().success);
    CHECK(l.isEmpty());
    OpResult empty = l.deleteAtStart();
    CHECK(!empty.success);
    CHECK_EQ(empty.message, std::string("Error: The list is empty: there is nothing to delete"));
    CHECK(!l.deleteAtEnd().success);
    CHECK(!l.deleteAtPosition(1).success);
    CHECK(!l.search(1).success);

    // Insert at end of an empty list, then reset frees everything.
    CHECK(l.insertAtEnd(7).success);
    CHECK(l.insertAtPosition(1, 3).success);
    l.reset();
    CHECK(l.isEmpty());
    CHECK_EQ(l.logSize(), 0);
    l.insertAtStart(1);
    l.insertAtStart(2);  // destructor must free these
}
