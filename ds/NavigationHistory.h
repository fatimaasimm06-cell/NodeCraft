// NavigationHistory.h
// Which section of the page is shown is decided by this stack (PRD 6.10).
// It starts with "home", and "home" is never popped.
#pragma once

#include <stack>
#include <string>

class NavigationHistory {
public:
    NavigationHistory();

    // NV-2: push a section name. Opening the section that is already on top
    // does nothing, so a double click cannot add duplicate entries.
    void open(const std::string& page);

    // NV-3: pop the current section (unless only "home" is left) and return the new top.
    std::string back();

    std::string current() const;
    bool canGoBack() const;  // NV-4: false when only "home" is on the stack
    int depth() const;       // number of names on the stack

private:
    std::stack<std::string> history_;
};
