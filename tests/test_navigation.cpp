// test_navigation.cpp - reproduces the PRD 6.10 table.
#include "check.h"
#include "../ds/NavigationHistory.h"

void testNavigation() {
    NavigationHistory nav;
    CHECK_EQ(nav.current(), std::string("home"));      // page loads
    CHECK(!nav.canGoBack());

    nav.open("stack");                                  // clicks Stack card
    CHECK_EQ(nav.current(), std::string("stack"));
    CHECK(nav.canGoBack());

    nav.open("queue");                                  // opens Queue
    CHECK_EQ(nav.depth(), 3);

    nav.open("queue");                                  // double click: no duplicate
    CHECK_EQ(nav.depth(), 3);

    CHECK_EQ(nav.back(), std::string("stack"));         // Back
    CHECK_EQ(nav.back(), std::string("home"));          // Back
    CHECK(!nav.canGoBack());                            // Back disabled

    CHECK_EQ(nav.back(), std::string("home"));          // back at home stays home
    CHECK_EQ(nav.depth(), 1);

    nav.open("");                                       // bad input ignored
    CHECK_EQ(nav.depth(), 1);
}
