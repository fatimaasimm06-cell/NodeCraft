# NodeCraft

*See how data structures really work.*

NodeCraft is an interactive visualizer for stacks, queues, circular queues, singly and doubly linked lists, and binary search trees. **All data structure logic is C++17** (`ds/`), compiled to WebAssembly with Emscripten. The web page (`web/`) only draws what C++ returns.

```
NodeCraft/
  ds/      Common.h, TrackedStructure, HistoryLog, RecentOps, StepRecorder, StepTimeline,
           Stack, Queue, CircularQueue, LinkedList (singly), DoublyLinkedList,
           BinarySearchTree, NavigationHistory           (no Emscripten code here)
  wasm/    bindings.cpp (Embind), build.sh
  tests/   check.h (tiny assert helper), one test file per area, test_main.cpp
  web/     index.html, style.css, script.js  (+ nodecraft.js / nodecraft.wasm after `make wasm`)
  Makefile
```

## Build

| Command | What it does |
|---|---|
| `make test` | Native `g++` build of `ds/` + `tests/`, then runs every test. |
| `make wasm` | `em++` build to `web/nodecraft.js` + `web/nodecraft.wasm` (needs Emscripten on `PATH`, e.g. `source emsdk_env.sh`). |
| `make streamlit` | Single-file build with the wasm embedded: `web/nodecraft.single.js` (used by `streamlit_app.py`). |
| `make clean` | Removes `build/` (the native test build). The built web files are kept. |

`make test` uses `-fsanitize=address,undefined` when the compiler can link them (Linux/macOS). On toolchains without sanitizer runtimes, such as MinGW/w64devkit on Windows, it falls back to UBSan in trap mode. In every case the test runner counts every `new`/`delete` and fails a test group that leaks memory.

Run the site through a local server. Opening `index.html` directly won't work, because browsers refuse to load `.wasm` from `file://`:

```bash
cd web && python -m http.server 8000
```

Then open <http://localhost:8000>.

## Deploy on Streamlit

Streamlit can't serve `.js`/`.wasm` files with the right content types, so `streamlit_app.py` builds **one self-contained HTML page** and shows it with `components.html()`. It inlines `index.html` + `style.css` + `nodecraft.single.js` + `script.js`. `nodecraft.single.js` is the same C++ engine with the `.wasm` embedded inside the JS (`make streamlit`). All the logic still runs in C++.

1. Rebuild whenever `ds/` changes: `make wasm streamlit`, then commit `web/nodecraft.single.js`. Streamlit Cloud can't run Emscripten, so the built files must be in the repo.
2. Try it locally: `pip install -r requirements.txt`, then `streamlit run streamlit_app.py`.
3. Push the `NodeCraft` folder to a GitHub repository, with this folder as the repo root.
4. At <https://share.streamlit.io>, choose **Create app**, pick the repo and branch, and set the main file to `streamlit_app.py`.

Inside Streamlit the page runs in an iframe. If the browser refuses `history.pushState` there, the Back button calls the C++ `nav.back()` directly.

## Rules the backend follows

* No `std::map` / `unordered_map` / `set` / `unordered_set` / `multimap` anywhere. Only `vector`, `stack`, `queue`, `string`.
* The history log (singly linked), step timeline (doubly linked), both linked lists and the BST are hand-written with `new`/`delete`. Every owner frees everything in its destructor, and copying is deleted (rule of three).
* Stack, Queue and CircularQueue are fixed arrays with capacity 8.
* Nothing throws. Bad input returns `success: false` with a plain message, and the failure is still logged.
* Positions shown to users are 1-based (position 1 = head). Array slots are 0-based (`slot 0` .. `slot 7`).

## JavaScript API

```js
const M = await createNodeCraft();
```

Returned **vectors** are Embind objects. Read them with `.size()` / `.get(i)`, then call `.delete()`. Returned **structs** are plain JS objects.

### Shared shapes

| Type | Fields |
|---|---|
| `Step` | `action` (`"highlight" \| "movePointer" \| "insert" \| "remove" \| "link" \| "unlink" \| "visit" \| "found" \| "notFound" \| "error"`), `index` (0-based slot/node, -1 if none), `value`, `pointer` (`"top" "front" "rear" "head" "tail" "cur" "prev" "next"` or `""`), `message` |
| `LogEntry` | `number`, `function`, `argument`, `success`, `result` |
| `OpResult` | `success`, `message` (same text as `entry.result`), `steps` (StepVector), `entry` (LogEntry) |
| `QueueSlot` | `index`, `value`, `occupied`, `wasted` (linear queue: freed slot left of front), `used` (has held a value since reset) |
| `TreeNodeView` | `value`, `x` (inorder index), `y` (depth), `leftValue`, `rightValue`, `hasLeft`, `hasRight` |

**Step index layout.** Steps up to and including `remove` refer to the layout *before* the operation. The first `insert`, and every step after a `remove`, refer to the layout *after* it. Tree steps name the node by `value` and use `index = -1`. `link`/`unlink` steps name the node whose `pointer` field (`next`/`prev`) changes; `value` is the node it now points to (0 = NULL).

### Methods on every structure (Stack, Queue, CircularQueue, SinglyLinkedList, DoublyLinkedList, BinarySearchTree)

| Method | Returns |
|---|---|
| `reset()` | `OpResult`: clears the structure, its log and its Recent strip. It isn't logged (`entry.number` is 0). |
| `clearLog()` | nothing. Clears only the log and the Recent strip. |
| `getLog()` | `LogEntryVector`, oldest first |
| `getRecent()` | `LogEntryVector`, the last 5 entries, oldest → newest |
| `logSize()` | int |
| `stepNext()` / `stepPrev()` / `currentStep()` | `Step`. These move through the **last** operation's steps. At either end they stay put. |
| `hasNextStep()` / `hasPrevStep()` | bool |
| `stepPosition()` / `stepCount()` | int. Position 0 means before the first step. `stepPrev()` never goes below 1. |

### Per structure

| Class | Operations → `OpResult` | Queries |
|---|---|---|
| `Stack` | `push(v)`, `pop()` | `getItems()` IntVector bottom→top, `getTop()` (-1 if empty), `peek()`, `size()`, `isEmpty()`, `isFull()`, `capacity()` |
| `Queue` (linear) | `enqueue(v)`, `dequeue()` | `getSlots()` QueueSlotVector (8), `getItems()`, `getFront()` / `getRear()` (-1 if empty), `size()`, `isEmpty()`, `isFull()`, `capacity()` |
| `CircularQueue` | `enqueue(v)`, `dequeue()` | same as `Queue` |
| `SinglyLinkedList`, `DoublyLinkedList` | `insertAtStart(v)`, `insertAtEnd(v)`, `insertAtPosition(pos, v)` (1..size+1), `deleteAtStart()`, `deleteAtEnd()`, `deleteAtPosition(pos)` (1..size), `search(v)` | `getValues()` IntVector head→tail, `size()`, `isEmpty()` |
| `BinarySearchTree` | `insert(v)`, `search(v)`, `remove(v)` (delete), `preorder()`, `inorder()`, `postorder()` (result = `"20 30 40"`) | `getNodes()` TreeNodeVector in preorder, `size()`, `height()`, `isEmpty()` |
| `NavigationHistory` | `open(name)` (no-op if `name` is already on top), `back()` → new top string (`"home"` is never popped) | `current()`, `canGoBack()`, `depth()` |

### Example

```js
const M = await createNodeCraft();
const stack = new M.Stack();
const result = stack.push(10);                  // { success, message, steps, entry }
for (let i = 0; i < result.steps.size(); i++) {
  const step = result.steps.get(i);             // { action, index, value, pointer, message }
  console.log(step.action, step.index, step.message);
}
result.steps.delete();
const log = stack.getLog();
console.log(log.get(0).function, log.get(0).result); // "Push" "Success"
log.delete();
```

## Design decisions (not fixed by the spec)

1. **`TrackedStructure` base class.** Every structure inherits its HistoryLog, RecentOps, StepRecorder and StepTimeline, plus the log and step-through methods, from one base. Embind binds that base once with `base<>`.
2. **Error text.** The log result is `"Error: "` followed by the exact message, e.g. `Error: Stack underflow: the stack is empty`. The PRD's short example `Error: stack is empty` became this longer, more explicit text.
3. **Search "not found"** and **traversing an empty tree** return `success: false`, so they show in red.
4. **`reset()` isn't logged.** After a reset the log really is empty (HL-6).
5. **`peek`/`size` and similar queries** aren't logged. Only operations are.
6. **Linear queue** never moves `front`/`rear` back until `reset()`, even when it empties. `getSlots()` marks `wasted` slots so the page doesn't have to work that out.
7. **`insertAtPosition`** is logged as `Insert at Middle` with argument `pos 3, value 42`. `deleteAtPosition` is logged as `Delete at Position` / `pos 3`.
8. **Doubly list `deleteAtPosition`** walks `cur` onto the target node, since it can reach its `prev`. The singly list stops one node before.
9. **NavigationHistory** ignores `open("")`.
10. **BST delete** is `remove(v)`, because `delete` is the name Embind uses to free a C++ object from JS. It is logged as `Delete Node`. It handles all three cases recursively:
    * **Leaf:** delete it.
    * **One child:** the child takes the node's place.
    * **Two children:** copy the inorder successor (the smallest value in the right subtree) into the node, then unlink the successor's old node.

    The steps show the path, which case applies, the successor search (`found` on the successor), and a final `remove` on the deleted value.

## C++ code panel

Every section has a **C++** panel beside the drawing. When you run an operation, it shows that operation's code, and each animation step highlights the line that does that work. Step Back and Step Forward move the highlight too.

The snippets live in `web/script.js` (the `CODE` table). They are simplified from `ds/*.cpp`, with the step-recording calls left out so the algorithm reads cleanly. Each line can end with `##tags` naming the steps that light it up: `action[:pointer][ /regex on the step message/]`, separated by `;`. If a step matches no line, the highlight stays on the last line that matched. If you change an algorithm in `ds/`, update its snippet too.

## Frontend notes: where the API shaped the page

* **Step layouts.** Following the C++ index rule, a step is drawn on the BEFORE snapshot until the first `insert`, or until just after the first `remove`. The spec's "first structural step" rule would have switched too early for deletes, because their `link`/`unlink` steps happen on the old layout.
* **Pointer chips** start from the BEFORE snapshot and replay every `movePointer` step so far. For lists, chips shift one place when the drawing switches layouts, so `tail` stays on the right node after a middle insert.
* **Link arrows.** In delete operations, a `link` step on the old layout is drawn as a bypass arrow that skips one node; an `unlink` arrow retracts. In inserts, arrows linked by later steps stay hidden until their step.
* **Replay.** C++ has no "rewind" call, so Play at the last step calls `stepPrev()` back to step 1 and plays again.
* **Back button.** The header Back button calls `history.back()`, and the `popstate` handler calls `nav.back()`, so the on-page button and the browser button go through the same C++ stack. `openPage()` only pushes browser history when `nav.depth()` grows, which keeps the two stacks in step when `open()` ignores a duplicate.
* **Tree identity.** Tree nodes are matched between renders by value (values are unique), so they glide to new layout positions instead of jumping.
