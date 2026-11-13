// test_steps.cpp - StepRecorder (queue) and StepTimeline (doubly linked list)
#include "check.h"
#include "../ds/StepRecorder.h"
#include "../ds/StepTimeline.h"
#include "../ds/Stack.h"

void testSteps() {
    // Recorder drains in FIFO order.
    {
        StepRecorder rec;
        rec.record("highlight", 0, 1, "", "a");
        rec.record("remove", 1, 2, "", "b");
        rec.record("movePointer", 2, 3, "top", "c");
        CHECK_EQ(rec.size(), 3);
        std::vector<Step> v = rec.drain();
        CHECK_EQ(rec.size(), 0);
        CHECK_EQ((int)v.size(), 3);
        CHECK_EQ(v[0].message, std::string("a"));
        CHECK_EQ(v[2].pointer, std::string("top"));
    }

    // Timeline forward / back / boundaries.
    {
        StepTimeline t;
        CHECK(!t.hasNext());
        CHECK(!t.hasPrev());
        CHECK_EQ(t.next().action, std::string(""));  // empty timeline: stays put

        std::vector<Step> steps;
        steps.push_back(makeStep("highlight", 0, 0, "", "one"));
        steps.push_back(makeStep("insert", 1, 0, "", "two"));
        steps.push_back(makeStep("link", 2, 0, "", "three"));
        t.load(steps);
        CHECK_EQ(t.size(), 3);
        CHECK_EQ(t.position(), 0);
        CHECK_EQ(t.current().action, std::string(""));
        CHECK(t.hasNext());
        CHECK(!t.hasPrev());

        CHECK_EQ(t.next().message, std::string("one"));
        CHECK(!t.hasPrev());  // cannot go back before step 1
        CHECK_EQ(t.next().message, std::string("two"));
        CHECK_EQ(t.next().message, std::string("three"));
        CHECK_EQ(t.position(), 3);
        CHECK(!t.hasNext());
        CHECK_EQ(t.next().message, std::string("three"));  // stays at the end
        CHECK_EQ(t.position(), 3);

        CHECK_EQ(t.prev().message, std::string("two"));
        CHECK_EQ(t.prev().message, std::string("one"));
        CHECK_EQ(t.prev().message, std::string("one"));  // stays at step 1
        CHECK_EQ(t.position(), 1);

        // Loading replaces the old steps.
        steps.pop_back();
        t.load(steps);
        CHECK_EQ(t.size(), 2);
        CHECK_EQ(t.position(), 0);
    }

    // Each structure loads its last operation into its own timeline.
    {
        Stack s;
        OpResult r = s.push(10);
        CHECK_EQ(s.stepCount(), (int)r.steps.size());
        CHECK_EQ(s.stepPosition(), 0);
        CHECK(s.hasNextStep());
        CHECK_EQ(s.stepNext().action, std::string("insert"));
        CHECK_EQ(s.stepNext().action, std::string("movePointer"));
        CHECK(!s.hasNextStep());
        CHECK(s.hasPrevStep());
        CHECK_EQ(s.stepPrev().action, std::string("insert"));
        CHECK_EQ(s.currentStep().action, std::string("insert"));
    }
}
