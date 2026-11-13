// StepTimeline.h
// A hand-written DOUBLY linked list of Step with a cursor (AN-5), so the user
// can move forward AND backward through the steps of the last operation.
//
// Cursor positions:
//   position() == 0          -> before the first step (nothing shown yet)
//   position() == k (1..n)   -> step k is the current step
// prev() never moves back past step 1.
#pragma once

#include "Common.h"

class StepTimeline {
public:
    StepTimeline();
    ~StepTimeline();

    StepTimeline(const StepTimeline&) = delete;
    StepTimeline& operator=(const StepTimeline&) = delete;

    // Replaces all old steps with these. The cursor goes before the first step.
    void load(const std::vector<Step>& steps);
    void clear();

    bool hasNext() const;
    bool hasPrev() const;

    // Moves the cursor and returns the new current step.
    // If there is nowhere to move, the cursor stays and current() is returned.
    Step next();
    Step prev();

    // The step under the cursor, or an empty step (action "") before the first one.
    Step current() const;

    int size() const;
    int position() const;

private:
    struct Node {
        Step step;
        Node* prev;
        Node* next;
    };

    Node* head_;
    Node* tail_;
    Node* cursor_;  // nullptr means "before the first step"
    int size_;
    int position_;
};
