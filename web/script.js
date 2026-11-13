/* =============================================================================
   NodeCraft — script.js  (display and interaction only)

   Every data structure operation, navigation decision, log entry and
   animation step comes from the C++ module (nodecraft.js / nodecraft.wasm).
   JavaScript only reads state back from C++ and draws it.

   Sections:
     0. Helpers
     1. Theme
     2. Loading the backend
     3. Navigation (C++ NavigationHistory stack)
     4. Workspaces (controls, history panel, recent strip)
     5. Animation engine  run() / playback / step-through
     6. Renderers: stack, queue, circular queue, linked lists, tree
     7. Tooltips + boot
   ========================================================================== */
"use strict";

/* ---------- 0. Helpers ---------- */
const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));
const esc = (s) => String(s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)");
const BASE_DELAY = 700; // ms between steps at 1x speed

/** Copies an Embind vector into a plain array and frees the C++ copy. */
function vecToArray(vec) {
  const out = [];
  const n = vec.size();
  for (let i = 0; i < n; i++) out.push(vec.get(i));
  vec.delete();
  return out;
}

/** Restart a CSS animation class on an element. */
function retrigger(el, cls) {
  if (!el) return;
  el.classList.remove(cls);
  void el.getBoundingClientRect();
  el.classList.add(cls);
}

function shake(el) { retrigger(el, "fx-shake"); }

const SVG_NS = "http://www.w3.org/2000/svg";
function svgEl(tag, attrs) {
  const el = document.createElementNS(SVG_NS, tag);
  for (const k in attrs) el.setAttribute(k, attrs[k]);
  return el;
}

/* ---------- 1. Theme ---------- */
const THEME_KEY = "nodecraft-theme";
const prefersLight = window.matchMedia("(prefers-color-scheme: light)");

function effectiveTheme() {
  const forced = document.documentElement.dataset.theme;
  if (forced === "light" || forced === "dark") return forced;
  return prefersLight.matches ? "light" : "dark";
}

function updateThemeButton() {
  const next = effectiveTheme() === "dark" ? "light" : "dark";
  $("#themeBtn").setAttribute("aria-label", `Switch to ${next} theme`);
  $("#themeBtn").title = `Switch to ${next} theme`;
}

function initTheme() {
  // Without a saved choice, mirror the system theme so the icons stay right.
  if (!document.documentElement.dataset.theme) document.documentElement.dataset.theme = effectiveTheme();
  prefersLight.addEventListener("change", () => {
    let saved = null;
    try { saved = localStorage.getItem(THEME_KEY); } catch (e) { /* storage blocked */ }
    if (!saved) { document.documentElement.dataset.theme = prefersLight.matches ? "light" : "dark"; updateThemeButton(); }
  });
  $("#themeBtn").addEventListener("click", () => {
    const next = effectiveTheme() === "dark" ? "light" : "dark";
    document.documentElement.dataset.theme = next;
    try { localStorage.setItem(THEME_KEY, next); } catch (e) { /* storage blocked */ }
    updateThemeButton();
  });
  updateThemeButton();
}

/* ---------- 2. Loading the backend ---------- */
let M = null;     // the Emscripten module
let nav = null;   // C++ NavigationHistory
const WS = {};    // workspaces by key: stack, queue, cqueue, sll, dll, bst

function loaderFail(html) {
  const loader = $("#loader");
  loader.classList.add("failed");
  $("#loaderMsg").innerHTML = html;
}

async function boot() {
  initTheme();
  const serverHint = 'Open this page through a local server instead of double-clicking the file, for example run <code>python -m http.server</code> in the <code>web</code> folder and visit <code>http://localhost:8000</code>.';
  // The single-file build (Streamlit) carries the wasm inside the JS, so file:// is fine there.
  if (location.protocol === "file:" && !window.NODECRAFT_SINGLE_FILE) {
    loaderFail("NodeCraft can't load its C++ engine from a file opened directly. " + serverHint);
    return;
  }
  if (typeof createNodeCraft !== "function") {
    loaderFail("The C++ engine (<code>nodecraft.js</code>) was not found. Build it with <code>make wasm</code>, then reload. " + serverHint);
    return;
  }
  try {
    M = await createNodeCraft();
  } catch (err) {
    loaderFail("The C++ engine failed to start. " + serverHint);
    console.error(err);
    return;
  }

  nav = new M.NavigationHistory();
  const objects = {
    stack: new M.Stack(),
    queue: new M.Queue(),
    cqueue: new M.CircularQueue(),
    sll: new M.SinglyLinkedList(),
    dll: new M.DoublyLinkedList(),
    bst: new M.BinarySearchTree(),
  };
  for (const key in objects) createWorkspace(key, objects[key]);

  initNavigation();
  initTabs();
  initTooltips();
  show(nav.current());
  $("#loader").classList.add("done");
}

/* ---------- 3. Navigation (PRD 6.10) ---------- */
function show(name) {
  let target = document.getElementById(name);
  if (!target || !target.classList.contains("page")) target = $("#home");
  $$("section.page").forEach((s) => {
    if (s === target) {
      s.hidden = false;
      s.classList.remove("entering");
      void s.offsetWidth;
      s.classList.add("entering");
    } else {
      s.hidden = true;
    }
  });
  document.body.dataset.page = target.id;
  $("#backBtn").disabled = !nav.canGoBack();
  window.scrollTo(0, 0);
  const heading = $("h1", target);
  if (heading) heading.focus({ preventScroll: true });
  // Layout-dependent views need a redraw once visible.
  $$(".workspace", target).forEach((el) => { const w = WS[el.dataset.ws]; if (w && !el.closest("[hidden]")) w.onShow(); });
}

// Some embeds (e.g. the Streamlit iframe) refuse history.pushState. Then the
// Back button calls nav.back() directly instead of going through browser history.
let browserHistoryOK = true;

function openPage(name) {
  const before = nav.depth();
  nav.open(name);
  if (nav.depth() > before && browserHistoryOK) {
    try { history.pushState({}, ""); } catch (e) { browserHistoryOK = false; }
  }
  show(nav.current());
}

function goBack() {
  show(nav.back());
}

function initNavigation() {
  $$("[data-open]").forEach((card) => {
    card.addEventListener("click", () => openPage(card.dataset.open));
    card.addEventListener("keydown", (e) => {
      if (e.key === "Enter" || e.key === " ") { e.preventDefault(); openPage(card.dataset.open); }
    });
  });
  // The Back button goes through browser history so both stay in step;
  // the popstate handler then asks C++ which section to show.
  $("#backBtn").addEventListener("click", () => {
    if (!nav.canGoBack()) return;
    if (browserHistoryOK) history.back();
    else goBack();
  });
  window.addEventListener("popstate", goBack);
}

function initTabs() {
  $$(".tabs").forEach((list) => {
    const tabs = $$(".tab", list);
    const select = (tab) => {
      tabs.forEach((t) => {
        const on = t === tab;
        t.setAttribute("aria-selected", on);
        t.tabIndex = on ? 0 : -1;
        document.getElementById(t.getAttribute("aria-controls")).hidden = !on;
      });
      const w = WS[tab.dataset.tab];
      if (w) w.onShow();
    };
    tabs.forEach((tab, i) => {
      tab.addEventListener("click", () => select(tab));
      tab.addEventListener("keydown", (e) => {
        if (e.key !== "ArrowRight" && e.key !== "ArrowLeft") return;
        const next = tabs[(i + (e.key === "ArrowRight" ? 1 : tabs.length - 1)) % tabs.length];
        next.focus();
        select(next);
      });
    });
  });
}

/* ---------- 4. Workspaces ---------- */
const PLAY_ICON = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M7 4.5v15l12-7.5z" fill="currentColor"/></svg>';
const PAUSE_ICON = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M6.5 4.5h4v15h-4zM13.5 4.5h4v15h-4z" fill="currentColor"/></svg>';
const BACK_ICON = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M6 5h2.5v14H6zM20 5v14L9.5 12z" fill="currentColor"/></svg>';
const FWD_ICON = '<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M15.5 5H18v14h-2.5zM4 5v14l10.5-7z" fill="currentColor"/></svg>';

/* Per-structure configuration: inputs, buttons, snapshot and renderer. */
const CONFIG = {
  stack: {
    name: "stack",
    inputs: ["value"],
    ops: [
      { label: "Push", needs: ["value"], aria: "Push the value onto the stack", call: (o, v) => o.push(v), primary: true },
      { label: "Pop", aria: "Pop the top value", call: (o) => o.pop() },
    ],
    enterOp: 0,
    welcome: "Type a number and press Push (or Enter). Click the top box to pop it.",
    emptyHistory: "No operations yet. Try pushing a value!",
    snap: (o) => {
      const top = o.getTop();
      return { items: vecToArray(o.getItems()), top, full: o.isFull(), pointers: { top } };
    },
    renderer: () => StackView,
  },
  queue: {
    name: "queue",
    inputs: ["value"],
    ops: [
      { label: "Enqueue", needs: ["value"], aria: "Add the value at the rear", call: (o, v) => o.enqueue(v), primary: true },
      { label: "Dequeue", aria: "Remove the front value", call: (o) => o.dequeue() },
    ],
    enterOp: 0,
    welcome: "Enqueue a few values, then dequeue. Watch the freed slots on the left get wasted.",
    emptyHistory: "No operations yet. Try enqueuing a value!",
    snap: (o) => {
      const front = o.getFront(), rear = o.getRear();
      return { slots: vecToArray(o.getSlots()), front, rear, size: o.size(), full: o.isFull(), empty: o.isEmpty(), pointers: { front, rear } };
    },
    renderer: () => QueueView,
  },
  cqueue: {
    name: "circular queue",
    inputs: ["value"],
    ops: [
      { label: "Enqueue", needs: ["value"], aria: "Add the value at the rear", call: (o, v) => o.enqueue(v), primary: true },
      { label: "Dequeue", aria: "Remove the front value", call: (o) => o.dequeue() },
    ],
    enterOp: 0,
    welcome: "Fill the ring, dequeue a couple, then enqueue again to see freed slots reused.",
    emptyHistory: "No operations yet. Try enqueuing a value!",
    snap: (o) => {
      const front = o.getFront(), rear = o.getRear();
      return { slots: vecToArray(o.getSlots()), front, rear, size: o.size(), full: o.isFull(), empty: o.isEmpty(), pointers: { front, rear } };
    },
    renderer: () => RingView,
  },
  sll: listConfig(false),
  dll: listConfig(true),
  bst: {
    name: "tree",
    byValue: true,
    inputs: ["value"],
    ops: [
      { label: "Insert", needs: ["value"], aria: "Insert the value into the tree", call: (o, v) => o.insert(v), primary: true },
      { label: "Search", needs: ["value"], aria: "Search the tree for the value", call: (o, v) => o.search(v) },
      { label: "Delete", needs: ["value"], aria: "Delete the value from the tree", call: (o, v) => o.remove(v) },
      { label: "Preorder", aria: "Preorder traversal: root, left, right", call: (o) => o.preorder(), sep: true },
      { label: "Inorder", aria: "Inorder traversal: left, root, right", call: (o) => o.inorder() },
      { label: "Postorder", aria: "Postorder traversal: left, right, root", call: (o) => o.postorder() },
    ],
    enterOp: 0,
    welcome: "Insert values like 50, 30, 70, 20, 40. Click a node to search for it, or its × to delete it.",
    emptyHistory: "No operations yet. Try inserting a value!",
    snap: (o) => ({ nodes: vecToArray(o.getNodes()), pointers: {} }),
    renderer: () => TreeView,
  },
};

function listConfig(doubly) {
  return {
    name: doubly ? "doubly linked list" : "singly linked list",
    doubly,
    shiftPointers: true,
    inputs: ["value", "position"],
    ops: [
      { label: "Insert at Start", needs: ["value"], aria: "Insert the value at the start", call: (o, v) => o.insertAtStart(v), primary: true },
      { label: "Insert at End", needs: ["value"], aria: "Insert the value at the end", call: (o, v) => o.insertAtEnd(v), primary: true },
      { label: "Insert at Position", needs: ["value", "position"], aria: "Insert the value at the given position", call: (o, v, p) => o.insertAtPosition(p, v), primary: true },
      { label: "Delete Start", aria: "Delete the first node", call: (o) => o.deleteAtStart(), sep: true },
      { label: "Delete End", aria: "Delete the last node", call: (o) => o.deleteAtEnd() },
      { label: "Delete at Position", needs: ["position"], aria: "Delete the node at the given position", call: (o, v, p) => o.deleteAtPosition(p) },
      { label: "Search", needs: ["value"], aria: "Search the list for the value", call: (o, v) => o.search(v), sep: true },
    ],
    enterOp: 1,
    welcome: doubly
      ? "Insert a few values. Hover between nodes for +, hover a node for ×, click a value to search."
      : "Insert a few values. Hover between nodes for +, hover a node for ×, click a value to search.",
    emptyHistory: "No operations yet. Try inserting a value!",
    snap: (o) => {
      const values = vecToArray(o.getValues());
      const n = values.length;
      return { values, pointers: { head: n ? 0 : -1, tail: n ? n - 1 : -1 } };
    },
    renderer: () => ListView,
  };
}

function createWorkspace(key, obj) {
  const root = $(`.workspace[data-ws="${key}"]`);
  const cfg = CONFIG[key];
  const uid = `ws-${key}`;

  const inputHtml = cfg.inputs.map((name) => `
    <div class="field">
      <label for="${uid}-${name}">${name === "value" ? "Value" : "Position"}</label>
      <input id="${uid}-${name}" data-input="${name}" type="text" inputmode="numeric" autocomplete="off"
             placeholder="${name === "value" ? "e.g. 42" : "e.g. 2"}" aria-describedby="${uid}-hint">
    </div>`).join("");
  const opHtml = cfg.ops.map((op, i) => `
    ${op.sep ? '</div><div class="ctl-group ctl-ops">' : ""}
    <button type="button" class="btn ${op.primary ? "btn-primary" : "btn-ghost"}" data-op="${i}" aria-label="${esc(op.aria)}">${esc(op.label)}</button>`).join("");

  root.innerHTML = `
    <div class="ws-main">
      <div class="recent" aria-label="Last 5 operations">
        <span class="recent-label">Recent</span>
        <div class="recent-pills"></div>
      </div>
      <div class="canvas-wrap">
        <div class="blob" aria-hidden="true"></div>
        <div class="canvas" role="img" aria-label="Drawing of the ${esc(cfg.name)}"></div>
      </div>
      <div class="stepbar" aria-live="polite">
        <span class="stepbar-msg">${esc(cfg.welcome)}</span>
      </div>
      <div class="controls" role="group" aria-label="${esc(cfg.name)} controls">
        <div class="ctl-group ctl-inputs">${inputHtml}</div>
        <div class="ctl-group ctl-ops">${opHtml}</div>
        <div class="ctl-group ctl-admin">
          <button type="button" class="btn btn-danger" data-admin="reset" aria-label="Reset the ${esc(cfg.name)} and its log">Reset</button>
          <button type="button" class="btn btn-ghost" data-admin="clear" aria-label="Clear the history log only">Clear Log</button>
        </div>
        <div class="ctl-group ctl-play" role="group" aria-label="Animation playback">
          <button type="button" class="btn btn-ghost icon-btn" data-play="back" aria-label="Step back">${BACK_ICON}</button>
          <button type="button" class="btn btn-ghost icon-btn" data-play="toggle" aria-label="Play">${PLAY_ICON}</button>
          <button type="button" class="btn btn-ghost icon-btn" data-play="fwd" aria-label="Step forward">${FWD_ICON}</button>
          <label class="speed" for="${uid}-speed">Speed
            <input id="${uid}-speed" type="range" min="0.25" max="3" step="0.25" value="1" aria-label="Animation speed">
            <output for="${uid}-speed">1x</output>
          </label>
          <span class="counter" aria-live="off">No steps yet</span>
        </div>
        <p class="hint" id="${uid}-hint" role="alert"></p>
      </div>
    </div>
    <div class="ws-side">
      <section class="code-panel" aria-label="C++ code for the current operation">
        <div class="code-head">
          <span class="code-lang">C++</span>
          <span class="code-sig"></span>
          <button type="button" class="code-copy" aria-label="Copy this C++ code">Copy</button>
        </div>
        <pre class="code-body"><code></code></pre>
        <p class="code-note"></p>
      </section>
      <aside class="history" aria-label="History log">
        <div class="history-head"><h2>History</h2><span class="history-count">0</span></div>
        <ol class="history-list"></ol>
        <p class="history-empty">${esc(cfg.emptyHistory)}</p>
      </aside>
    </div>`;

  const w = {
    key, cfg, obj, root,
    canvas: $(".canvas", root),
    canvasWrap: $(".canvas-wrap", root),
    msg: $(".stepbar-msg", root),
    stepbar: $(".stepbar", root),
    hint: $(".hint", root),
    counter: $(".counter", root),
    inputs: {},
    before: null, after: null, steps: [], switchAt: 0, lastOp: "",
    playing: false, busy: false, timer: null, speed: 1,
    lastLogMax: 0, lastRecentMax: 0,
  };
  $$("[data-input]", root).forEach((el) => { w.inputs[el.dataset.input] = el; });
  WS[key] = w;

  // Renderer
  w.view = cfg.renderer();
  w.view.init(w);
  w.onShow = () => { if (w.view.onShow) w.view.onShow(w); };

  // Operation buttons
  $$("[data-op]", root).forEach((btn) => {
    btn.addEventListener("click", () => runOp(w, cfg.ops[+btn.dataset.op]));
  });
  // Enter in an input runs the main operation
  Object.values(w.inputs).forEach((input) => {
    input.addEventListener("keydown", (e) => {
      if (e.key === "Enter") { e.preventDefault(); runOp(w, cfg.ops[cfg.enterOp]); }
    });
    input.addEventListener("input", () => { input.classList.remove("invalid"); setHint(w, ""); });
  });

  // Reset (inline confirm) and Clear Log
  const resetBtn = $('[data-admin="reset"]', root);
  let confirmTimer = null;
  resetBtn.addEventListener("click", () => {
    if (!resetBtn.classList.contains("confirm")) {
      resetBtn.classList.add("confirm");
      resetBtn.textContent = "Sure? Click again";
      confirmTimer = setTimeout(() => { resetBtn.classList.remove("confirm"); resetBtn.textContent = "Reset"; }, 3000);
      return;
    }
    clearTimeout(confirmTimer);
    resetBtn.classList.remove("confirm");
    resetBtn.textContent = "Reset";
    run(w, () => w.obj.reset(), "reset");
  });
  $('[data-admin="clear"]', root).addEventListener("click", () => {
    w.obj.clearLog();
    refreshPanels(w);
    setMessage(w, "The history log was cleared. The structure itself is unchanged.", "info");
  });

  // Playback controls
  $('[data-play="toggle"]', root).addEventListener("click", () => togglePlay(w));
  $('[data-play="back"]', root).addEventListener("click", () => manualStep(w, -1));
  $('[data-play="fwd"]', root).addEventListener("click", () => manualStep(w, +1));
  const speed = $(`#${uid}-speed`);
  speed.addEventListener("input", () => {
    w.speed = parseFloat(speed.value);
    $("output", speed.parentElement).textContent = `${w.speed}x`;
  });

  // History hover: briefly highlight the element an entry refers to
  const list = $(".history-list", root);
  list.addEventListener("mouseover", (e) => {
    const row = e.target.closest(".row");
    if (row && row !== w.hoverRow) { w.hoverRow = row; flashReference(w, row.dataset.arg); }
  });
  list.addEventListener("mouseleave", () => { w.hoverRow = null; });

  // C++ code panel: show the main operation until the user runs something.
  w.code = { body: $(".code-body", root), sig: $(".code-sig", root), note: $(".code-note", root), label: null, lines: [] };
  $(".code-copy", root).addEventListener("click", (e) => copyCode(w, e.currentTarget));
  setCode(w, cfg.ops[0].label);

  // First draw from C++ state
  w.after = cfg.snap(obj);
  renderIdle(w);
  refreshPanels(w);
  updatePlayback(w);
}

/** Validates a number field: whole number in -999..999. Returns null if invalid. */
function readNumber(w, name) {
  const input = w.inputs[name];
  const raw = input.value.trim();
  const label = name === "value" ? "value" : "position";
  let problem = "";
  if (raw === "") problem = `Type a ${label} first.`;
  else if (!/^[+-]?\d+$/.test(raw)) problem = `The ${label} must be a whole number, like ${name === "value" ? "42" : "2"}.`;
  else if (Math.abs(parseInt(raw, 10)) > 999) problem = `Pick a ${label} between -999 and 999.`;
  if (problem) {
    input.classList.add("invalid");
    setHint(w, problem);
    input.focus();
    return null;
  }
  return parseInt(raw, 10);
}

function setHint(w, text, info) {
  w.hint.textContent = text;
  w.hint.classList.toggle("info", !!info);
}

function runOp(w, op) {
  if (w.busy) return;
  setHint(w, "");
  const needs = op.needs || [];
  let v = 0, p = 0;
  for (const n of needs) {
    const num = readNumber(w, n);
    if (num === null) return;
    if (n === "value") v = num; else p = num;
  }
  run(w, () => op.call(w.obj, v, p), op.label);
}

/* History panel + Recent strip, always re-read from C++ */
function refreshPanels(w) {
  const log = vecToArray(w.obj.getLog());
  const list = $(".history-list", w.root);
  const maxBefore = w.lastLogMax;
  list.innerHTML = log.map((e) => {
    const call = `<b>${esc(e.function)}</b>(${esc(e.argument)})`;
    const fresh = e.number > maxBefore ? " row-new" : "";
    return `<li class="row ${e.success ? "ok" : "err"}${fresh}" data-arg="${esc(e.argument)}">
      <span class="row-num">${e.number}.</span>
      <span class="row-call">${call}</span>
      <span class="row-badge">${esc(e.result)}</span></li>`;
  }).join("");
  w.lastLogMax = log.length ? log[log.length - 1].number : 0;
  $(".history-count", w.root).textContent = log.length;
  $(".history-empty", w.root).hidden = log.length > 0;
  list.scrollTop = list.scrollHeight;

  const recent = vecToArray(w.obj.getRecent());
  const pills = $(".recent-pills", w.root);
  if (!recent.length) {
    pills.innerHTML = '<span class="recent-empty">Nothing yet</span>';
  } else {
    const newestNum = recent[recent.length - 1].number;
    pills.innerHTML = recent.map((e, i) => {
      const newest = i === recent.length - 1;
      const slide = newest && newestNum !== w.lastRecentMax ? " slide-in" : "";
      const arg = e.argument ? `(${e.argument})` : "()";
      return `<span class="pill ${e.success ? "ok" : "err"}${newest ? " newest" : ""}${slide}" data-tip="${esc(e.number + ". " + e.function + arg + " → " + e.result)}">${esc(e.function + arg)}</span>`;
    }).join("");
    w.lastRecentMax = newestNum;
  }
}

/** Highlights the element a log entry refers to ("pos 3" -> position, "42" -> value). */
function flashReference(w, arg) {
  if (!arg || w.busy) return;
  let el = null;
  const pos = /pos (-?\d+)/.exec(arg);
  if (pos) el = $(`[data-el][data-index="${parseInt(pos[1], 10) - 1}"]`, w.canvas);
  else if (/^-?\d+$/.test(arg)) el = $(`[data-el][data-value="${arg}"]`, w.canvas);
  if (!el) return;
  retrigger(el, "fx-ref");
  clearTimeout(el._refTimer);
  el._refTimer = setTimeout(() => el.classList.remove("fx-ref"), 900);
}

/* ---------- 5. Animation engine (AN-1 .. AN-6) ---------- */

/**
 * Runs one C++ operation and plays its steps.
 *   1. snapshot BEFORE   2. call C++   3. snapshot AFTER   4. play steps
 */
function run(w, fn, label) {
  if (w.busy) return null;
  stopTimer(w);
  const before = w.cfg.snap(w.obj);
  let result;
  try {
    result = fn();
  } catch (err) {
    console.error(err);
    setMessage(w, "Something went wrong while talking to the C++ engine.", "error");
    return null;
  }
  const steps = vecToArray(result.steps);
  w.before = before;
  w.after = w.cfg.snap(w.obj);
  w.steps = steps;
  w.switchAt = findSwitch(steps);
  w.result = { success: result.success, message: result.message, entry: result.entry };
  w.lastOp = label || "";
  setCode(w, w.lastOp);
  if (w.view.onOperation) w.view.onOperation(w);

  setBusy(w, true);
  w.playing = true;
  updatePlayback(w);
  tick(w);
  return w.result;
}

/**
 * The step where drawing switches from the BEFORE to the AFTER snapshot:
 * the first "insert" (drawn on AFTER), or the step right after the first
 * "remove" (the remove itself shrinks the element on BEFORE).
 */
function findSwitch(steps) {
  for (let i = 0; i < steps.length; i++) {
    if (steps[i].action === "insert") return i;
    if (steps[i].action === "remove") return i + 1;
  }
  return steps.length;
}

function delay(w) { return BASE_DELAY / w.speed; }

function tick(w) {
  if (!w.playing) return;
  if (!w.obj.hasNextStep()) { finishPlayback(w); return; }
  w.obj.stepNext();
  renderStep(w);
  w.timer = setTimeout(() => tick(w), delay(w));
}

function stopTimer(w) {
  clearTimeout(w.timer);
  w.timer = null;
}

function finishPlayback(w) {
  stopTimer(w);
  w.playing = false;
  setBusy(w, false);
  renderIdle(w, true);
  refreshPanels(w);
  updatePlayback(w);
}

function togglePlay(w) {
  if (w.playing) {
    // Pause: keep the current step on screen, allow manual stepping.
    stopTimer(w);
    w.playing = false;
    setBusy(w, false);
    refreshPanels(w);
    updatePlayback(w);
    return;
  }
  if (!w.obj.stepCount()) return;
  // Play again from the start when we are already at the last step.
  if (!w.obj.hasNextStep()) {
    while (w.obj.hasPrevStep()) w.obj.stepPrev();
    renderStep(w);
    setBusy(w, true);
    w.playing = true;
    updatePlayback(w);
    w.timer = setTimeout(() => tick(w), delay(w));
    return;
  }
  setBusy(w, true);
  w.playing = true;
  updatePlayback(w);
  tick(w);
}

function manualStep(w, dir) {
  if (w.playing) { stopTimer(w); w.playing = false; setBusy(w, false); refreshPanels(w); }
  if (dir > 0 && w.obj.hasNextStep()) w.obj.stepNext();
  else if (dir < 0 && w.obj.hasPrevStep()) w.obj.stepPrev();
  else { updatePlayback(w); return; }
  renderStep(w);
  updatePlayback(w);
}

function setBusy(w, busy) {
  w.busy = busy;
  w.root.classList.toggle("busy", busy);
  $$("[data-op], [data-admin], [data-input]", w.root).forEach((el) => { el.disabled = busy; });
  if (busy && w.view.onBusy) w.view.onBusy(w);
}

function updatePlayback(w) {
  const count = w.obj.stepCount();
  const pos = w.obj.stepPosition();
  const toggle = $('[data-play="toggle"]', w.root);
  toggle.innerHTML = w.playing ? PAUSE_ICON : PLAY_ICON;
  toggle.setAttribute("aria-label", w.playing ? "Pause" : (pos >= count && count ? "Replay" : "Play"));
  toggle.disabled = count === 0;
  $('[data-play="back"]', w.root).disabled = !w.obj.hasPrevStep();
  $('[data-play="fwd"]', w.root).disabled = !w.obj.hasNextStep();
  w.counter.textContent = count ? `Step ${pos} of ${count}` : "No steps yet";
}

/** Pointer positions at step p: BEFORE pointers + every movePointer up to p. */
function replayPointers(w, p) {
  const ptrs = Object.assign({}, w.before.pointers);
  for (let i = 0; i < p; i++) {
    const s = w.steps[i];
    // Linked lists: when the drawing switches to the AFTER layout, nodes after an
    // inserted node move one place right (or left after a remove), and so do the
    // pointers that were sitting on them.
    if (w.cfg.shiftPointers && i === w.switchAt) {
      const removed = i > 0 && w.steps[i - 1].action === "remove" ? w.steps[i - 1].index : null;
      for (const k in ptrs) {
        if (s.action === "insert" && ptrs[k] >= s.index) ptrs[k] += 1;
        else if (removed !== null && ptrs[k] > removed) ptrs[k] -= 1;
      }
    }
    if (s.action === "movePointer" && s.pointer) ptrs[s.pointer] = s.index;
  }
  return ptrs;
}

/** Draw the current timeline step (used by auto-play and manual stepping). */
function renderStep(w) {
  const p = w.obj.stepPosition();
  if (p < 1) return;
  const step = w.obj.currentStep();
  const phase = p - 1 >= w.switchAt ? "after" : "before";
  const ctx = { step, p, phase, steps: w.steps, switchAt: w.switchAt, pointers: replayPointers(w, p) };
  clearEffects(w);
  w.view.draw(w, phase === "after" ? w.after : w.before, ctx);
  applyEffects(w, ctx);
  const tone = step.action === "error" || step.action === "notFound" ? "error" : step.action === "found" ? "success" : "";
  setMessage(w, step.message, tone);
  highlightCode(w, p);
  updatePlayback(w);
}

/** Draw the resting AFTER state, keeping the operation's result message. */
function renderIdle(w, afterOp) {
  clearEffects(w);
  w.view.draw(w, w.after, null);
  if (afterOp && w.result) {
    setMessage(w, w.result.message, w.result.success ? "success" : "error");
  }
}

function setMessage(w, text, tone) {
  w.msg.textContent = text || "";
  retrigger(w.msg, "swap");
  w.stepbar.classList.toggle("is-error", tone === "error");
  w.stepbar.classList.toggle("is-success", tone === "success");
}

function clearEffects(w) {
  $$('[class*="fx-"]', w.canvasWrap).forEach((el) => {
    el.classList.forEach((c) => { if (c.startsWith("fx-") && c !== "fx-ref") el.classList.remove(c); });
  });
  w.canvasWrap.classList.remove("fx-shake");
}

function effectTarget(w, step) {
  if (w.cfg.byValue) return $(`[data-el][data-value="${step.value}"]`, w.canvas);
  if (step.index < 0) return null;
  return $(`[data-el][data-index="${step.index}"]`, w.canvas);
}

/** One visual effect per step action. */
function applyEffects(w, ctx) {
  const s = ctx.step;
  const el = effectTarget(w, s);
  // Force a reflow so re-used elements replay their animation.
  if (el) void el.getBoundingClientRect();
  switch (s.action) {
    case "highlight": if (el) el.classList.add("fx-highlight"); break;
    case "insert": if (el) el.classList.add("fx-insert"); break;
    case "remove": if (el) el.classList.add("fx-remove"); break;
    case "found": if (el) el.classList.add("fx-found"); break;
    case "visit": if (el) el.classList.add("fx-visit"); break;
    case "notFound": shake(w.canvasWrap); break;
    case "error": {
      const target = (w.view.errorTarget && w.view.errorTarget(w, s)) || el;
      if (target) retrigger(target, "fx-error"); else shake(w.canvasWrap);
      break;
    }
    case "movePointer":
      if (s.pointer === "cur" && el) el.classList.add("fx-cur");
      break;
    default: break; // link / unlink are drawn by the list renderer
  }
  if (w.view.afterEffects) w.view.afterEffects(w, ctx);
}

/* ---------- 5b. C++ code panel ----------
   Each operation has a simplified version of its C++ from ds/*.cpp (the
   step-recording calls are left out so the algorithm is easy to read).
   A line can end with "##tags": the animation steps that light that line up.
     tag   = action[:pointer][ /regex on step.message/]
     tags  = tag;tag;...
   While a step plays, every line whose tag matches it is highlighted. If no
   line matches, the highlight stays on the last step that did match. */

function cpp(sig, file, src) {
  const lines = src.replace(/^\n+|\n+$/g, "").split("\n").map((raw) => {
    const at = raw.indexOf("##");
    const text = (at >= 0 ? raw.slice(0, at) : raw).replace(/\s+$/, "");
    const tags = at < 0 ? [] : raw.slice(at + 2).split(";").map((t) => t.trim()).filter(Boolean).map((t) => {
      const m = /^(\w+)(?::(\w+))?(?:\s+\/(.*)\/)?$/.exec(t);
      return m ? { action: m[1], pointer: m[2] || "", re: m[3] ? new RegExp(m[3]) : null } : null;
    }).filter(Boolean);
    return { text, tags };
  });
  return { sig, file, lines };
}

function listSearch(cls) {
  return cpp(`${cls}::search(int value)`, cls === "SinglyLinkedList" ? "ds/LinkedList.cpp" : "ds/DoublyLinkedList.cpp", String.raw`
OpResult ${cls}::search(int value) {
    Node* cur = head;
    int pos = 1;
    while (cur != nullptr) {
        if (cur->value == value)          ##movePointer:cur / = -?\d+$/
            return found(pos);            ##found
        cur = cur->next;   // keep walking  ##movePointer:cur /is not/
        pos = pos + 1;
    }
    return notFound();  // cur reached NULL ##notFound
}`);
}

const CODE = {
  stack: {
    Push: cpp("Stack::push(int value)", "ds/Stack.cpp", String.raw`
OpResult Stack::push(int value) {
    if (top == CAPACITY - 1) {           ##error
        return fail("Stack overflow");   ##error
    }
    items[top + 1] = value;  // above top  ##insert
    top = top + 1;           // top moves up ##movePointer:top
    return success();
}`),
    Pop: cpp("Stack::pop()", "ds/Stack.cpp", String.raw`
OpResult Stack::pop() {
    if (top == -1) {                      ##error
        return fail("Stack underflow");   ##error
    }
    int value = items[top];  // only the top ##highlight
    top = top - 1;           // top moves down ##remove;movePointer:top
    return success(value);
}`),
  },
  queue: {
    Enqueue: cpp("Queue::enqueue(int value)", "ds/Queue.cpp", String.raw`
OpResult Queue::enqueue(int value) {
    // rear only moves right, it never wraps
    if (rear == CAPACITY - 1) {          ##error
        return fail("Queue is full");    ##error
    }
    items[rear + 1] = value;             ##insert
    rear = rear + 1;                     ##movePointer:rear
    // front stays put (slot 0 at first)  ##movePointer:front
    return success();
}`),
    Dequeue: cpp("Queue::dequeue()", "ds/Queue.cpp", String.raw`
OpResult Queue::dequeue() {
    if (rear < front) {    // nothing left ##error
        return fail("Queue is empty");   ##error
    }
    int value = items[front];            ##highlight;remove
    front = front + 1;  // old slot wasted ##movePointer:front
    return success(value);
}`),
  },
  cqueue: {
    Enqueue: cpp("CircularQueue::enqueue(int value)", "ds/CircularQueue.cpp", String.raw`
OpResult CircularQueue::enqueue(int value) {
    if (count == CAPACITY) {             ##error
        return fail("Queue is full");    ##error
    }
    rear = (rear + 1) % CAPACITY; // 7 -> 0 ##movePointer:rear
    items[rear] = value;                 ##insert
    count = count + 1;
    // first item? front already points here ##movePointer:front
    return success();
}`),
    Dequeue: cpp("CircularQueue::dequeue()", "ds/CircularQueue.cpp", String.raw`
OpResult CircularQueue::dequeue() {
    if (count == 0) {                    ##error
        return fail("Queue is empty");   ##error
    }
    int value = items[front];            ##highlight
    count = count - 1;  // slot is free again ##remove
    front = (front + 1) % CAPACITY; // 7 -> 0 ##movePointer:front
    return success(value);
}`),
  },
  sll: {
    "Insert at Start": cpp("SinglyLinkedList::insertAtStart(int value)", "ds/LinkedList.cpp", String.raw`
OpResult SinglyLinkedList::insertAtStart(int value) {
    Node* node = new Node;               ##insert
    node->value = value;                 ##insert
    node->next = head;  // 1) old head   ##link:next
    head = node;        // 2) move head  ##movePointer:head
    size = size + 1;
    return success();
}`),
    "Insert at End": cpp("SinglyLinkedList::insertAtEnd(int value)", "ds/LinkedList.cpp", String.raw`
OpResult SinglyLinkedList::insertAtEnd(int value) {
    if (head == nullptr)       // empty list ##insert /holding -?\d+$/;link:next /was empty/
        return insertAtStart(value);     ##movePointer:head
    Node* cur = head;                    ##movePointer:cur /starts/
    while (cur->next != nullptr)         ##highlight:cur
        cur = cur->next;                 ##movePointer:cur /Follow/
    Node* node = new Node{value, nullptr}; ##insert /NULL\)$/
    cur->next = node;                    ##link:next /last node/
    size = size + 1;
    return success();
}`),
    "Insert at Position": cpp("SinglyLinkedList::insertAtPosition(int pos, int value)", "ds/LinkedList.cpp", String.raw`
OpResult SinglyLinkedList::insertAtPosition(int pos, int value) {
    if (pos < 1 || pos > size + 1)       ##error
        return fail("Invalid position"); ##error
    if (pos == 1)                        ##link:next /old head|was empty/
        return insertAtStart(value);     ##movePointer:head
    Node* cur = head;                    ##movePointer:cur /starts/
    for (int i = 1; i < pos - 1; i++)    ##highlight:cur
        cur = cur->next;                 ##movePointer:cur /Follow/
    Node* node = new Node{value, nullptr}; ##insert
    node->next = cur->next; // 1) new node first ##link:next /First/
    cur->next = node;       // 2) then cur  ##link:next /Then/
    size = size + 1;
    return success();
}`),
    "Delete Start": cpp("SinglyLinkedList::deleteAtStart()", "ds/LinkedList.cpp", String.raw`
OpResult SinglyLinkedList::deleteAtStart() {
    if (head == nullptr)                 ##error
        return fail("The list is empty"); ##error
    Node* old = head;                    ##highlight:head
    head = head->next;  // head moves on ##movePointer:head
    old->next = nullptr;                 ##unlink:next
    int value = old->value;
    delete old;         // free memory   ##remove
    size = size - 1;
    return success(value);
}`),
    "Delete End": cpp("SinglyLinkedList::deleteAtEnd()", "ds/LinkedList.cpp", String.raw`
OpResult SinglyLinkedList::deleteAtEnd() {
    if (head == nullptr)                 ##error
        return fail("The list is empty"); ##error
    if (head->next == nullptr)  // one node ##highlight:head;movePointer:head
        return deleteAtStart();          ##unlink:next /old head/;remove /old node/
    Node* cur = head;                    ##movePointer:cur /starts/
    while (cur->next->next != nullptr)   ##highlight:cur
        cur = cur->next;                 ##movePointer:cur /Follow/
    Node* last = cur->next;              ##highlight /^The last node/
    cur->next = nullptr;  // new last node ##unlink:next /cur\.next/
    delete last;                         ##remove /old last/
    size = size - 1;
    return success();
}`),
    "Delete at Position": cpp("SinglyLinkedList::deleteAtPosition(int pos)", "ds/LinkedList.cpp", String.raw`
OpResult SinglyLinkedList::deleteAtPosition(int pos) {
    if (head == nullptr || pos < 1 || pos > size) ##error
        return fail("Invalid position"); ##error
    if (pos == 1)                        ##highlight:head;movePointer:head;unlink:next
        return deleteAtStart();          ##remove /old node/
    Node* cur = head;                    ##movePointer:cur /starts/
    for (int i = 1; i < pos - 1; i++)    ##highlight:cur
        cur = cur->next;                 ##movePointer:cur /Follow/
    Node* target = cur->next;            ##highlight /^The node at position/
    cur->next = target->next; // skip it ##link:next
    delete target;                       ##remove /Delete the node/
    size = size - 1;
    return success();
}`),
    Search: listSearch("SinglyLinkedList"),
  },
  dll: {
    "Insert at Start": cpp("DoublyLinkedList::insertAtStart(int value)", "ds/DoublyLinkedList.cpp", String.raw`
OpResult DoublyLinkedList::insertAtStart(int value) {
    Node* node = new Node{value, nullptr, head}; ##insert;link:next
    if (head != nullptr)
        head->prev = node;  // old head points back ##link:prev
    else
        tail = node;        // only node: also tail ##movePointer:tail
    head = node;                         ##movePointer:head
    size = size + 1;
    return success();
}`),
    "Insert at End": cpp("DoublyLinkedList::insertAtEnd(int value)", "ds/DoublyLinkedList.cpp", String.raw`
OpResult DoublyLinkedList::insertAtEnd(int value) {
    if (tail == nullptr)        // empty list ##insert /prev is NULL\)$/;link:next /was empty/
        return insertAtStart(value);     ##movePointer:head;movePointer:tail /only node/
    // no walking needed: tail is the last node ##highlight:tail
    Node* node = new Node{value, tail, nullptr}; ##insert /next is NULL\)$/;link:prev
    tail->next = node;                   ##link:next
    tail = node;                         ##movePointer:tail /moves to/
    size = size + 1;
    return success();
}`),
    "Insert at Position": cpp("DoublyLinkedList::insertAtPosition(int pos, int value)", "ds/DoublyLinkedList.cpp", String.raw`
OpResult DoublyLinkedList::insertAtPosition(int pos, int value) {
    if (pos < 1 || pos > size + 1)       ##error
        return fail("Invalid position"); ##error
    if (pos == 1) return insertAtStart(value); ##insert /prev is NULL\)$/;link:next /old head|was empty/;link:prev /old head/;movePointer:head
    if (pos == size + 1) return insertAtEnd(value); ##highlight:tail;insert /next is NULL\)$/;link:prev /old tail/;link:next /old tail/;movePointer:tail /moves to/
    Node* cur = head;                    ##movePointer:cur /starts/
    for (int i = 1; i < pos - 1; i++)    ##highlight:cur
        cur = cur->next;                 ##movePointer:cur /Follow/
    Node* after = cur->next;
    Node* node = new Node{value, nullptr, nullptr}; ##insert /holding -?\d+$/
    // middle: four links change, in this order
    node->next = after;  // 1)           ##link:next /^1\)/
    node->prev = cur;    // 2)           ##link:prev /^2\)/
    after->prev = node;  // 3)           ##link:prev /^3\)/
    cur->next = node;    // 4)           ##link:next /^4\)/
    size = size + 1;
    return success();
}`),
    "Delete Start": cpp("DoublyLinkedList::deleteAtStart()", "ds/DoublyLinkedList.cpp", String.raw`
OpResult DoublyLinkedList::deleteAtStart() {
    if (head == nullptr)                 ##error
        return fail("The list is empty"); ##error
    Node* old = head;                    ##highlight:head
    head = head->next;                   ##movePointer:head
    if (head != nullptr)
        head->prev = nullptr; // cut the link back ##unlink:prev
    else
        tail = nullptr;       // list is now empty ##movePointer:tail
    int value = old->value;
    delete old;                          ##remove
    size = size - 1;
    return success(value);
}`),
    "Delete End": cpp("DoublyLinkedList::deleteAtEnd()", "ds/DoublyLinkedList.cpp", String.raw`
OpResult DoublyLinkedList::deleteAtEnd() {
    if (head == nullptr)                 ##error
        return fail("The list is empty"); ##error
    if (head == tail) return deleteAtStart(); ##highlight:head;movePointer:head;movePointer:tail /NULL too/;remove /old node/
    Node* old = tail;     // no walking! ##highlight:tail
    tail = tail->prev;                   ##movePointer:tail
    tail->next = nullptr;                ##unlink:next
    int value = old->value;
    delete old;                          ##remove /old last/
    size = size - 1;
    return success(value);
}`),
    "Delete at Position": cpp("DoublyLinkedList::deleteAtPosition(int pos)", "ds/DoublyLinkedList.cpp", String.raw`
OpResult DoublyLinkedList::deleteAtPosition(int pos) {
    if (head == nullptr || pos < 1 || pos > size) ##error
        return fail("Invalid position"); ##error
    if (pos == 1) return deleteAtStart(); ##highlight:head;movePointer:head;unlink:prev;remove /old node/
    if (pos == size) return deleteAtEnd(); ##highlight:tail;movePointer:tail;unlink:next;remove /old last/
    Node* cur = head;                    ##movePointer:cur /starts/
    for (int i = 1; i < pos; i++)        ##highlight:cur
        cur = cur->next;                 ##movePointer:cur /Follow/
    cur->prev->next = cur->next;  // 1)  ##link:next
    cur->next->prev = cur->prev;  // 2)  ##link:prev
    int value = cur->value;
    delete cur;                          ##remove /Delete the node/
    size = size - 1;
    return success(value);
}`),
    Search: listSearch("DoublyLinkedList"),
  },
  bst: {
    Insert: cpp("BinarySearchTree::insertRec(Node*& node, int value)", "ds/BinarySearchTree.cpp", String.raw`
// insert(value) calls insertRec(root, value)
bool BinarySearchTree::insertRec(Node*& node, int value) {
    if (node == nullptr) {     // empty spot ##insert
        node = new Node{value};          ##insert
        return true;
    }
    if (value == node->value)            ##error
        return false;  // duplicate: no change ##error
    if (value < node->value)             ##highlight /go left/
        return insertRec(node->left, value); ##highlight /go left/
    return insertRec(node->right, value); ##highlight /go right/
}`),
    Search: cpp("BinarySearchTree::searchRec(Node* node, int value)", "ds/BinarySearchTree.cpp", String.raw`
// search(value) calls searchRec(root, value)
bool BinarySearchTree::searchRec(Node* node, int value) {
    if (node == nullptr)                 ##notFound
        return false;      // not in tree ##notFound
    if (value == node->value)            ##found
        return true;                     ##found
    if (value < node->value)             ##highlight /go left/
        return searchRec(node->left, value); ##highlight /go left/
    return searchRec(node->right, value); ##highlight /go right/
}`),
    Delete: cpp("BinarySearchTree::removeRec(Node*& node, int value)", "ds/BinarySearchTree.cpp", String.raw`
// remove(value) calls removeRec(root, value)
bool BinarySearchTree::removeRec(Node*& node, int value) {
    if (node == nullptr) return false;   ##notFound
    if (value < node->value)             ##highlight /go left/
        return removeRec(node->left, value); ##highlight /go left/
    if (value > node->value)             ##highlight /go right/
        return removeRec(node->right, value); ##highlight /go right/
    // Case 1: a leaf -> just delete it
    if (node->left == nullptr && node->right == nullptr) { ##highlight /leaf/
        delete node;  node = nullptr;    ##remove /^Case 1/
        return true;
    }
    // Case 2: one child -> child takes its place
    if (node->left == nullptr || node->right == nullptr) { ##highlight /one child/
        Node* child = node->left ? node->left : node->right;
        delete node;  node = child;      ##remove /^Case 2/
        return true;
    }
    // Case 3: two children -> use the inorder successor ##highlight /two children/
    Node*& succ = minLink(node->right);  ##highlight /step right/
    node->value = succ->value;   // copy it ##remove /^Case 3/
    Node* old = succ;
    succ = succ->right;          // unlink old node ##remove /^Case 3/
    delete old;
    return true;
}

// The pointer to the smallest node: keep going left.
Node*& BinarySearchTree::minLink(Node*& node) {
    if (node->left == nullptr) return node; ##found
    return minLink(node->left);          ##highlight /go left to/
}`),
    Preorder: cpp("BinarySearchTree::preorderRec(Node* node)", "ds/BinarySearchTree.cpp", String.raw`
OpResult BinarySearchTree::preorder() {
    if (root == nullptr)                 ##error
        return fail("The tree is empty"); ##error
    preorderRec(root);  // builds the output
    return success(output);
}

void BinarySearchTree::preorderRec(Node* node) {
    if (node == nullptr) return;  // base case
    visit(node);                  // Root  ##visit
    preorderRec(node->left);      // Left
    preorderRec(node->right);     // Right
}`),
    Inorder: cpp("BinarySearchTree::inorderRec(Node* node)", "ds/BinarySearchTree.cpp", String.raw`
OpResult BinarySearchTree::inorder() {
    if (root == nullptr)                 ##error
        return fail("The tree is empty"); ##error
    inorderRec(root);   // values come out sorted
    return success(output);
}

void BinarySearchTree::inorderRec(Node* node) {
    if (node == nullptr) return;  // base case
    inorderRec(node->left);       // Left
    visit(node);                  // Root  ##visit
    inorderRec(node->right);      // Right
}`),
    Postorder: cpp("BinarySearchTree::postorderRec(Node* node)", "ds/BinarySearchTree.cpp", String.raw`
OpResult BinarySearchTree::postorder() {
    if (root == nullptr)                 ##error
        return fail("The tree is empty"); ##error
    postorderRec(root);  // builds the output
    return success(output);
}

void BinarySearchTree::postorderRec(Node* node) {
    if (node == nullptr) return;  // base case
    postorderRec(node->left);     // Left
    postorderRec(node->right);    // Right
    visit(node);                  // Root  ##visit
}`),
  },
};

const TOKEN_RE = /(\/\/.*$)|("(?:[^"\\]|\\.)*")|\b(if|else|while|for|return|new|delete|nullptr|true|false|const)\b|\b(int|bool|void|[A-Z]\w*)\b|\b(\d+)\b|\b([a-z_]\w*)(?=\()/g;
const TOKEN_CLASSES = ["tk-com", "tk-str", "tk-kw", "tk-type", "tk-num", "tk-fn"];

/** Tiny C++ syntax colouring for one line of code. */
function highlightCpp(line) {
  let out = "", last = 0, m;
  TOKEN_RE.lastIndex = 0;
  while ((m = TOKEN_RE.exec(line))) {
    out += esc(line.slice(last, m.index));
    const group = m.slice(1).findIndex((g) => g !== undefined);
    out += `<span class="${TOKEN_CLASSES[group]}">${esc(m[0])}</span>`;
    last = m.index + m[0].length;
  }
  return out + esc(line.slice(last));
}

/** Show the code for an operation label ("Push", "Insert at End", ...). */
function setCode(w, label) {
  const snip = (CODE[w.key] || {})[label];
  if (!snip) { clearCodeHighlight(w); return; }   // e.g. Reset: keep the last code
  if (w.code.label === label) { clearCodeHighlight(w); return; }
  w.code.label = label;
  w.code.lines = snip.lines;
  w.code.sig.textContent = snip.sig;
  w.code.note.textContent = `Simplified from ${snip.file}. The calls that record animation steps are left out.`;
  $("code", w.code.body).innerHTML = snip.lines.map((l, i) =>
    `<span class="cl"><span class="ln" aria-hidden="true">${i + 1}</span><span class="lc">${highlightCpp(l.text) || " "}</span></span>`).join("");
  w.code.body.scrollTop = 0;
  retrigger(w.code.body, "swap");
}

function clearCodeHighlight(w) {
  $$(".cl.is-active", w.code.body).forEach((el) => el.classList.remove("is-active"));
}

function stepMatchesTag(tag, s) {
  if (tag.action !== s.action) return false;
  if (tag.pointer && tag.pointer !== s.pointer) return false;
  if (tag.re && !tag.re.test(s.message)) return false;
  return true;
}

/** Light up the line(s) for step p (or the latest earlier step that has a line). */
function highlightCode(w, p) {
  const lines = w.code.lines;
  if (!lines.length || !w.code.label || w.code.label !== w.lastOp) return;
  let hit = [];
  for (let q = p; q >= 1 && hit.length === 0; q--) {
    const s = w.steps[q - 1];
    lines.forEach((l, i) => { if (l.tags.some((t) => stepMatchesTag(t, s))) hit.push(i); });
  }
  const els = $$(".cl", w.code.body);
  els.forEach((el, i) => el.classList.toggle("is-active", hit.includes(i)));
  if (hit.length) {
    // Scroll inside the code box only (never the page).
    const first = els[hit[0]], body = w.code.body;
    const top = first.offsetTop - body.clientHeight / 3;
    if (first.offsetTop < body.scrollTop || first.offsetTop + first.offsetHeight > body.scrollTop + body.clientHeight) {
      body.scrollTo({ top: Math.max(0, top), behavior: reducedMotion.matches ? "auto" : "smooth" });
    }
  }
}

async function copyCode(w, btn) {
  const text = w.code.lines.map((l) => l.text).join("\n");
  try {
    await navigator.clipboard.writeText(text);
    btn.textContent = "Copied";
  } catch (e) {
    btn.textContent = "Copy failed";
  }
  setTimeout(() => { btn.textContent = "Copy"; }, 1300);
}

/* ---------- 6a. Stack renderer ---------- */
const StackView = {
  init(w) {
    const slots = [];
    for (let i = 0; i < 8; i++) {
      slots.push(`<div class="slot" data-slot="${i}"><span class="slot-idx" aria-hidden="true">${i}</span><div class="slot-inner"></div></div>`);
    }
    w.canvas.innerHTML = `
      <div class="stack-view">
        <div class="stack-col">${slots.join("")}</div>
        <div class="ptr ptr-top" aria-hidden="true">◀ top</div>
      </div>
      <p class="canvas-caption"></p>`;
    w.col = $(".stack-col", w.canvas);
    w.ptrTop = $(".ptr-top", w.canvas);
    w.caption = $(".canvas-caption", w.canvas);
    w.canvas.addEventListener("click", (e) => {
      const box = e.target.closest(".box");
      if (!box || w.busy) return;
      if (box.classList.contains("is-top")) {
        run(w, () => w.obj.pop(), "Pop");
      } else {
        setMessage(w, "Only the top can be removed, that's the LIFO rule", "error");
        retrigger($(".box.is-top", w.canvas), "fx-wiggle");
      }
    });
  },
  draw(w, snap, ctx) {
    const top = ctx ? ctx.pointers.top : snap.top;
    $$(".slot", w.col).forEach((slot, i) => {
      const inner = $(".slot-inner", slot);
      if (i < snap.items.length) {
        const v = snap.items[i];
        const isTop = i === snap.items.length - 1;
        inner.innerHTML = `<button type="button" class="box${isTop ? " is-top" : ""}" data-el data-act data-index="${i}" data-value="${v}"
          data-tip="${esc(`Value ${v} · slot ${i}${i === top ? " · top" : ""}`)}"
          aria-label="${esc(`${v} in slot ${i}${isTop ? ", top of the stack, click to pop" : ""}`)}">${v}</button>`;
      } else {
        inner.innerHTML = "";
      }
    });
    w.ptrTop.style.setProperty("--i", Math.max(top, 0));
    w.ptrTop.classList.toggle("is-null", top < 0);
    const n = snap.items.length;
    w.caption.innerHTML = n === 0
      ? 'top = -1<span class="tag tag-empty">EMPTY</span>'
      : `${n} of 8 slots used · top = ${snap.top}${snap.full ? '<span class="tag tag-full">FULL</span>' : ""}`;
  },
  errorTarget(w) { return w.col; }, // a 9th push shakes the whole column
};

/* ---------- 6b. Queue renderer (linear) ---------- */
const QueueView = {
  init(w) {
    let row = "", idx = "";
    for (let i = 0; i < 8; i++) {
      row += `<div class="qslot" data-slot="${i}"></div>`;
      idx += `<span>${i}</span>`;
    }
    w.canvas.innerHTML = `
      <div class="q-view">
        <div class="q-lane"><div class="ptr q-ptr q-ptr-front" aria-hidden="true">front ▼</div></div>
        <div class="q-row">${row}</div>
        <div class="q-idx" aria-hidden="true">${idx}</div>
        <div class="q-lane"><div class="ptr q-ptr q-ptr-rear" aria-hidden="true">▲ rear</div></div>
      </div>
      <p class="canvas-caption"></p>`;
    w.qslots = $$(".qslot", w.canvas);
    w.ptrFront = $(".q-ptr-front", w.canvas);
    w.ptrRear = $(".q-ptr-rear", w.canvas);
    w.caption = $(".canvas-caption", w.canvas);
    w.canvas.addEventListener("click", (e) => {
      const box = e.target.closest(".qbox");
      if (!box || w.busy) return;
      if (box.classList.contains("is-front")) run(w, () => w.obj.dequeue(), "Dequeue");
      else {
        setMessage(w, "Only the front item can leave, that's the FIFO rule", "error");
        retrigger($(".qbox.is-front", w.canvas), "fx-wiggle");
      }
    });
  },
  draw(w, snap, ctx) {
    const ptrs = ctx ? ctx.pointers : snap.pointers;
    snap.slots.forEach((s, i) => {
      const el = w.qslots[i];
      el.classList.toggle("is-wasted", s.wasted);
      if (s.occupied) {
        const isFront = i === snap.front;
        const tags = [isFront ? "front" : "", i === snap.rear ? "rear" : ""].filter(Boolean).join(", ");
        el.innerHTML = `<button type="button" class="qbox${isFront ? " is-front" : ""}" data-el data-act data-index="${i}" data-value="${s.value}"
          data-tip="${esc(`Value ${s.value} · slot ${i}${tags ? " · " + tags : ""}`)}"
          aria-label="${esc(`${s.value} in slot ${i}${isFront ? ", front of the queue, click to dequeue" : ""}`)}">${s.value}</button>`;
      } else if (s.wasted) {
        el.innerHTML = `<span class="qwaste" data-el data-index="${i}" data-tip="Slot ${i} was freed, but a normal queue cannot reuse it">wasted</span>`;
      } else {
        el.innerHTML = `<span class="sr-only">empty slot ${i}</span>`;
        el.dataset.tip = `Slot ${i} · empty`;
      }
    });
    setLinearPtr(w.ptrFront, ptrs.front);
    setLinearPtr(w.ptrRear, ptrs.rear);
    let cap = `${snap.size} in queue · front = ${snap.front} · rear = ${snap.rear}`;
    if (snap.full) cap += '<span class="tag tag-full">FULL: rear reached the last slot</span>';
    else if (snap.empty) cap += '<span class="tag tag-empty">EMPTY</span>';
    w.caption.innerHTML = cap;
  },
};

function setLinearPtr(el, i) {
  const valid = i >= 0 && i < 8;
  el.classList.toggle("is-null", !valid);
  if (valid) el.style.setProperty("--i", i);
}

/* ---------- 6c. Circular queue renderer (ring) ---------- */
const ARROW_SVG = '<svg class="rp-arrow" viewBox="0 0 14 14" aria-hidden="true"><path d="M7 14L1 5h12z"/></svg>';
const RingView = {
  init(w) {
    let html = '<div class="ring-track"></div>';
    for (let i = 0; i < 8; i++) {
      const a = i * 45;
      html += `<div class="ring-pos" style="--a:${a}deg; --r: var(--R)"><div class="cslot" data-el data-index="${i}" role="button" tabindex="-1"></div></div>`;
      html += `<div class="ring-pos" style="--a:${a}deg; --r: calc(var(--R) + 40px)" aria-hidden="true"><span class="ring-idx">${i}</span></div>`;
    }
    html += `<div class="ring-ptr ring-ptr-front" aria-hidden="true">${ARROW_SVG}<span class="ptr rp-label">front</span></div>`;
    html += `<div class="ring-ptr ring-ptr-rear" aria-hidden="true">${ARROW_SVG}<span class="ptr rp-label">rear</span></div>`;
    w.canvas.innerHTML = `
      <div class="ring-wrap"><div class="ring">${html}</div><div class="ring-center"></div></div>
      <p class="canvas-caption"></p>
      <p class="ring-hint">Drag the ring to rotate it · double-click to reset</p>`;
    w.caption = $(".canvas-caption", w.canvas);
    w.ring = $(".ring", w.canvas);
    w.ringWrap = $(".ring-wrap", w.canvas);
    w.cslots = $$(".cslot", w.canvas);
    w.ringFront = $(".ring-ptr-front", w.canvas);
    w.ringRear = $(".ring-ptr-rear", w.canvas);
    w.ringCenter = $(".ring-center", w.canvas);
    w.ptrAngle = { front: null, rear: null };
    w.viewAngle = 0;

    // Click the front slot to dequeue (ignored if the pointer was dragging).
    w.canvas.addEventListener("click", (e) => {
      const slot = e.target.closest(".cslot");
      if (!slot || w.busy || w.ringDragged) return;
      if (slot.classList.contains("is-front")) run(w, () => w.obj.dequeue(), "Dequeue");
    });
    w.canvas.addEventListener("keydown", (e) => {
      const slot = e.target.closest(".cslot.is-front");
      if (slot && (e.key === "Enter" || e.key === " ") && !w.busy) { e.preventDefault(); run(w, () => w.obj.dequeue(), "Dequeue"); }
    });

    // Drag to rotate the view, double-click to reset it.
    const center = () => { const r = w.ringWrap.getBoundingClientRect(); return [r.left + r.width / 2, r.top + r.height / 2]; };
    const angleAt = (e) => { const [cx, cy] = center(); return Math.atan2(e.clientY - cy, e.clientX - cx) * 180 / Math.PI; };
    let start = null;
    w.ringWrap.addEventListener("pointerdown", (e) => {
      start = { a: angleAt(e), view: w.viewAngle, x: e.clientX, y: e.clientY, id: e.pointerId };
      w.ringDragged = false;
    });
    w.ringWrap.addEventListener("pointermove", (e) => {
      if (!start || e.pointerId !== start.id) return;
      if (!w.ringDragged && Math.hypot(e.clientX - start.x, e.clientY - start.y) < 5) return;
      if (!w.ringDragged) { w.ringDragged = true; w.ringWrap.classList.add("dragging"); w.ringWrap.setPointerCapture(e.pointerId); }
      w.viewAngle = start.view + (angleAt(e) - start.a);
      w.ring.style.setProperty("--view", `${w.viewAngle}deg`);
    });
    const end = () => {
      if (!start) return;
      start = null;
      w.ringWrap.classList.remove("dragging");
      setTimeout(() => { w.ringDragged = false; }, 0);
    };
    w.ringWrap.addEventListener("pointerup", end);
    w.ringWrap.addEventListener("pointercancel", end);
    w.ringWrap.addEventListener("dblclick", () => {
      w.viewAngle = 0;
      w.ring.style.setProperty("--view", "0deg");
    });
  },
  draw(w, snap, ctx) {
    const ptrs = ctx ? ctx.pointers : snap.pointers;
    snap.slots.forEach((s, i) => {
      const el = w.cslots[i];
      const isFront = s.occupied && i === snap.front;
      el.className = "cslot" + (s.occupied ? " occupied" : s.used ? " freed" : "") + (isFront ? " is-front" : "");
      el.dataset.value = s.occupied ? s.value : "";
      el.tabIndex = isFront ? 0 : -1;
      el.toggleAttribute("data-act", isFront);
      el.textContent = s.occupied ? s.value : s.used ? "free" : "";
      const tags = [isFront ? "front" : "", s.occupied && i === snap.rear ? "rear" : ""].filter(Boolean).join(", ");
      el.dataset.tip = s.occupied
        ? `Value ${s.value} · slot ${i}${tags ? " · " + tags : ""}`
        : s.used ? `Slot ${i} · freed, can be reused` : `Slot ${i} · never used yet`;
      el.setAttribute("aria-label", s.occupied ? `${s.value} in slot ${i}${isFront ? ", front, press Enter to dequeue" : ""}` : `slot ${i}, empty`);
    });
    this.setPtr(w, w.ringFront, "front", ptrs.front);
    this.setPtr(w, w.ringRear, "rear", ptrs.rear);
    const status = snap.full ? '<span class="full">full</span>' : snap.empty ? "empty" : "of 8";
    w.ringCenter.innerHTML = `<strong>${snap.size}</strong><span>${status}</span>`;
    let cap = `${snap.size} of 8 slots used · front = ${snap.front} · rear = ${snap.rear}`;
    if (snap.full) cap += '<span class="tag tag-full">FULL</span>';
    else if (snap.empty) cap += '<span class="tag tag-empty">EMPTY</span>';
    w.caption.innerHTML = cap;
  },
  /** Pointers travel around the ring by the shortest way, never snapping across 0. */
  setPtr(w, el, name, index) {
    const valid = index >= 0 && index < 8;
    el.classList.toggle("is-null", !valid);
    if (!valid) return;
    const target = index * 45;
    let cur = w.ptrAngle[name];
    if (cur === null) cur = target;
    let delta = (((target - cur) % 360) + 360) % 360;
    if (delta > 180) delta -= 360;
    w.ptrAngle[name] = cur + delta;
    el.style.setProperty("--pa", `${w.ptrAngle[name]}deg`);
  },
  afterEffects(w, ctx) {
    // CQ-5: an enqueue into a slot that was used before flashes "reused!".
    const s = ctx.step;
    if (s.action === "insert" && w.before.slots[s.index] && w.before.slots[s.index].used) {
      const el = w.cslots[s.index];
      retrigger(el, "fx-reused");
      const badge = document.createElement("span");
      badge.className = "reused-badge";
      badge.textContent = "reused!";
      el.appendChild(badge);
    }
  },
};

/* ---------- 6d. Linked list renderer (singly + doubly) ---------- */
const LIST = { H: 52, GAP: 66, PAD: 56, NULLW: 56, Y: 96 };
const ListView = {
  init(w) {
    w.canvas.classList.add("list-canvas");
    w.canvas.innerHTML = `
      <div class="list-scroll">
        <div class="list-stage">
          <div class="list-content"></div>
          <div class="list-ptrs" aria-hidden="true">
            <div class="ptr list-ptr ptr-head">head</div>
            ${w.cfg.doubly ? '<div class="ptr list-ptr ptr-tail">tail</div>' : ""}
            <div class="ptr list-ptr ptr-cur">cur</div>
          </div>
        </div>
      </div>
      <p class="canvas-caption"></p>`;
    w.stage = $(".list-stage", w.canvas);
    w.content = $(".list-content", w.canvas);
    w.scroller = $(".list-scroll", w.canvas);
    w.caption = $(".canvas-caption", w.canvas);
    w.chips = { head: $(".ptr-head", w.canvas), tail: $(".ptr-tail", w.canvas), cur: $(".ptr-cur", w.canvas) };
    w.nodeW = w.cfg.doubly ? 150 : 112;

    w.canvas.addEventListener("click", (e) => {
      if (w.busy) return;
      const del = e.target.closest(".node-del");
      if (del) { run(w, () => w.obj.deleteAtPosition(+del.dataset.pos), "Delete at Position"); return; }
      const add = e.target.closest(".gap-add");
      if (add) { this.openPop(w, +add.dataset.pos, add); return; }
      const data = e.target.closest(".cell-data");
      if (data) {
        const v = data.closest(".node").dataset.value;
        w.inputs.value.value = v;
        run(w, () => w.obj.search(parseInt(v, 10)), "Search");
      }
    });
  },
  onBusy(w) { this.closePop(w); },
  x(w, i) { return LIST.PAD + (w.cfg.doubly ? LIST.NULLW + LIST.GAP : 0) + i * (w.nodeW + LIST.GAP); },

  /** Which arrows exist at this moment: from the snapshot, adjusted by link/unlink steps. */
  arrowsFor(w, n, ctx) {
    const arrows = [];
    for (let i = 0; i < n; i++) {
      arrows.push({ from: i, ptr: "next", to: i + 1 < n ? i + 1 : "null" });
      if (w.cfg.doubly) arrows.push({ from: i, ptr: "prev", to: i - 1 >= 0 ? i - 1 : "null" });
    }
    if (!ctx) return arrows;
    const find = (from, ptr) => arrows.find((a) => a.from === from && a.ptr === ptr);
    if (ctx.phase === "before") {
      // Delete operations rewire links on the BEFORE layout.
      for (let i = 0; i < ctx.p; i++) {
        const s = ctx.steps[i];
        const a = find(s.index, s.pointer);
        if (!a) continue;
        if (s.action === "link") {
          // bypass: next skips one node forward, prev skips one node back
          const to = s.pointer === "next" ? s.index + 2 : s.index - 2;
          a.to = to >= 0 && to < n ? to : "null";
          a.bypass = true;
          a.fx = i === ctx.p - 1 ? "fx-draw" : "";
        } else if (s.action === "unlink") {
          a.fx = i === ctx.p - 1 ? "fx-retract" : "";
          a.gone = i !== ctx.p - 1;
        }
      }
    } else {
      // Insert operations: links that happen in later steps are not drawn yet.
      for (let i = ctx.switchAt; i < ctx.steps.length; i++) {
        const s = ctx.steps[i];
        if (s.action !== "link") continue;
        const a = find(s.index, s.pointer);
        if (!a) continue;
        if (i >= ctx.p) a.gone = true;
        else if (i === ctx.p - 1) a.fx = "fx-draw";
      }
    }
    return arrows;
  },

  draw(w, snap, ctx) {
    const n = snap.values.length;
    const W = w.nodeW, H = LIST.H, Y = LIST.Y;
    const nullRight = this.x(w, n);
    const width = nullRight + LIST.NULLW + LIST.PAD;
    w.stage.style.width = `${width}px`;

    // Nodes shift to make room when a node is inserted in the middle.
    const shiftFrom = ctx && ctx.step.action === "insert" ? ctx.step.index + 1 : Infinity;
    const shiftDx = -(W + LIST.GAP);
    let html = `<svg class="list-arrows" width="${width}" height="250" aria-hidden="true">
      <defs><marker id="ah-${w.key}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path class="arrow-head" d="M0 0L10 5L0 10z"/></marker></defs>`;
    for (const a of this.arrowsFor(w, n, ctx)) {
      if (a.gone) continue;
      html += this.arrowPath(w, a, n);
    }
    html += "</svg>";

    if (w.cfg.doubly) {
      html += `<div class="null-box" style="left:${LIST.PAD}px; top:${Y}px">NULL</div>`;
    }
    for (let i = 0; i < n; i++) {
      const v = snap.values[i];
      const left = this.x(w, i);
      const shift = i >= shiftFrom ? ` fx-shift" style="--dx:${shiftDx}px; left:${left}px; top:${Y}px; width:${W}px` : `" style="left:${left}px; top:${Y}px; width:${W}px`;
      const tags = [i === 0 ? "head" : "", i === n - 1 && w.cfg.doubly ? "tail" : ""].filter(Boolean).join(", ");
      html += `<div class="node${shift}" data-el data-index="${i}" data-value="${v}" data-tip="${esc(`Value ${v} · position ${i + 1}${tags ? " · " + tags : ""}`)}">
        ${w.cfg.doubly ? '<span class="cell cell-prev" aria-hidden="true"></span>' : ""}
        <button type="button" class="cell cell-data" data-act aria-label="${esc(`Search for ${v} (position ${i + 1})`)}">${v}</button>
        <span class="cell cell-next" aria-hidden="true"></span>
        <button type="button" class="node-del" data-pos="${i + 1}" aria-label="Delete the node at position ${i + 1}">×</button>
        <span class="node-pos" aria-hidden="true">${i + 1}</span>
      </div>`;
    }
    const nullShift = n - 1 >= shiftFrom - 1 && shiftFrom !== Infinity ? ` fx-shift" style="--dx:${shiftDx}px;` : `" style="`;
    html += `<div class="null-box${nullShift} left:${nullRight}px; top:${Y}px">NULL</div>`;

    // "+" buttons in every gap: before node 0 (position 1) ... after the last node (n + 1)
    if (!ctx) {
      for (let k = 0; k <= n; k++) {
        const gx = k === 0 ? this.x(w, 0) - LIST.GAP / 2 : this.x(w, k - 1) + W + LIST.GAP / 2;
        html += `<div class="gap" style="left:${gx - LIST.GAP / 2}px; top:${Y}px; width:${LIST.GAP}px; height:${H}px">
          <button type="button" class="gap-add" data-pos="${k + 1}" aria-label="Insert a value at position ${k + 1}" data-tip="Insert at position ${k + 1}">+</button></div>`;
      }
    }
    w.content.innerHTML = html;

    // Pointer chips glide between nodes.
    const ptrs = ctx ? ctx.pointers : snap.pointers;
    const sameHT = w.cfg.doubly && ptrs.head === ptrs.tail;
    const chipX = (i, nudge) => (i >= 0 && i < n ? this.x(w, i) + W / 2 : nullRight + LIST.NULLW / 2) + nudge;
    this.placeChip(w.chips.head, chipX(ptrs.head, sameHT ? -30 : 0), Y - 44, true);
    if (w.chips.tail) this.placeChip(w.chips.tail, chipX(ptrs.tail, sameHT ? 30 : 0), Y - 44, true);
    const cur = ctx ? ptrs.cur : undefined;
    this.placeChip(w.chips.cur, chipX(cur, 0), Y + H + 34, cur !== undefined && cur >= 0 && cur < n);

    w.caption.textContent = n === 0
      ? "The list is empty: head is NULL. Hover the gap and press + to add a node."
      : `${n} node${n === 1 ? "" : "s"} · positions 1 to ${n}`;

    // Keep the active node in view on long lists.
    if (ctx && ctx.step.index >= 0) {
      const target = this.x(w, Math.min(ctx.step.index, n)) + W / 2 - w.scroller.clientWidth / 2;
      w.scroller.scrollTo({ left: Math.max(0, target), behavior: reducedMotion.matches ? "auto" : "smooth" });
    }
  },
  placeChip(el, x, y, visible) {
    el.style.left = `${x}px`;
    el.style.top = `${y}px`;
    el.classList.toggle("is-null", !visible);
  },
  arrowPath(w, a, n) {
    const W = w.nodeW, H = LIST.H, Y = LIST.Y, mid = Y + H / 2;
    const dbl = w.cfg.doubly;
    let sx, sy, ex, ey, bend;
    if (a.ptr === "next") {
      sx = this.x(w, a.from) + W - 18; sy = dbl ? mid - 9 : mid;
      ex = a.to === "null" ? this.x(w, n) : this.x(w, a.to); ey = sy;
      if (a.to === "null" && a.from !== n - 1) ex = this.x(w, n); // bypass to NULL
      bend = a.bypass ? -70 : -26;
    } else {
      sx = this.x(w, a.from) + 18; sy = mid + 9;
      ex = a.to === "null" ? LIST.PAD + LIST.NULLW : this.x(w, a.to) + W; ey = sy;
      bend = a.bypass ? 70 : 26;
    }
    const dx = (ex - sx) / 3;
    const d = `M${sx} ${sy} C${sx + dx} ${sy + bend}, ${ex - dx} ${ey + bend}, ${ex - (ex > sx ? 2 : -2)} ${ey}`;
    const cls = `arrow ${a.ptr}-arrow${a.bypass ? " bypass" : ""} ${a.fx || ""}`;
    return `<path class="${cls}" d="${d}" pathLength="1" marker-end="url(#ah-${w.key})"/>`;
  },

  /* Mini input that opens from a "+" gap button. */
  openPop(w, pos, anchor) {
    this.closePop(w);
    const pop = document.createElement("div");
    pop.className = "mini-pop";
    const gap = anchor.parentElement;
    pop.style.left = `${parseFloat(gap.style.left) + LIST.GAP / 2}px`;
    pop.style.top = `${LIST.Y + LIST.H + 10}px`;
    pop.innerHTML = `<span>pos ${pos}</span><input type="text" inputmode="numeric" aria-label="Value to insert at position ${pos}" placeholder="value">
      <button type="button" class="btn btn-primary">Insert</button>`;
    w.stage.appendChild(pop);
    w.pop = pop;
    const input = $("input", pop);
    const submit = () => {
      const raw = input.value.trim();
      if (!/^[+-]?\d+$/.test(raw) || Math.abs(parseInt(raw, 10)) > 999) {
        input.classList.add("invalid");
        setHint(w, "The value must be a whole number between -999 and 999.");
        input.focus();
        return;
      }
      this.closePop(w);
      run(w, () => w.obj.insertAtPosition(pos, parseInt(raw, 10)), "Insert at Position");
    };
    $("button", pop).addEventListener("click", submit);
    input.addEventListener("keydown", (e) => {
      if (e.key === "Enter") { e.preventDefault(); submit(); }
      if (e.key === "Escape") { this.closePop(w); anchor.focus(); }
    });
    setTimeout(() => {
      input.focus();
      w.popOutside = (e) => { if (!pop.contains(e.target)) this.closePop(w); };
      document.addEventListener("pointerdown", w.popOutside);
    }, 0);
  },
  closePop(w) {
    if (w.pop) { w.pop.remove(); w.pop = null; }
    if (w.popOutside) { document.removeEventListener("pointerdown", w.popOutside); w.popOutside = null; }
  },
};

/* ---------- 6e. Tree renderer ---------- */
const TREE = { SX: 56, SY: 80, R: 20, TWEEN: 420 };
const TreeView = {
  init(w) {
    w.canvas.classList.add("tree-canvas");
    w.canvas.innerHTML = `
      <div class="tree-wrap">
        <svg class="tree-svg" aria-hidden="true"><g class="tree-view"><g class="tree-lines"></g><g class="tree-nodes"></g><g class="tree-label"></g></g></svg>
        <div class="tree-empty">The tree is empty. Insert a value to plant the root.</div>
        <div class="tree-tools">
          <button type="button" class="btn btn-ghost icon-btn" data-zoom="1.25" aria-label="Zoom in">+</button>
          <button type="button" class="btn btn-ghost icon-btn" data-zoom="0.8" aria-label="Zoom out">−</button>
          <button type="button" class="btn btn-ghost" data-fit aria-label="Fit the whole tree in view">Fit</button>
        </div>
      </div>
      <div class="trav-out" aria-live="off"><span class="trav-label">Output</span><div class="trav-chips"><span class="trav-none">Run a traversal to see the visiting order.</span></div></div>`;
    w.treeWrap = $(".tree-wrap", w.canvas);
    w.svg = $(".tree-svg", w.canvas);
    w.gView = $(".tree-view", w.canvas);
    w.gLines = $(".tree-lines", w.canvas);
    w.gNodes = $(".tree-nodes", w.canvas);
    w.gLabel = $(".tree-label", w.canvas);
    w.travChips = $(".trav-chips", w.canvas);
    w.travLabel = $(".trav-label", w.canvas);
    w.tnodes = new Map(); // value -> { g, x, y }   (view elements only)
    w.tlines = new Map(); // child value -> { line, parent }
    w.view2d = { k: 1, tx: 0, ty: 0 };
    w.userMoved = false;
    w.traversal = null;

    $("[data-fit]", w.canvas).addEventListener("click", () => { w.userMoved = false; this.fit(w, true); });
    $$("[data-zoom]", w.canvas).forEach((b) => b.addEventListener("click", () => {
      const r = w.svg.getBoundingClientRect();
      this.zoomAt(w, parseFloat(b.dataset.zoom), r.width / 2, r.height / 2);
    }));

    // Clicking a node searches for its value; its × deletes it.
    w.svg.addEventListener("click", (e) => {
      const g = e.target.closest(".tnode");
      if (!g || w.busy || w.dragged) return;
      const v = g.dataset.value;
      w.inputs.value.value = v;
      if (e.target.closest(".tnode-del")) run(w, () => w.obj.remove(parseInt(v, 10)), "Delete");
      else run(w, () => w.obj.search(parseInt(v, 10)), "Search");
    });

    // Pan (one pointer) and pinch zoom (two pointers).
    const pointers = new Map();
    let pan = null, pinch = null;
    w.treeWrap.addEventListener("pointerdown", (e) => {
      if (e.target.closest(".tree-tools")) return;
      pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
      w.dragged = false;
      if (pointers.size === 1) pan = { x: e.clientX, y: e.clientY, tx: w.view2d.tx, ty: w.view2d.ty };
      if (pointers.size === 2) {
        const [a, b] = Array.from(pointers.values());
        pinch = { d: Math.hypot(a.x - b.x, a.y - b.y), k: w.view2d.k };
        pan = null;
      }
    });
    w.treeWrap.addEventListener("pointermove", (e) => {
      if (!pointers.has(e.pointerId)) return;
      pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
      if (pinch && pointers.size === 2) {
        const [a, b] = Array.from(pointers.values());
        const r = w.svg.getBoundingClientRect();
        const target = pinch.k * Math.hypot(a.x - b.x, a.y - b.y) / pinch.d;
        this.zoomAt(w, target / w.view2d.k, (a.x + b.x) / 2 - r.left, (a.y + b.y) / 2 - r.top, true);
        w.dragged = true;
        return;
      }
      if (!pan) return;
      const dx = e.clientX - pan.x, dy = e.clientY - pan.y;
      if (!w.dragged && Math.hypot(dx, dy) < 5) return;
      if (!w.dragged) { w.dragged = true; w.treeWrap.classList.add("dragging"); w.treeWrap.setPointerCapture(e.pointerId); }
      w.view2d.tx = pan.tx + dx;
      w.view2d.ty = pan.ty + dy;
      w.userMoved = true;
      this.applyView(w);
    });
    const up = (e) => {
      pointers.delete(e.pointerId);
      if (pointers.size < 2) pinch = null;
      if (pointers.size === 0) { pan = null; w.treeWrap.classList.remove("dragging"); setTimeout(() => { w.dragged = false; }, 0); }
    };
    w.treeWrap.addEventListener("pointerup", up);
    w.treeWrap.addEventListener("pointercancel", up);
    w.treeWrap.addEventListener("wheel", (e) => {
      e.preventDefault();
      const r = w.svg.getBoundingClientRect();
      this.zoomAt(w, e.deltaY < 0 ? 1.12 : 1 / 1.12, e.clientX - r.left, e.clientY - r.top, true);
    }, { passive: false });
  },

  onShow(w) { if (!w.userMoved) this.fit(w, false); },

  onOperation(w) {
    // Traversal output belongs to the last traversal; other changes clear it.
    const name = w.lastOp;
    if (/order$/.test(name)) w.traversal = { name, values: w.steps.filter((s) => s.action === "visit").map((s) => s.value) };
    else if (name !== "Search") w.traversal = null;
  },

  draw(w, snap, ctx) {
    const nodes = snap.nodes;
    $(".tree-empty", w.canvas).hidden = nodes.length > 0;
    const seen = new Set();
    // Path lighting: every node compared so far in this step sequence.
    const path = new Set();
    if (ctx) for (let i = 0; i < ctx.p; i++) {
      const s = ctx.steps[i];
      if (s.action === "highlight" || s.action === "found") path.add(s.value);
    }

    for (const n of nodes) {
      seen.add(n.value);
      const tx = n.x * TREE.SX, ty = n.y * TREE.SY;
      let rec = w.tnodes.get(n.value);
      if (!rec) {
        const g = svgEl("g", { class: "tnode", "data-el": "", "data-value": n.value });
        const body = svgEl("g", { class: "tnode-body" });
        body.appendChild(svgEl("circle", { r: TREE.R }));
        const t = svgEl("text", {});
        t.textContent = n.value;
        body.appendChild(t);
        g.appendChild(body);
        const del = svgEl("g", { class: "tnode-del", transform: `translate(${TREE.R - 2},${-TREE.R + 2})` });
        del.appendChild(svgEl("circle", { r: 8 }));
        const x = svgEl("path", { d: "M-3 -3L3 3M3 -3L-3 3" });
        del.appendChild(x);
        g.appendChild(del);
        w.gNodes.appendChild(g);
        rec = { g, x: tx, y: ty };
        g.setAttribute("transform", `translate(${tx},${ty})`);
        w.tnodes.set(n.value, rec);
      }
      rec.tx = tx; rec.ty = ty;
      rec.g.classList.toggle("on-path", path.has(n.value));
      rec.g.dataset.tip = `Value ${n.value} · depth ${n.y}${n.y === 0 ? " · root" : ""}`;
    }
    for (const [v, rec] of w.tnodes) if (!seen.has(v)) { rec.g.remove(); w.tnodes.delete(v); }

    // Lines: one per child, keyed by the child's value.
    const lineSeen = new Set();
    for (const n of nodes) {
      for (const [has, child] of [[n.hasLeft, n.leftValue], [n.hasRight, n.rightValue]]) {
        if (!has) continue;
        lineSeen.add(child);
        let rec = w.tlines.get(child);
        if (!rec) {
          rec = { line: svgEl("line", { class: "tree-line" }), parent: n.value };
          w.gLines.appendChild(rec.line);
          w.tlines.set(child, rec);
        }
        rec.parent = n.value;
        rec.line.classList.toggle("on-path", path.has(n.value) && path.has(child));
      }
    }
    for (const [c, rec] of w.tlines) if (!lineSeen.has(c)) { rec.line.remove(); w.tlines.delete(c); }

    this.tween(w);
    this.drawLabel(w, ctx);
    this.drawOutput(w, ctx);
    if (!w.userMoved) this.fit(w, true);
  },

  /** Glide nodes (and their lines) to new layout positions. */
  tween(w) {
    cancelAnimationFrame(w.tweenId);
    const recs = Array.from(w.tnodes.values());
    recs.forEach((r) => { r.sx = r.x; r.sy = r.y; });
    const t0 = performance.now();
    const dur = reducedMotion.matches ? 0 : TREE.TWEEN;
    const ease = (u) => 1 - Math.pow(1 - u, 3);
    const frame = (now) => {
      const u = dur ? Math.min(1, (now - t0) / dur) : 1;
      const e = ease(u);
      for (const r of recs) {
        r.x = r.sx + (r.tx - r.sx) * e;
        r.y = r.sy + (r.ty - r.sy) * e;
        r.g.setAttribute("transform", `translate(${r.x},${r.y})`);
      }
      for (const [child, l] of w.tlines) {
        const p = w.tnodes.get(l.parent), c = w.tnodes.get(child);
        if (!p || !c) continue;
        l.line.setAttribute("x1", p.x); l.line.setAttribute("y1", p.y);
        l.line.setAttribute("x2", c.x); l.line.setAttribute("y2", c.y);
      }
      if (u < 1) w.tweenId = requestAnimationFrame(frame);
    };
    frame(t0);
  },

  /** Floating comparison label next to the current node ("42 > 30 → right"). */
  drawLabel(w, ctx) {
    w.gLabel.innerHTML = "";
    if (!ctx || ctx.step.action !== "highlight" || !/, go (left|right)$/.test(ctx.step.message)) return;
    const rec = w.tnodes.get(ctx.step.value);
    if (!rec) return;
    const text = ctx.step.message.replace(/, go (left|right)$/, " → $1");
    const width = text.length * 7.6 + 18;
    const g = svgEl("g", { class: "cmp-label", transform: `translate(${rec.tx + TREE.R + 8},${rec.ty - TREE.R - 18})` });
    g.appendChild(svgEl("rect", { x: 0, y: -12, width, height: 24, rx: 8 }));
    const t = svgEl("text", { x: 9, y: 0 });
    t.textContent = text;
    g.appendChild(t);
    w.gLabel.appendChild(g);
  },

  /** Traversal output chips (BT-6): build up one visit at a time. */
  drawOutput(w, ctx) {
    const trav = w.traversal;
    if (!trav) {
      w.travLabel.textContent = "Output";
      w.travChips.innerHTML = '<span class="trav-none">Run a traversal to see the visiting order.</span>';
      return;
    }
    let values = trav.values;
    let flyLast = false;
    if (ctx && /order$/.test(w.lastOp)) {
      values = [];
      for (let i = 0; i < ctx.p; i++) if (ctx.steps[i].action === "visit") values.push(ctx.steps[i].value);
      flyLast = ctx.step.action === "visit";
    }
    w.travLabel.textContent = trav.name;
    w.travChips.innerHTML = values.length
      ? values.map((v) => `<span class="trav-chip">${v}</span>`).join("")
      : '<span class="trav-none">…</span>';
    if (flyLast && values.length) {
      const chip = w.travChips.lastElementChild;
      const rec = w.tnodes.get(ctx.step.value);
      if (rec) {
        const nodeBox = rec.g.getBoundingClientRect();
        const chipBox = chip.getBoundingClientRect();
        chip.style.setProperty("--fx", `${nodeBox.left + nodeBox.width / 2 - (chipBox.left + chipBox.width / 2)}px`);
        chip.style.setProperty("--fy", `${nodeBox.top + nodeBox.height / 2 - (chipBox.top + chipBox.height / 2)}px`);
      }
      chip.classList.add("fly");
    }
  },

  /* Pan / zoom */
  applyView(w, animate) {
    const v = w.view2d;
    w.gView.classList.toggle("no-anim", !animate);
    w.gView.style.transform = `translate(${v.tx}px, ${v.ty}px) scale(${v.k})`;
  },
  zoomAt(w, factor, cx, cy, noAnim) {
    const v = w.view2d;
    const k = Math.min(3, Math.max(0.25, v.k * factor));
    const f = k / v.k;
    v.tx = cx - (cx - v.tx) * f;
    v.ty = cy - (cy - v.ty) * f;
    v.k = k;
    w.userMoved = true;
    this.applyView(w, !noAnim);
  },
  fit(w, animate) {
    const r = w.svg.getBoundingClientRect();
    if (!r.width || !r.height) return;
    let minX = 0, maxX = 0, maxY = 0, first = true;
    for (const rec of w.tnodes.values()) {
      const x = rec.tx !== undefined ? rec.tx : rec.x, y = rec.ty !== undefined ? rec.ty : rec.y;
      if (first) { minX = maxX = x; maxY = y; first = false; }
      minX = Math.min(minX, x); maxX = Math.max(maxX, x); maxY = Math.max(maxY, y);
    }
    const pad = 56;
    const bw = maxX - minX + pad * 2, bh = maxY + pad * 2;
    const k = Math.min(1.4, Math.max(0.25, Math.min(r.width / bw, r.height / bh)));
    w.view2d.k = k;
    w.view2d.tx = r.width / 2 - k * (minX + maxX) / 2;
    w.view2d.ty = Math.max(pad * k, (r.height - k * maxY) / 2 - 10);
    if (bh * k > r.height) w.view2d.ty = pad * k;
    this.applyView(w, animate);
  },
};

/* ---------- 7. Tooltips + boot ---------- */
function initTooltips() {
  const tip = $("#tooltip");
  let current = null;
  const showTip = (el) => {
    const text = el.dataset.tip;
    if (!text) return;
    current = el;
    tip.textContent = text;
    tip.hidden = false;
    const r = el.getBoundingClientRect();
    const half = tip.offsetWidth / 2;
    const x = Math.min(window.innerWidth - half - 8, Math.max(half + 8, r.left + r.width / 2));
    tip.style.left = `${x}px`;
    tip.style.top = `${Math.max(tip.offsetHeight + 12, r.top)}px`;
  };
  const hideTip = () => { current = null; tip.hidden = true; };
  document.addEventListener("pointerover", (e) => {
    const el = e.target.closest("[data-tip]");
    if (el && el !== current) showTip(el);
    else if (!el && current) hideTip();
  });
  document.addEventListener("focusin", (e) => { const el = e.target.closest("[data-tip]"); if (el) showTip(el); });
  document.addEventListener("focusout", hideTip);
  document.addEventListener("scroll", hideTip, true);
  document.addEventListener("pointerdown", hideTip);
}

boot();
