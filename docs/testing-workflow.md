# GDB MCP Server - Testing Workflow

A comprehensive step-by-step guide for testing all 21 tools of the GDB MCP server.

## Prerequisites

1. **Build the server:**
   ```bash
   make
   ```

2. **Build the example application:**
   ```bash
   make examples
   ```

3. **Configure the MCP server** in Claude Code (see README.md)

## Test Session Overview

The testing workflow exercises all tools in a logical sequence:

| Category | Tools | Count |
|----------|-------|-------|
| Session Management | gdb_start, gdb_terminate, gdb_list_sessions | 3 |
| Program Loading | gdb_load, gdb_attach, gdb_load_core | 3 |
| Breakpoints | gdb_set_breakpoint | 1 |
| Execution Control | gdb_continue, gdb_step, gdb_next, gdb_finish | 4 |
| Inspection | gdb_backtrace, gdb_print, gdb_examine, gdb_info_registers, gdb_command | 5 |
| GLib/GObject | gdb_glib_print_gobject, gdb_glib_print_glist, gdb_glib_print_ghash, gdb_glib_type_hierarchy, gdb_glib_signal_info | 5 |

**Total: 21 tools**

---

## 1. Session Management Tests

### 1.1 gdb_list_sessions (empty)

**Purpose:** Verify listing works with no active sessions.

**Action:** Call `gdb_list_sessions` with no arguments.

**Expected:** Response showing 0 active sessions.

---

### 1.2 gdb_start

**Purpose:** Create a new GDB debugging session.

**Action:** Call `gdb_start` with no arguments (uses defaults).

**Expected:**
- Returns a sessionId
- Session is in READY state
- GDB path shown in response

**Save the sessionId for subsequent tests.**

---

### 1.3 gdb_start (with options)

**Purpose:** Verify custom options are respected.

**Action:** Call `gdb_start` with:
- `workingDir`: "/tmp"

**Expected:** Session created with specified working directory.

---

### 1.4 gdb_list_sessions (with sessions)

**Purpose:** Verify active sessions are listed.

**Action:** Call `gdb_list_sessions`.

**Expected:** Shows both sessions with their IDs and states.

---

### 1.5 gdb_terminate

**Purpose:** End a session cleanly.

**Action:** Call `gdb_terminate` with the second session's ID.

**Expected:** Session terminated, no longer appears in list.

---

## 2. Program Loading Tests

### 2.1 gdb_load

**Purpose:** Load an executable for debugging.

**Action:** Call `gdb_load` with:
- `sessionId`: (from step 1.2)
- `program`: "./build/gobject-demo"

**Expected:** Program loaded successfully.

---

### 2.2 gdb_attach (optional)

**Purpose:** Attach to a running process.

**Setup:** Start gobject-demo in a separate terminal with a sleep:
```bash
./build/gobject-demo & echo $!
```

**Action:** Call `gdb_attach` with:
- `sessionId`: (new session)
- `pid`: (from above)

**Expected:** Attached to process.

**Note:** Requires appropriate permissions. Skip if not available.

---

### 2.3 gdb_load_core (optional)

**Purpose:** Load a core dump for post-mortem debugging.

**Setup:** Generate a core dump (requires `ulimit -c unlimited`).

**Action:** Call `gdb_load_core` with:
- `sessionId`: (new session)
- `program`: "./build/gobject-demo"
- `corePath`: "/path/to/core"

**Note:** Skip if core dump not available.

---

## 3. Breakpoint Tests

### 3.1 gdb_set_breakpoint (function name)

**Purpose:** Set breakpoint on a function.

**Action:** Call `gdb_set_breakpoint` with:
- `sessionId`: (from step 1.2)
- `location`: "main"

**Expected:** Breakpoint 1 set at main.

---

### 3.2 gdb_set_breakpoint (file:line)

**Action:** Call `gdb_set_breakpoint` with:
- `sessionId`: (from step 1.2)
- `location`: "gobject-demo.c:280"

**Expected:** Breakpoint 2 set at specified line.

---

### 3.3 gdb_set_breakpoint (with condition)

**Action:** Call `gdb_set_breakpoint` with:
- `sessionId`: (from step 1.2)
- `location`: "demo_object_increment"
- `condition`: "self->counter > 0"

**Expected:** Conditional breakpoint set.

---

## 4. Execution Control Tests

### 4.1 gdb_continue

**Purpose:** Start/continue program execution.

**Action:** Call `gdb_continue` with sessionId.

**Expected:** Program runs and stops at main breakpoint.

---

### 4.2 gdb_next

**Purpose:** Step over function calls.

**Action:** Call `gdb_next` with sessionId.

**Expected:** Advances to next source line without entering functions.

---

### 4.3 gdb_step

**Purpose:** Step into function calls.

**Action:** Call `gdb_step` with sessionId.

**Expected:** Steps into the next function.

---

### 4.4 gdb_step (instruction mode)

**Action:** Call `gdb_step` with:
- `sessionId`: (from step 1.2)
- `instructions`: true

**Expected:** Steps one machine instruction.

---

### 4.5 gdb_next (instruction mode)

**Action:** Call `gdb_next` with:
- `sessionId`: (from step 1.2)
- `instructions`: true

**Expected:** Steps over at instruction level.

---

### 4.6 gdb_finish

**Purpose:** Run until current function returns.

**Setup:** Step into a function first.

**Action:** Call `gdb_finish` with sessionId.

**Expected:** Executes until function returns, showing return value.

---

## 5. Inspection Tests

### 5.1 gdb_backtrace

**Purpose:** Show call stack.

**Action:** Call `gdb_backtrace` with sessionId.

**Expected:** Shows current call stack with frame numbers.

---

### 5.2 gdb_backtrace (full)

**Action:** Call `gdb_backtrace` with:
- `sessionId`: (from step 1.2)
- `full`: true

**Expected:** Shows call stack with local variables.

---

### 5.3 gdb_backtrace (limited)

**Action:** Call `gdb_backtrace` with:
- `sessionId`: (from step 1.2)
- `limit`: 3

**Expected:** Shows only top 3 frames.

---

### 5.4 gdb_print

**Purpose:** Evaluate and print expressions.

**Action:** Call `gdb_print` with:
- `sessionId`: (from step 1.2)
- `expression`: "demo"

**Expected:** Shows pointer value.

---

### 5.5 gdb_print (struct member)

**Action:** Call `gdb_print` with:
- `sessionId`: (from step 1.2)
- `expression`: "demo->counter"

**Expected:** Shows counter value.

---

### 5.6 gdb_examine

**Purpose:** Examine raw memory.

**Action:** Call `gdb_examine` with:
- `sessionId`: (from step 1.2)
- `expression`: "demo"
- `format`: "x"
- `count`: 8

**Expected:** Shows 8 hex words starting at demo's address.

---

### 5.7 gdb_info_registers

**Purpose:** Show CPU registers.

**Action:** Call `gdb_info_registers` with sessionId.

**Expected:** Shows all register values.

---

### 5.8 gdb_info_registers (specific)

**Action:** Call `gdb_info_registers` with:
- `sessionId`: (from step 1.2)
- `register`: "rsp"

**Expected:** Shows only RSP value.

---

### 5.9 gdb_command

**Purpose:** Execute arbitrary GDB commands.

**Action:** Call `gdb_command` with:
- `sessionId`: (from step 1.2)
- `command`: "info locals"

**Expected:** Shows local variables.

---

### 5.10 gdb_command (ptype)

**Action:** Call `gdb_command` with:
- `sessionId`: (from step 1.2)
- `command`: "ptype DemoObject"

**Expected:** Shows DemoObject type definition.

---

## 6. GLib/GObject Tests

**Setup:** Continue to a breakpoint inside `demo_object_increment` where `self` is available.

### 6.1 gdb_glib_print_gobject

**Purpose:** Pretty-print GObject instances.

**Action:** Call `gdb_glib_print_gobject` with:
- `sessionId`: (from step 1.2)
- `expression`: "self"

**Expected:** Shows GObject type, reference count, and properties.

---

### 6.2 gdb_glib_print_glist

**Purpose:** Print GList contents.

**Action:** Call `gdb_glib_print_glist` with:
- `sessionId`: (from step 1.2)
- `expression`: "self->items"

**Expected:** Shows list contents (apple, banana, cherry, date).

---

### 6.3 gdb_glib_print_ghash

**Purpose:** Print GHashTable key-value pairs.

**Action:** Call `gdb_glib_print_ghash` with:
- `sessionId`: (from step 1.2)
- `expression`: "self->properties"

**Expected:** Shows hash table entries (color, size, priority).

---

### 6.4 gdb_glib_type_hierarchy

**Purpose:** Show GType inheritance chain.

**Action:** Call `gdb_glib_type_hierarchy` with:
- `sessionId`: (from step 1.2)
- `expression`: "self"

**Expected:** Shows: DemoObject -> GObject -> ...

---

### 6.5 gdb_glib_signal_info

**Purpose:** List signals on a GObject.

**Action:** Call `gdb_glib_signal_info` with:
- `sessionId`: (from step 1.2)
- `expression`: "self"

**Expected:** Shows "counter-changed" signal.

---

## 7. Cleanup

### 7.1 gdb_terminate (final)

**Action:** Call `gdb_terminate` with the session ID.

**Expected:** Session ends cleanly.

---

### 7.2 gdb_list_sessions (verify empty)

**Action:** Call `gdb_list_sessions`.

**Expected:** Shows 0 active sessions.

---

## Test Results Template

| Tool | Status | Notes |
|------|--------|-------|
| gdb_start | | |
| gdb_terminate | | |
| gdb_list_sessions | | |
| gdb_load | | |
| gdb_attach | | |
| gdb_load_core | | |
| gdb_set_breakpoint | | |
| gdb_continue | | |
| gdb_step | | |
| gdb_next | | |
| gdb_finish | | |
| gdb_backtrace | | |
| gdb_print | | |
| gdb_examine | | |
| gdb_info_registers | | |
| gdb_command | | |
| gdb_glib_print_gobject | | |
| gdb_glib_print_glist | | |
| gdb_glib_print_ghash | | |
| gdb_glib_type_hierarchy | | |
| gdb_glib_signal_info | | |

**Status:** PASS / FAIL / SKIP
