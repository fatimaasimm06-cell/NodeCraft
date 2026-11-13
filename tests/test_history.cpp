// test_history.cpp - HistoryLog (singly linked list) and RecentOps (circular buffer)
#include "check.h"
#include "../ds/HistoryLog.h"
#include "../ds/RecentOps.h"
#include "../ds/Stack.h"

void testHistory() {
    // Numbering, append at end, clear.
    {
        HistoryLog log;
        CHECK_EQ(log.size(), 0);
        LogEntry a = log.append("Push", "10", true, "Success");
        LogEntry b = log.append("Pop", "", true, "Success (removed 10)");
        CHECK_EQ(a.number, 1);
        CHECK_EQ(b.number, 2);
        std::vector<LogEntry> v = log.toVector();
        CHECK_EQ((int)v.size(), 2);
        CHECK_EQ(v[0].function, std::string("Push"));
        CHECK_EQ(v[1].result, std::string("Success (removed 10)"));
        log.clear();
        CHECK_EQ(log.size(), 0);
        CHECK_EQ(log.append("Push", "5", true, "Success").number, 1);  // restarts at 1
    }

    // RecentOps keeps only the last 5, overwriting the oldest.
    {
        RecentOps r;
        for (int i = 1; i <= 3; i++) {
            LogEntry e; e.number = i; r.add(e);
        }
        CHECK_EQ(r.size(), 3);
        CHECK_EQ(r.toVector()[0].number, 1);
        for (int i = 4; i <= 7; i++) {
            LogEntry e; e.number = i; r.add(e);
        }
        std::vector<LogEntry> v = r.toVector();
        CHECK_EQ((int)v.size(), 5);
        CHECK_EQ(v[0].number, 3);  // 1 and 2 were overwritten
        CHECK_EQ(v[4].number, 7);  // newest last
        r.clear();
        CHECK_EQ(r.size(), 0);
    }

    // Structures: every operation (also failed ones) is logged; clearLog vs reset.
    {
        Stack s;
        s.push(1);
        s.pop();
        s.pop();  // fails, still logged
        CHECK_EQ(s.logSize(), 3);
        CHECK_EQ((int)s.getRecent().size(), 3);
        CHECK(!s.getLog()[2].success);

        s.push(7);
        s.clearLog();  // only log + recent
        CHECK_EQ(s.logSize(), 0);
        CHECK_EQ((int)s.getRecent().size(), 0);
        CHECK_EQ(s.size(), 1);  // structure kept
        CHECK_EQ(s.push(8).entry.number, 1);

        OpResult r = s.reset();  // structure + log + recent
        CHECK(r.success);
        CHECK_EQ(r.entry.number, 0);
        CHECK_EQ(s.size(), 0);
        CHECK_EQ(s.logSize(), 0);
        CHECK_EQ((int)s.getRecent().size(), 0);

        for (int i = 0; i < 7; i++) s.push(i);
        std::vector<LogEntry> recent = s.getRecent();
        CHECK_EQ((int)recent.size(), 5);
        CHECK_EQ(recent[0].number, 3);
        CHECK_EQ(recent[4].number, 7);
    }
}
