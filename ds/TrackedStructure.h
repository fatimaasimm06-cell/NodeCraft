// TrackedStructure.h
// Base class shared by every NodeCraft data structure.
// It owns the four helpers each structure needs:
//   HistoryLog   - full log of every operation (singly linked list)
//   RecentOps    - last 5 operations (circular queue)
//   StepRecorder - collects animation steps while an operation runs (queue)
//   StepTimeline - lets the user step back and forth (doubly linked list)
#pragma once

#include "Common.h"
#include "HistoryLog.h"
#include "RecentOps.h"
#include "StepRecorder.h"
#include "StepTimeline.h"

class TrackedStructure {
public:
    TrackedStructure() = default;
    virtual ~TrackedStructure() = default;

    TrackedStructure(const TrackedStructure&) = delete;
    TrackedStructure& operator=(const TrackedStructure&) = delete;

    // Clears ONLY the log and the Recent strip (HL-7). The structure is kept.
    void clearLog();

    std::vector<LogEntry> getLog() const;     // oldest first
    std::vector<LogEntry> getRecent() const;  // oldest -> newest, at most 5
    int logSize() const;

    const StepTimeline& getTimeline() const;

    // Step-through controls for the last operation (AN-5).
    Step stepNext();
    Step stepPrev();
    Step currentStep() const;
    bool hasNextStep() const;
    bool hasPrevStep() const;
    int stepPosition() const;  // 0 = before first step, 1..stepCount()
    int stepCount() const;

protected:
    // Adds one animation step for the operation that is running now.
    void record(const std::string& action, int index, int value,
                const std::string& pointer, const std::string& message);

    // Ends an operation: logs it (log + recent), loads its steps into the
    // timeline and builds the OpResult returned to the caller.
    OpResult finish(const std::string& function, const std::string& argument,
                    bool success, const std::string& result);

    // Shortcut for a failed operation: records an "error" step and finishes.
    OpResult fail(const std::string& function, const std::string& argument,
                  int index, int value, const std::string& errorText);

    // Used by reset(): clears the log, recent strip and timeline, and returns
    // an unlogged OpResult with one explanatory step.
    OpResult finishReset(const std::string& message);

private:
    HistoryLog log_;
    RecentOps recent_;
    StepRecorder recorder_;
    StepTimeline timeline_;
};

// Turns an int into text, e.g. 42 -> "42".
std::string toText(int value);
