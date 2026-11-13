// check.h
// A tiny assert helper. No test framework is needed.
//   CHECK(cond)        - fails if cond is false
//   CHECK_EQ(a, b)     - fails if a != b, printing both values
#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace nc_test {
inline int& failures() { static int n = 0; return n; }
inline int& checks() { static int n = 0; return n; }

inline void report(bool ok, const char* expr, const char* file, int line, const std::string& extra) {
    checks()++;
    if (!ok) {
        failures()++;
        std::cerr << "  FAIL " << file << ":" << line << "  " << expr;
        if (!extra.empty()) std::cerr << "\n       " << extra;
        std::cerr << "\n";
    }
}

template <typename A, typename B>
void reportEq(const A& a, const B& b, const char* expr, const char* file, int line) {
    bool ok = (a == b);
    std::string extra;
    if (!ok) {
        std::ostringstream s;
        s << "got: [" << a << "]  expected: [" << b << "]";
        extra = s.str();
    }
    report(ok, expr, file, line, extra);
}

// Live heap allocations (counted in test_main.cpp). Used to catch leaks.
long liveAllocations();
}  // namespace nc_test

#define CHECK(cond) nc_test::report((cond), #cond, __FILE__, __LINE__, "")
#define CHECK_EQ(a, b) nc_test::reportEq((a), (b), #a " == " #b, __FILE__, __LINE__)

// Test groups (one file each).
void testHistory();
void testSteps();
void testStack();
void testQueue();
void testCircularQueue();
void testSinglyList();
void testDoublyList();
void testTree();
void testNavigation();

// Small helpers shared by the tests.
#include <vector>
inline std::string join(const std::vector<int>& v) {
    std::ostringstream s;
    for (size_t i = 0; i < v.size(); i++) s << (i ? " " : "") << v[i];
    return s.str();
}
