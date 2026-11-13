// NavigationHistory.cpp
#include "NavigationHistory.h"

NavigationHistory::NavigationHistory() {
    history_.push("home");
}

void NavigationHistory::open(const std::string& page) {
    if (page.empty()) return;              // ignore bad input
    if (history_.top() == page) return;    // already showing it
    history_.push(page);
}

std::string NavigationHistory::back() {
    if (history_.size() > 1) {
        history_.pop();
    }
    return history_.top();
}

std::string NavigationHistory::current() const {
    return history_.top();
}

bool NavigationHistory::canGoBack() const {
    return history_.size() > 1;
}

int NavigationHistory::depth() const {
    return static_cast<int>(history_.size());
}
