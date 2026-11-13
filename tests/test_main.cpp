// test_main.cpp
// Runs every test group. Each group runs inside its own scope and we check that
// the number of live heap allocations is back to where it started, so a missing
// delete makes the run fail (TR-3) even on compilers without AddressSanitizer.
#include <cstdlib>
#include <new>

#include "check.h"

// ---------- allocation counter ----------
// When AddressSanitizer is on it replaces operator new itself, so we skip this.
#ifndef NODECRAFT_SANITIZE
static long g_live = 0;

void* operator new(std::size_t n) {
    void* p = std::malloc(n ? n : 1);
    if (!p) throw std::bad_alloc();
    g_live++;
    return p;
}
void* operator new[](std::size_t n) { return operator new(n); }
void operator delete(void* p) noexcept {
    if (p) { g_live--; std::free(p); }
}
void operator delete[](void* p) noexcept { operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { operator delete(p); }

long nc_test::liveAllocations() { return g_live; }
#else
long nc_test::liveAllocations() { return 0; }
#endif

static void runGroup(const char* name, void (*fn)()) {
    int before = nc_test::failures();
    long liveBefore = nc_test::liveAllocations();
    fn();
    long leaked = nc_test::liveAllocations() - liveBefore;
    nc_test::report(leaked == 0, "no heap memory leaked", name, 0,
                    leaked ? std::to_string(leaked) + " allocation(s) not freed" : "");
    std::cout << (nc_test::failures() == before ? "  ok    " : "  FAIL  ") << name << "\n";
}

int main() {
    std::cout << "NodeCraft backend tests\n";
    runGroup("HistoryLog / RecentOps", testHistory);
    runGroup("StepRecorder / StepTimeline", testSteps);
    runGroup("Stack", testStack);
    runGroup("Queue", testQueue);
    runGroup("CircularQueue", testCircularQueue);
    runGroup("SinglyLinkedList", testSinglyList);
    runGroup("DoublyLinkedList", testDoublyList);
    runGroup("BinarySearchTree", testTree);
    runGroup("NavigationHistory", testNavigation);

    std::cout << nc_test::checks() << " checks, " << nc_test::failures() << " failed\n";
    return nc_test::failures() == 0 ? 0 : 1;
}
