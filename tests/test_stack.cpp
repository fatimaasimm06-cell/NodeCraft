// test_stack.cpp
#include "check.h"
#include "../ds/Stack.h"

void testStack() {
    // Push to 8, the 9th push fails.
    {
        Stack s;
        CHECK(s.isEmpty());
        CHECK_EQ(s.getTop(), -1);
        for (int i = 1; i <= 8; i++) CHECK(s.push(i * 10).success);
        CHECK(s.isFull());
        CHECK_EQ(s.size(), 8);
        CHECK_EQ(s.getTop(), 7);
        CHECK_EQ(s.peek(), 80);
        OpResult r = s.push(90);
        CHECK(!r.success);
        CHECK_EQ(r.message, std::string("Error: Stack overflow: the stack is full (8 items)"));
        CHECK_EQ(r.steps[0].action, std::string("error"));
        CHECK_EQ(s.size(), 8);
        CHECK_EQ(join(s.getItems()), std::string("10 20 30 40 50 60 70 80"));
    }

    // Pop on an empty stack fails.
    {
        Stack s;
        OpResult r = s.pop();
        CHECK(!r.success);
        CHECK_EQ(r.message, std::string("Error: Stack underflow: the stack is empty"));
        CHECK_EQ(s.peek(), 0);
    }

    // PRD example: Push 10, Push 25, Pop, Pop, Pop.
    {
        Stack s;
        s.push(10);
        s.push(25);
        OpResult p1 = s.pop();
        CHECK_EQ(p1.message, std::string("Success (removed 25)"));
        s.pop();
        s.pop();
        std::vector<LogEntry> log = s.getLog();
        CHECK_EQ((int)log.size(), 5);
        CHECK_EQ(log[0].function, std::string("Push"));
        CHECK_EQ(log[0].argument, std::string("10"));
        CHECK_EQ(log[1].argument, std::string("25"));
        CHECK_EQ(log[2].result, std::string("Success (removed 25)"));
        CHECK_EQ(log[3].result, std::string("Success (removed 10)"));
        CHECK_EQ(log[4].function, std::string("Pop"));
        CHECK(!log[4].success);
        CHECK(log[4].result.find("the stack is empty") != std::string::npos);
        CHECK_EQ(log[4].result.substr(0, 7), std::string("Error: "));
        CHECK_EQ(log[4].number, 5);
    }

    // Step shapes.
    {
        Stack s;
        OpResult push = s.push(5);
        CHECK_EQ((int)push.steps.size(), 2);
        CHECK_EQ(push.steps[0].action, std::string("insert"));
        CHECK_EQ(push.steps[0].index, 0);
        CHECK_EQ(push.steps[1].action, std::string("movePointer"));
        CHECK_EQ(push.steps[1].pointer, std::string("top"));

        OpResult pop = s.pop();
        CHECK_EQ((int)pop.steps.size(), 3);
        CHECK_EQ(pop.steps[0].action, std::string("highlight"));
        CHECK_EQ(pop.steps[1].action, std::string("remove"));
        CHECK_EQ(pop.steps[2].action, std::string("movePointer"));
        CHECK_EQ(pop.steps[2].index, -1);
    }
}
