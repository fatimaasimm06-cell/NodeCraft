// BinarySearchTree.cpp
#include "BinarySearchTree.h"

BinarySearchTree::BinarySearchTree() : root_(nullptr), count_(0) {}

BinarySearchTree::~BinarySearchTree() {
    freeRec(root_);
}

// Post-order delete: free both children before the node itself.
void BinarySearchTree::freeRec(Node* node) {
    if (node == nullptr) return;
    freeRec(node->left);
    freeRec(node->right);
    delete node;
}

// ---------- insert ----------

// Returns false if the value is already in the tree (nothing is changed).
bool BinarySearchTree::insertRec(Node*& node, int value, int depth) {
    const std::string v = toText(value);
    if (node == nullptr) {
        node = new Node{value, nullptr, nullptr, 0, depth};
        if (depth == 0) {
            record("insert", -1, value, "", "The tree is empty, so " + v + " becomes the root");
        } else {
            record("insert", -1, value, "", "Empty spot found: place " + v + " here");
        }
        return true;
    }

    const std::string n = toText(node->value);
    if (value == node->value) {
        record("error", -1, node->value, "", "Value " + v + " is already in the tree");
        return false;
    }
    if (value < node->value) {
        record("highlight", -1, node->value, "", v + " < " + n + ", go left");
        return insertRec(node->left, value, depth + 1);
    }
    record("highlight", -1, node->value, "", v + " > " + n + ", go right");
    return insertRec(node->right, value, depth + 1);
}

OpResult BinarySearchTree::insert(int value) {
    const std::string v = toText(value);
    if (!insertRec(root_, value, 0)) {
        // The "error" step was already recorded on the existing node.
        return finish("Insert Node", v, false, "Error: Value " + v + " is already in the tree");
    }
    count_++;
    int nextX = 0;
    layoutRec(root_, 0, nextX);  // recompute positions after every insert
    return finish("Insert Node", v, true, "Success");
}

// ---------- search ----------

bool BinarySearchTree::searchRec(Node* node, int value, int depth) {
    const std::string v = toText(value);
    if (node == nullptr) {
        record("notFound", -1, value, "", "Reached an empty spot: " + v + " is not in the tree");
        return false;
    }
    const std::string n = toText(node->value);
    if (value == node->value) {
        record("found", -1, node->value, "",
               "Found " + v + " at depth " + toText(depth) + " (" + v + " = " + n + ")");
        return true;
    }
    if (value < node->value) {
        record("highlight", -1, node->value, "", v + " < " + n + ", go left");
        return searchRec(node->left, value, depth + 1);
    }
    record("highlight", -1, node->value, "", v + " > " + n + ", go right");
    return searchRec(node->right, value, depth + 1);
}

OpResult BinarySearchTree::search(int value) {
    const std::string v = toText(value);
    if (root_ == nullptr) {
        record("notFound", -1, value, "", "The tree is empty, so " + v + " is not in it");
        return finish("Search", v, false, "Not found: " + v + " is not in the tree");
    }
    if (searchRec(root_, value, 0)) {
        return finish("Search", v, true, "Found " + v);
    }
    return finish("Search", v, false, "Not found: " + v + " is not in the tree");
}

// ---------- delete ----------

// Follows left pointers down to the smallest node of a subtree and returns a
// REFERENCE to the pointer that holds it, so the caller can unlink it directly.
BinarySearchTree::Node*& BinarySearchTree::minLink(Node*& node, bool firstStep) {
    if (firstStep) {
        record("highlight", -1, node->value, "",
               "Successor search: step right to " + toText(node->value) +
                   ", then go left as far as possible");
    } else {
        record("highlight", -1, node->value, "", "Successor search: go left to " + toText(node->value));
    }
    if (node->left == nullptr) {
        return node;  // no smaller value below: this is the minimum
    }
    return minLink(node->left, false);
}

// Returns false if the value is not in the tree.
bool BinarySearchTree::removeRec(Node*& node, int value) {
    const std::string v = toText(value);
    if (node == nullptr) {
        record("notFound", -1, value, "", "Reached an empty spot: " + v + " is not in the tree");
        return false;
    }

    const std::string n = toText(node->value);
    if (value < node->value) {
        record("highlight", -1, node->value, "", v + " < " + n + ", go left");
        return removeRec(node->left, value);
    }
    if (value > node->value) {
        record("highlight", -1, node->value, "", v + " > " + n + ", go right");
        return removeRec(node->right, value);
    }

    // Found the node to delete. There are three cases.

    // Case 1: a leaf (no children). Just delete it.
    if (node->left == nullptr && node->right == nullptr) {
        record("highlight", -1, value, "", "Found " + v + ". It is a leaf (no children)");
        record("remove", -1, value, "", "Case 1: just delete " + v + " and set its parent's pointer to NULL");
        delete node;
        node = nullptr;
        return true;
    }

    // Case 2: exactly one child. The child takes the node's place.
    if (node->left == nullptr || node->right == nullptr) {
        Node* child = (node->left != nullptr) ? node->left : node->right;
        const std::string c = toText(child->value);
        record("highlight", -1, value, "", "Found " + v + ". It has one child (" + c + ")");
        record("remove", -1, value, "",
               "Case 2: " + v + "'s parent now points to " + c + ", then " + v + " is deleted");
        delete node;
        node = child;
        return true;
    }

    // Case 3: two children. Copy the inorder successor (smallest value in the
    // right subtree) into this node, then delete the successor's old node.
    record("highlight", -1, value, "", "Found " + v + ". It has two children");
    Node*& succLink = minLink(node->right, true);
    Node* succ = succLink;
    const std::string s = toText(succ->value);
    record("found", -1, succ->value, "",
           s + " is the inorder successor: the smallest value bigger than " + v);
    record("remove", -1, value, "",
           "Case 3: copy " + s + " into " + v + "'s node, then delete the old " + s + " node");
    node->value = succ->value;
    succLink = succ->right;  // the successor never has a left child
    delete succ;
    return true;
}

OpResult BinarySearchTree::remove(int value) {
    const std::string v = toText(value);
    if (root_ == nullptr) {
        record("notFound", -1, value, "", "The tree is empty, so " + v + " is not in it");
        return finish("Delete Node", v, false, "Not found: " + v + " is not in the tree");
    }
    if (!removeRec(root_, value)) {
        return finish("Delete Node", v, false, "Not found: " + v + " is not in the tree");
    }
    count_--;
    int nextX = 0;
    layoutRec(root_, 0, nextX);  // recompute positions after the change
    return finish("Delete Node", v, true, "Success (removed " + v + ")");
}

// ---------- traversals ----------

void BinarySearchTree::visit(Node* node, std::string& out, const std::string& order) {
    if (!out.empty()) out += " ";
    out += toText(node->value);
    record("visit", -1, node->value, "", "Visit " + toText(node->value) + " (" + order + ")");
}

void BinarySearchTree::preorderRec(Node* node, std::string& out) {
    if (node == nullptr) return;
    visit(node, out, "Root, Left, Right");
    preorderRec(node->left, out);
    preorderRec(node->right, out);
}

void BinarySearchTree::inorderRec(Node* node, std::string& out) {
    if (node == nullptr) return;
    inorderRec(node->left, out);
    visit(node, out, "Left, Root, Right");
    inorderRec(node->right, out);
}

void BinarySearchTree::postorderRec(Node* node, std::string& out) {
    if (node == nullptr) return;
    postorderRec(node->left, out);
    postorderRec(node->right, out);
    visit(node, out, "Left, Right, Root");
}

OpResult BinarySearchTree::traversal(const std::string& name) {
    if (root_ == nullptr) {
        return fail(name, "", -1, 0, "The tree is empty: insert a value first");
    }
    std::string out;
    if (name == "Preorder") preorderRec(root_, out);
    else if (name == "Inorder") inorderRec(root_, out);
    else postorderRec(root_, out);
    return finish(name, "", true, out);
}

OpResult BinarySearchTree::preorder() { return traversal("Preorder"); }
OpResult BinarySearchTree::inorder() { return traversal("Inorder"); }
OpResult BinarySearchTree::postorder() { return traversal("Postorder"); }

// ---------- layout and drawing data ----------

// x = inorder index, y = depth. Inorder indices are unique, so nodes never overlap.
void BinarySearchTree::layoutRec(Node* node, int depth, int& nextX) {
    if (node == nullptr) return;
    layoutRec(node->left, depth + 1, nextX);
    node->x = nextX++;
    node->y = depth;
    layoutRec(node->right, depth + 1, nextX);
}

void BinarySearchTree::collectRec(const Node* node, std::vector<TreeNodeView>& out) const {
    if (node == nullptr) return;
    TreeNodeView view;
    view.value = node->value;
    view.x = node->x;
    view.y = node->y;
    view.hasLeft = node->left != nullptr;
    view.hasRight = node->right != nullptr;
    view.leftValue = view.hasLeft ? node->left->value : 0;
    view.rightValue = view.hasRight ? node->right->value : 0;
    out.push_back(view);
    collectRec(node->left, out);
    collectRec(node->right, out);
}

std::vector<TreeNodeView> BinarySearchTree::getNodes() const {
    std::vector<TreeNodeView> out;
    collectRec(root_, out);
    return out;
}

int BinarySearchTree::heightRec(const Node* node) const {
    if (node == nullptr) return 0;
    int l = heightRec(node->left);
    int r = heightRec(node->right);
    return 1 + (l > r ? l : r);
}

int BinarySearchTree::height() const { return heightRec(root_); }
int BinarySearchTree::size() const { return count_; }
bool BinarySearchTree::isEmpty() const { return count_ == 0; }

OpResult BinarySearchTree::reset() {
    freeRec(root_);
    root_ = nullptr;
    count_ = 0;
    return finishReset("The tree was reset. Every node was freed and the log starts again.");
}
