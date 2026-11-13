// test_queue.cpp - linear Queue and CircularQueue
#include "check.h"
#include "../ds/Queue.h"
#include "../ds/CircularQueue.h"

void testQueue() {
    {
        Queue q;
        CHECK(q.isEmpty());
        CHECK_EQ(q.getFront(), -1);
        CHECK(!q.dequeue().success);
        for (int i = 1; i <= 8; i++) CHECK(q.enqueue(i).success);
        CHECK(q.isFull());
        CHECK_EQ(q.getFront(), 0);
        CHECK_EQ(q.getRear(), 7);

        // Dequeue three: slots 0..2 are free, but the queue is still "full".
        for (int i = 0; i < 3; i++) CHECK(q.dequeue().success);
        CHECK_EQ(q.size(), 5);
        CHECK(q.isFull());
        OpResult r = q.enqueue(99);
        CHECK(!r.success);
        CHECK_EQ(r.message, std::string("Error: Queue is full: rear has reached the last slot"));
        CHECK_EQ(join(q.getItems()), std::string("4 5 6 7 8"));

        std::vector<QueueSlot> slots = q.getSlots();
        CHECK_EQ((int)slots.size(), 8);
        CHECK(slots[0].wasted && !slots[0].occupied);
        CHECK(slots[2].wasted);
        CHECK(!slots[3].wasted && slots[3].occupied);
        CHECK_EQ(slots[3].value, 4);
        CHECK_EQ(q.getFront(), 3);

        // Empty it completely: front passes rear, still cannot enqueue.
        for (int i = 0; i < 5; i++) q.dequeue();
        CHECK(q.isEmpty());
        CHECK_EQ(q.getFront(), -1);
        CHECK(!q.enqueue(1).success);
        CHECK(!q.dequeue().success);

        q.reset();
        CHECK(q.enqueue(1).success);
        CHECK_EQ(q.getFront(), 0);
        CHECK_EQ(q.getRear(), 0);
    }
}

void testCircularQueue() {
    {
        CircularQueue q;
        CHECK(q.isEmpty());
        CHECK(!q.dequeue().success);
        for (int i = 1; i <= 8; i++) CHECK(q.enqueue(i).success);
        CHECK(q.isFull());
        OpResult full = q.enqueue(9);
        CHECK(!full.success);
        CHECK_EQ(full.message, std::string("Error: Circular queue is full: all 8 slots are in use"));

        // Free slots 0 and 1, then reuse them by wrapping around.
        q.dequeue();
        q.dequeue();
        CHECK(!q.isFull());
        CHECK_EQ(q.getFront(), 2);
        OpResult wrap = q.enqueue(100);
        CHECK(wrap.success);
        CHECK_EQ(q.getRear(), 0);
        CHECK_EQ(wrap.steps[0].action, std::string("movePointer"));
        CHECK_EQ(wrap.steps[0].message, std::string("Rear wraps around to slot 0 and reuses a freed slot"));
        OpResult reuse = q.enqueue(101);
        CHECK_EQ(q.getRear(), 1);
        CHECK(reuse.steps[0].message.find("reuses") != std::string::npos);
        CHECK(q.isFull());
        CHECK_EQ(join(q.getItems()), std::string("3 4 5 6 7 8 100 101"));

        std::vector<QueueSlot> slots = q.getSlots();
        CHECK(slots[0].occupied && slots[0].value == 100);
        CHECK(slots[1].occupied && slots[1].value == 101);

        // Empty after wrap: front wraps too.
        for (int i = 0; i < 8; i++) CHECK(q.dequeue().success);
        CHECK(q.isEmpty());
        CHECK(!q.isFull());
        CHECK_EQ(q.getFront(), -1);
        CHECK_EQ(q.getRear(), -1);
        CHECK(!q.dequeue().success);

        // Next item goes where front is now (slot 2).
        q.enqueue(5);
        CHECK_EQ(q.getFront(), 2);
        CHECK_EQ(q.getRear(), 2);
        CHECK_EQ(q.size(), 1);

        q.reset();
        CHECK(q.isEmpty());
        CHECK(!q.getSlots()[0].used);
    }
}
