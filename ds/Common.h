// Common.h
// Shared types used by every NodeCraft data structure.
// Nothing here knows about web pages or drawing; these are plain C++ structs.
#pragma once

#include <string>
#include <vector>

// Fixed capacity used by Stack, Queue and CircularQueue (PRD ST-5, QU-4, CQ-1).
const int NC_CAPACITY = 8;

// One animation step. The frontend plays these one after another.
//
// action  : "highlight", "movePointer", "insert", "remove", "link", "unlink",
//           "visit", "found", "notFound", "error"
// index   : slot / node index the step refers to (0-based, -1 if none).
//           Steps up to and including "remove" use the layout BEFORE the
//           operation; the first "insert" and every step after a "remove"
//           use the layout AFTER it.
// value   : value involved (0 if none)
// pointer : pointer name for movePointer / link / unlink
//           ("top", "front", "rear", "head", "tail", "cur", "prev", "next")
// message : short plain-English explanation shown under the animation (AN-3)
struct Step {
    std::string action;
    int index = -1;
    int value = 0;
    std::string pointer;
    std::string message;
};

// One row in a structure's history log (HL-3).
struct LogEntry {
    int number = 0;        // 1, 2, 3... per structure
    std::string function;  // "Push", "Pop", "Insert at Start", ...
    std::string argument;  // "10", "pos 3", "" for none
    bool success = false;
    std::string result;    // "Success", "Success (removed 25)", "Error: ..."
};

// What every operation returns to the frontend.
struct OpResult {
    bool success = false;
    std::string message;      // same text as entry.result
    std::vector<Step> steps;  // animation steps, in order
    LogEntry entry;           // the history entry just added
};

// One slot of an array-based queue, so the frontend can draw all 8 boxes
// without working anything out on its own.
struct QueueSlot {
    int index = 0;          // 0..7
    int value = 0;          // only meaningful when occupied
    bool occupied = false;  // holds a live value right now
    bool wasted = false;    // linear queue only: freed slot left of front, cannot be reused
    bool used = false;      // this slot has held a value at some point since the last reset
};

// Small helper so every file builds steps the same way.
inline Step makeStep(const std::string& action, int index, int value,
                     const std::string& pointer, const std::string& message) {
    Step s;
    s.action = action;
    s.index = index;
    s.value = value;
    s.pointer = pointer;
    s.message = message;
    return s;
}
