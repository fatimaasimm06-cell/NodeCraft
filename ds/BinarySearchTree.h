// BinarySearchTree.h
// Hand-written Binary Search Tree: smaller values go left, larger go right.
// Insert, search, traversals and layout are all RECURSIVE.
//
// Tree steps identify a node by its VALUE (values are unique); index is -1.
#pragma once

#include "TrackedStructure.h"

// What the frontend needs to draw one node and the line to its children.
struct TreeNodeView {
    int value = 0;
    int x = 0;           // inorder index (0, 1, 2, ...), unique per node
    int y = 0;           // depth (root = 0)
    int leftValue = 0;   // only meaningful when hasLeft
    int rightValue = 0;  // only meaningful when hasRight
    bool hasLeft = false;
    bool hasRight = false;
};

class BinarySearchTree : public TrackedStructure {
public:
    BinarySearchTree();
    ~BinarySearchTree() override;

    OpResult insert(int value);  // BT-1, BT-9
    OpResult search(int value);  // BT-8
    OpResult remove(int value);  // delete a node: leaf, one child, or two children
    OpResult preorder();         // BT-3  Root, Left, Right
    OpResult inorder();          // BT-4  Left, Root, Right (sorted)
    OpResult postorder();        // BT-5  Left, Right, Root
    OpResult reset();            // BT-7

    std::vector<TreeNodeView> getNodes() const;  // in preorder
    int size() const;
    int height() const;  // number of levels, 0 for an empty tree
    bool isEmpty() const;

private:
    struct Node {
        int value;
        Node* left;
        Node* right;
        int x;  // layout: inorder index
        int y;  // layout: depth
    };

    Node* root_;
    int count_;

    // Recursive helpers.
    bool insertRec(Node*& node, int value, int depth);
    bool searchRec(Node* node, int value, int depth);
    bool removeRec(Node*& node, int value);
    Node*& minLink(Node*& node, bool firstStep);  // the pointer that holds the smallest node
    void preorderRec(Node* node, std::string& out);
    void inorderRec(Node* node, std::string& out);
    void postorderRec(Node* node, std::string& out);
    void layoutRec(Node* node, int depth, int& nextX);
    void collectRec(const Node* node, std::vector<TreeNodeView>& out) const;
    int heightRec(const Node* node) const;
    void freeRec(Node* node);

    void visit(Node* node, std::string& out, const std::string& order);
    OpResult traversal(const std::string& name);
};
