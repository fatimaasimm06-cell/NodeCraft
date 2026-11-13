// TrackedStructure.cpp
#include "TrackedStructure.h"

#include <sstream>

std::string toText(int value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

void TrackedStructure::clearLog() {
    log_.clear();
    recent_.clear();
}

std::vector<LogEntry> TrackedStructure::getLog() const { return log_.toVector(); }
std::vector<LogEntry> TrackedStructure::getRecent() const { return recent_.toVector(); }
int TrackedStructure::logSize() const { return log_.size(); }
const StepTimeline& TrackedStructure::getTimeline() const { return timeline_; }

Step TrackedStructure::stepNext() { return timeline_.next(); }
Step TrackedStructure::stepPrev() { return timeline_.prev(); }
Step TrackedStructure::currentStep() const { return timeline_.current(); }
bool TrackedStructure::hasNextStep() const { return timeline_.hasNext(); }
bool TrackedStructure::hasPrevStep() const { return timeline_.hasPrev(); }
int TrackedStructure::stepPosition() const { return timeline_.position(); }
int TrackedStructure::stepCount() const { return timeline_.size(); }

void TrackedStructure::record(const std::string& action, int index, int value,
                              const std::string& pointer, const std::string& message) {
    recorder_.record(action, index, value, pointer, message);
}

OpResult TrackedStructure::finish(const std::string& function, const std::string& argument,
                                  bool success, const std::string& result) {
    OpResult out;
    out.success = success;
    out.message = result;
    out.steps = recorder_.drain();        // FIFO order
    timeline_.load(out.steps);            // ready for Step Forward / Step Back
    out.entry = log_.append(function, argument, success, result);
    recent_.add(out.entry);
    return out;
}

OpResult TrackedStructure::fail(const std::string& function, const std::string& argument,
                                int index, int value, const std::string& errorText) {
    record("error", index, value, "", errorText);
    return finish(function, argument, false, "Error: " + errorText);
}

OpResult TrackedStructure::finishReset(const std::string& message) {
    log_.clear();
    recent_.clear();
    recorder_.clear();
    recorder_.record("highlight", -1, 0, "", message);

    OpResult out;
    out.success = true;
    out.message = message;
    out.steps = recorder_.drain();
    timeline_.load(out.steps);
    // Reset is not stored in the log (the log is now empty). The entry is
    // returned only so the frontend can show it; number 0 means "not logged".
    out.entry.number = 0;
    out.entry.function = "Reset";
    out.entry.argument = "";
    out.entry.success = true;
    out.entry.result = "Success";
    return out;
}
