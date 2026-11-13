// test_bst.cpp
#include "check.h"
#include "../ds/BinarySearchTree.h"

void testTree() {
    BinarySearchTree t;

    // Empty-tree behaviour.
    CHECK(!t.inorder().success);
    CHECK(!t.search(1).success);
    CHECK_EQ(t.height(), 0);

    int order[] = {50, 30, 70, 20, 40, 60, 80};
    for (int v : order) CHECK(t.insert(v).success);
    CHECK_EQ(t.size(), 7);
    CHECK_EQ(t.height(), 3);

    CHECK_EQ(t.inorder().message, std::string("20 30 40 50 60 70 80"));
    CHECK_EQ(t.preorder().message, std::string("50 30 20 40 70 60 80"));
    CHECK_EQ(t.postorder().message, std::string("20 40 30 60 80 70 50"));
    OpResult in = t.inorder();
    CHECK_EQ((int)in.steps.size(), 7);
    CHECK_EQ(in.steps[0].action, std::string("visit"));
    CHECK_EQ(in.steps[0].value, 20);
    CHECK_EQ(in.entry.result, std::string("20 30 40 50 60 70 80"));

    // Duplicates are rejected and the tree does not change.
    OpResult dup = t.insert(40);
    CHECK(!dup.success);
    CHECK_EQ(dup.message, std::string("Error: Value 40 is already in the tree"));
    CHECK_EQ(dup.steps.back().action, std::string("error"));
    CHECK_EQ(dup.steps.back().value, 40);
    CHECK_EQ(t.size(), 7);

    // Insert path steps.
    OpResult ins = t.insert(45);
    CHECK_EQ((int)ins.steps.size(), 4);  // 50, 30, 40, then insert
    CHECK_EQ(ins.steps[0].message, std::string("45 < 50, go left"));
    CHECK_EQ(ins.steps[1].message, std::string("45 > 30, go right"));
    CHECK_EQ(ins.steps[2].message, std::string("45 > 40, go right"));
    CHECK_EQ(ins.steps[3].action, std::string("insert"));

    // Search path steps.
    OpResult f = t.search(60);
    CHECK(f.success);
    CHECK_EQ((int)f.steps.size(), 3);
    CHECK_EQ(f.steps[0].value, 50);
    CHECK_EQ(f.steps[1].value, 70);
    CHECK_EQ(f.steps[2].action, std::string("found"));
    OpResult nf = t.search(65);
    CHECK(!nf.success);
    CHECK_EQ(nf.steps.back().action, std::string("notFound"));

    // Layout: unique x (inorder index), y = depth, preorder order.
    std::vector<TreeNodeView> nodes = t.getNodes();
    CHECK_EQ((int)nodes.size(), 8);
    CHECK_EQ(nodes[0].value, 50);
    CHECK_EQ(nodes[0].y, 0);
    CHECK(nodes[0].hasLeft && nodes[0].leftValue == 30);
    CHECK(nodes[0].hasRight && nodes[0].rightValue == 70);
    bool seen[8] = {false, false, false, false, false, false, false, false};
    bool unique = true;
    for (const TreeNodeView& n : nodes) {
        if (n.x < 0 || n.x >= 8 || seen[n.x]) unique = false;
        else seen[n.x] = true;
        if (n.value == 20) CHECK_EQ(n.x, 0);
        if (n.value == 80) CHECK_EQ(n.x, 7);
        if (n.value == 45) CHECK_EQ(n.y, 3);
    }
    CHECK(unique);

    // ---- delete ----
    // Tree now: 50 30 70 20 40 60 80 45
    OpResult miss = t.remove(65);
    CHECK(!miss.success);
    CHECK_EQ(miss.message, std::string("Not found: 65 is not in the tree"));
    CHECK_EQ(miss.steps.back().action, std::string("notFound"));
    CHECK_EQ(miss.entry.function, std::string("Delete Node"));
    CHECK_EQ(t.size(), 8);

    // Case 1: leaf.
    OpResult leaf = t.remove(20);
    CHECK(leaf.success);
    CHECK_EQ(leaf.message, std::string("Success (removed 20)"));
    CHECK_EQ(leaf.steps[0].message, std::string("20 < 50, go left"));
    CHECK_EQ(leaf.steps.back().action, std::string("remove"));
    CHECK(leaf.steps.back().message.find("Case 1") == 0);
    CHECK_EQ(t.inorder().message, std::string("30 40 45 50 60 70 80"));

    // Case 2: one child (40 has only 45).
    OpResult one = t.remove(40);
    CHECK(one.success);
    CHECK(one.steps.back().message.find("Case 2") == 0);
    CHECK_EQ(t.inorder().message, std::string("30 45 50 60 70 80"));
    CHECK_EQ(t.preorder().message, std::string("50 30 45 70 60 80"));

    // Case 3: two children at the root. Successor of 50 is 60.
    OpResult two = t.remove(50);
    CHECK(two.success);
    bool sawSuccessor = false;
    for (const Step& s : two.steps) {
        if (s.action == "found" && s.value == 60) sawSuccessor = true;
    }
    CHECK(sawSuccessor);
    CHECK(two.steps.back().message.find("Case 3") == 0);
    CHECK_EQ(t.preorder().message, std::string("60 30 45 70 80"));
    CHECK_EQ(t.inorder().message, std::string("30 45 60 70 80"));
    CHECK_EQ(t.size(), 5);
    CHECK_EQ(t.getNodes()[0].value, 60);

    // Successor deeper down: 30 has two children after inserting 35 and 40.
    t.insert(35);
    t.insert(40);  // tree: 60 (30 (-, 45 (35 (-, 40), -)), 70 (-, 80))
    OpResult deep = t.remove(30);  // 30 has only a right child (45): case 2
    CHECK(deep.success);
    CHECK_EQ(t.inorder().message, std::string("35 40 45 60 70 80"));
    t.insert(30);                  // 30 goes left of 35
    OpResult deep2 = t.remove(45); // 45 has one child (35)
    CHECK(deep2.success);
    CHECK_EQ(t.inorder().message, std::string("30 35 40 60 70 80"));
    OpResult rootDel = t.remove(60);  // successor 70 has a right child (80)
    CHECK(rootDel.success);
    CHECK_EQ(t.preorder().message, std::string("70 35 30 40 80"));

    // Layout stays unique after deletes.
    std::vector<TreeNodeView> after = t.getNodes();
    bool seenAfter[5] = {false, false, false, false, false};
    bool uniqueAfter = true;
    for (const TreeNodeView& n : after) {
        if (n.x < 0 || n.x >= 5 || seenAfter[n.x]) uniqueAfter = false;
        else seenAfter[n.x] = true;
    }
    CHECK(uniqueAfter);

    // Delete everything, then delete from an empty tree.
    int rest[] = {70, 35, 30, 40, 80};
    for (int v : rest) CHECK(t.remove(v).success);
    CHECK(t.isEmpty());
    CHECK(!t.remove(1).success);
    CHECK_EQ(t.height(), 0);

    t.reset();
    CHECK(t.isEmpty());
    CHECK_EQ(t.logSize(), 0);
    CHECK_EQ((int)t.getNodes().size(), 0);

    // A tall, one-sided tree, then let the destructor free it.
    for (int v = 1; v <= 20; v++) t.insert(v);
    CHECK_EQ(t.height(), 20);
}
