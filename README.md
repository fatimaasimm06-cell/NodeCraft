# StructViz

**See how data structures really work.**

StructViz is an interactive data structure visualizer for students learning data structures in C++. You perform operations with buttons and input boxes and watch the structure change on screen, step by step. Every structure keeps its own history log, so you can look back over everything you did.

The app is also built on the structures it teaches. Page navigation runs on a stack, animations play from a queue, and history logs are linked lists.

## 🚀 Live Demo

**👉 [Open StructViz on Streamlit](https://nodecraft-go6coqss2oq2yqftgwcvdk.streamlit.app/)**

No installation needed. It runs in any modern browser (Chrome, Edge, Firefox).

---

## ✨ Features

| Concept | What you can do |
|---|---|
| **Stack** | Push, Pop, Reset. The `top` pointer moves with each operation. Fixed capacity of 8, with overflow and underflow messages. |
| **Queue** | Enqueue, Dequeue, Reset. Front and rear pointers are shown. Fixed capacity of 8. |
| **Circular Queue** | A ring of 8 slots. Pointers wrap around with `(index + 1) % size`, and freed slots get reused. |
| **Singly Linked List** | Insert at start, end or a given position. Delete from start, end or a given position. Search, with the traversing pointer highlighted. |
| **Doubly Linked List** | The same operations as the singly linked list. Both `prev` and `next` links are drawn and animated. |
| **Binary Search Tree** | Insert, search (the path from the root is highlighted), and preorder, inorder and postorder traversals. The layout is calculated automatically, and duplicate values are rejected. |

### Shared by every structure
- **Step-by-step animations**, each with a short explanation of what is happening.
- **Speed control**, plus **Play / Pause** and **Step Back / Step Forward** through the last operation.
- **History log**: a separate log for each structure that records the entry number, the operation, the value or position used, and the result. Failed operations are logged too.
- **Recent strip**: the last 5 operations, kept in a circular queue of size 5.
- **Reset** clears the structure and its log. **Clear Log** clears only the log.
- **Back navigation** goes through a stack. It works from both the Back button and the browser's back button.

### Example history log (Stack)
```
1. Push(10)   Success
2. Push(25)   Success
3. Pop()      Success (removed 25)
4. Pop()      Success (removed 10)
5. Pop()      Error: stack is empty
```

---

## 🧠 Data Structures Used Inside the App

Each structure is taught in its own section and is also used to run the app:

| Concept | Used inside the app for |
|---|---|
| Stack | Back navigation (`NavigationHistory`) |
| Queue | Playing animation steps in first-in, first-out order |
| Circular Queue | The "Recent" strip of the last 5 operations |
| Singly Linked List | Storing each structure's history log |
| Doubly Linked List | Moving back and forward through animation steps |
| Binary Search Tree | Building, searching and drawing the tree |
| Recursion | Traversals, BST insert and search, and node layout positions |
| Arrays | Fixed-capacity stack, queue and circular queue |
| Pointers and dynamic memory | Every node-based structure: nodes are created with `new` and freed in destructors |

> **No hash tables.** The project does not use `std::map`, `std::unordered_map`, `std::set` or any other hash or map container. Linked lists and the binary tree are written by hand with nodes and pointers.

---

## 🛠️ Tech Stack

- **Core logic:** C++. All data structures, navigation, step recording and history logging live here.
- **Allowed containers:** `std::vector`, `std::stack`, `std::queue`, `std::string`
- **Frontend:** HTML, CSS, JavaScript and SVG on a single page that never reloads
- **Deployment:** Streamlit Community Cloud

## 📁 Project Structure

```
StructViz/
├── ds/        # Stack, Queue, CircularQueue, LinkedList, DoublyLinkedList,
│              # BinarySearchTree, HistoryLog, NavigationHistory, StepRecorder (.h + .cpp)
├── wasm/      # bindings.cpp and Emscripten build script
├── web/       # index.html, style.css, script.js
└── tests/     # C++ tests for each structure (no graphics)
```

## ▶️ Running Locally

```bash
# 1. Clone the repository
git clone <your-repo-url>
cd StructViz

# 2. Install dependencies
pip install -r requirements.txt

# 3. Start the app
streamlit run app.py
```

Then open the local URL that Streamlit prints, usually `http://localhost:8501`.

---

## 🎯 Who It's For

- **Students** learning data structures in a C++ course
- **Teachers** who want a live demonstration tool for class

## 📄 License

This project was built for a Data Structures and Algorithms course.
