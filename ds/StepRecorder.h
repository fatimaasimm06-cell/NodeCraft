// StepRecorder.h
// Collects animation steps while an operation runs (AN-1).
// Steps go into a std::queue, so they come out first-in, first-out.
#pragma once

#include <queue>
#include "Common.h"

class StepRecorder {
public:
    void record(const Step& step);
    void record(const std::string& action, int index, int value,
                const std::string& pointer, const std::string& message);

    // Pops every step in FIFO order into a vector. The recorder is empty afterwards.
    std::vector<Step> drain();

    void clear();
    int size() const;

private:
    std::queue<Step> steps_;
};
