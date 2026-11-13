// StepRecorder.cpp
#include "StepRecorder.h"

void StepRecorder::record(const Step& step) {
    steps_.push(step);
}

void StepRecorder::record(const std::string& action, int index, int value,
                          const std::string& pointer, const std::string& message) {
    steps_.push(makeStep(action, index, value, pointer, message));
}

std::vector<Step> StepRecorder::drain() {
    std::vector<Step> out;
    out.reserve(steps_.size());
    while (!steps_.empty()) {
        out.push_back(steps_.front());
        steps_.pop();
    }
    return out;
}

void StepRecorder::clear() {
    while (!steps_.empty()) {
        steps_.pop();
    }
}

int StepRecorder::size() const {
    return static_cast<int>(steps_.size());
}
