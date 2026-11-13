// bindings.cpp
// The ONLY file that knows about Emscripten. It exposes the ds/ classes to
// JavaScript with Embind. Everything is returned BY VALUE: plain structs become
// JS objects, std::vector<T> becomes an Embind vector (read with .size()/.get(i),
// free with .delete()).
#include <emscripten/bind.h>

#include "../ds/BinarySearchTree.h"
#include "../ds/CircularQueue.h"
#include "../ds/DoublyLinkedList.h"
#include "../ds/LinkedList.h"
#include "../ds/NavigationHistory.h"
#include "../ds/Queue.h"
#include "../ds/Stack.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(nodecraft) {
    // ---- plain value types --------------------------------------------------
    value_object<Step>("Step")
        .field("action", &Step::action)
        .field("index", &Step::index)
        .field("value", &Step::value)
        .field("pointer", &Step::pointer)
        .field("message", &Step::message);

    value_object<LogEntry>("LogEntry")
        .field("number", &LogEntry::number)
        .field("function", &LogEntry::function)
        .field("argument", &LogEntry::argument)
        .field("success", &LogEntry::success)
        .field("result", &LogEntry::result);

    value_object<OpResult>("OpResult")
        .field("success", &OpResult::success)
        .field("message", &OpResult::message)
        .field("steps", &OpResult::steps)
        .field("entry", &OpResult::entry);

    value_object<QueueSlot>("QueueSlot")
        .field("index", &QueueSlot::index)
        .field("value", &QueueSlot::value)
        .field("occupied", &QueueSlot::occupied)
        .field("wasted", &QueueSlot::wasted)
        .field("used", &QueueSlot::used);

    value_object<TreeNodeView>("TreeNodeView")
        .field("value", &TreeNodeView::value)
        .field("x", &TreeNodeView::x)
        .field("y", &TreeNodeView::y)
        .field("leftValue", &TreeNodeView::leftValue)
        .field("rightValue", &TreeNodeView::rightValue)
        .field("hasLeft", &TreeNodeView::hasLeft)
        .field("hasRight", &TreeNodeView::hasRight);

    register_vector<int>("IntVector");
    register_vector<Step>("StepVector");
    register_vector<LogEntry>("LogEntryVector");
    register_vector<QueueSlot>("QueueSlotVector");
    register_vector<TreeNodeView>("TreeNodeVector");

    // ---- shared base: log, recent strip and step-through --------------------
    class_<TrackedStructure>("TrackedStructure")
        .function("clearLog", &TrackedStructure::clearLog)
        .function("getLog", &TrackedStructure::getLog)
        .function("getRecent", &TrackedStructure::getRecent)
        .function("logSize", &TrackedStructure::logSize)
        .function("stepNext", &TrackedStructure::stepNext)
        .function("stepPrev", &TrackedStructure::stepPrev)
        .function("currentStep", &TrackedStructure::currentStep)
        .function("hasNextStep", &TrackedStructure::hasNextStep)
        .function("hasPrevStep", &TrackedStructure::hasPrevStep)
        .function("stepPosition", &TrackedStructure::stepPosition)
        .function("stepCount", &TrackedStructure::stepCount);

    // ---- structures -----------------------------------------------------------
    class_<Stack, base<TrackedStructure>>("Stack")
        .constructor<>()
        .function("push", &Stack::push)
        .function("pop", &Stack::pop)
        .function("reset", &Stack::reset)
        .function("peek", &Stack::peek)
        .function("size", &Stack::size)
        .function("isEmpty", &Stack::isEmpty)
        .function("isFull", &Stack::isFull)
        .function("getTop", &Stack::getTop)
        .function("capacity", &Stack::capacity)
        .function("getItems", &Stack::getItems);

    class_<Queue, base<TrackedStructure>>("Queue")
        .constructor<>()
        .function("enqueue", &Queue::enqueue)
        .function("dequeue", &Queue::dequeue)
        .function("reset", &Queue::reset)
        .function("isEmpty", &Queue::isEmpty)
        .function("isFull", &Queue::isFull)
        .function("size", &Queue::size)
        .function("capacity", &Queue::capacity)
        .function("getFront", &Queue::getFront)
        .function("getRear", &Queue::getRear)
        .function("getSlots", &Queue::getSlots)
        .function("getItems", &Queue::getItems);

    class_<CircularQueue, base<TrackedStructure>>("CircularQueue")
        .constructor<>()
        .function("enqueue", &CircularQueue::enqueue)
        .function("dequeue", &CircularQueue::dequeue)
        .function("reset", &CircularQueue::reset)
        .function("isEmpty", &CircularQueue::isEmpty)
        .function("isFull", &CircularQueue::isFull)
        .function("size", &CircularQueue::size)
        .function("capacity", &CircularQueue::capacity)
        .function("getFront", &CircularQueue::getFront)
        .function("getRear", &CircularQueue::getRear)
        .function("getSlots", &CircularQueue::getSlots)
        .function("getItems", &CircularQueue::getItems);

    class_<SinglyLinkedList, base<TrackedStructure>>("SinglyLinkedList")
        .constructor<>()
        .function("insertAtStart", &SinglyLinkedList::insertAtStart)
        .function("insertAtEnd", &SinglyLinkedList::insertAtEnd)
        .function("insertAtPosition", &SinglyLinkedList::insertAtPosition)
        .function("deleteAtStart", &SinglyLinkedList::deleteAtStart)
        .function("deleteAtEnd", &SinglyLinkedList::deleteAtEnd)
        .function("deleteAtPosition", &SinglyLinkedList::deleteAtPosition)
        .function("search", &SinglyLinkedList::search)
        .function("reset", &SinglyLinkedList::reset)
        .function("getValues", &SinglyLinkedList::getValues)
        .function("size", &SinglyLinkedList::size)
        .function("isEmpty", &SinglyLinkedList::isEmpty);

    class_<DoublyLinkedList, base<TrackedStructure>>("DoublyLinkedList")
        .constructor<>()
        .function("insertAtStart", &DoublyLinkedList::insertAtStart)
        .function("insertAtEnd", &DoublyLinkedList::insertAtEnd)
        .function("insertAtPosition", &DoublyLinkedList::insertAtPosition)
        .function("deleteAtStart", &DoublyLinkedList::deleteAtStart)
        .function("deleteAtEnd", &DoublyLinkedList::deleteAtEnd)
        .function("deleteAtPosition", &DoublyLinkedList::deleteAtPosition)
        .function("search", &DoublyLinkedList::search)
        .function("reset", &DoublyLinkedList::reset)
        .function("getValues", &DoublyLinkedList::getValues)
        .function("size", &DoublyLinkedList::size)
        .function("isEmpty", &DoublyLinkedList::isEmpty);

    class_<BinarySearchTree, base<TrackedStructure>>("BinarySearchTree")
        .constructor<>()
        .function("insert", &BinarySearchTree::insert)
        .function("search", &BinarySearchTree::search)
        .function("remove", &BinarySearchTree::remove)
        .function("preorder", &BinarySearchTree::preorder)
        .function("inorder", &BinarySearchTree::inorder)
        .function("postorder", &BinarySearchTree::postorder)
        .function("reset", &BinarySearchTree::reset)
        .function("getNodes", &BinarySearchTree::getNodes)
        .function("size", &BinarySearchTree::size)
        .function("height", &BinarySearchTree::height)
        .function("isEmpty", &BinarySearchTree::isEmpty);

    class_<NavigationHistory>("NavigationHistory")
        .constructor<>()
        .function("open", &NavigationHistory::open)
        .function("back", &NavigationHistory::back)
        .function("current", &NavigationHistory::current)
        .function("canGoBack", &NavigationHistory::canGoBack)
        .function("depth", &NavigationHistory::depth);
}
