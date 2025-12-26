# GDB MCP Server Tools Reference

## Session Management

### gdb_start

Start a new GDB debugging session.

**Parameters:**
- `gdbPath` (string, optional): Path to the GDB binary. Defaults to "gdb".
- `workingDir` (string, optional): Working directory for GDB.

**Returns:**
- `sessionId`: Unique identifier for the new session.

**Example:**
```json
{
  "name": "gdb_start",
  "arguments": {
    "gdbPath": "/usr/bin/gdb",
    "workingDir": "/path/to/project"
  }
}
```

### gdb_terminate

Terminate a GDB session.

**Parameters:**
- `sessionId` (string, required): The session ID to terminate.

**Returns:**
- Confirmation message.

### gdb_list_sessions

List all active GDB sessions.

**Parameters:** None

**Returns:**
- List of active sessions with their IDs and states.

---

## Program Loading

### gdb_load

Load a program into GDB for debugging.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `program` (string, required): Path to the program executable.
- `arguments` (array of strings, optional): Command-line arguments for the program.

**Example:**
```json
{
  "name": "gdb_load",
  "arguments": {
    "sessionId": "sess_abc123",
    "program": "/path/to/myprogram",
    "arguments": ["--config", "test.conf"]
  }
}
```

### gdb_attach

Attach to a running process.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `pid` (integer, required): Process ID to attach to.

**Note:** Requires appropriate permissions. May need `ptrace_scope` settings on Linux.

### gdb_load_core

Load a core dump file for post-mortem debugging.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `program` (string, required): Path to the program executable.
- `corePath` (string, required): Path to the core dump file.

---

## Execution Control

### gdb_continue

Continue program execution until next breakpoint or exit.

**Parameters:**
- `sessionId` (string, required): GDB session ID.

### gdb_step

Step into functions (single step by source line or instruction).

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `instructions` (boolean, optional): If true, step by instructions (stepi) instead of source lines.

### gdb_next

Step over function calls.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `instructions` (boolean, optional): If true, step by instructions (nexti) instead of source lines.

### gdb_finish

Execute until the current function returns.

**Parameters:**
- `sessionId` (string, required): GDB session ID.

---

## Breakpoints

### gdb_set_breakpoint

Set a breakpoint at a location.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `location` (string, required): Breakpoint location. Can be:
  - Function name: `main`
  - File and line: `myfile.c:42`
  - Address: `*0x400520`
- `condition` (string, optional): Conditional expression for the breakpoint.

**Example:**
```json
{
  "name": "gdb_set_breakpoint",
  "arguments": {
    "sessionId": "sess_abc123",
    "location": "myfile.c:100",
    "condition": "i > 10"
  }
}
```

---

## Inspection

### gdb_backtrace

Show the current call stack.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `full` (boolean, optional): Include local variables in each frame.
- `limit` (integer, optional): Maximum number of frames to show.

### gdb_print

Evaluate and print an expression.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `expression` (string, required): Expression to evaluate.

**Example:**
```json
{
  "name": "gdb_print",
  "arguments": {
    "sessionId": "sess_abc123",
    "expression": "*my_struct_ptr"
  }
}
```

### gdb_examine

Examine memory at a given address.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `address` (string, required): Memory address to examine.
- `count` (integer, optional): Number of units to display. Default: 1.
- `format` (string, optional): Display format. Options:
  - `x`: Hexadecimal
  - `d`: Decimal
  - `u`: Unsigned decimal
  - `o`: Octal
  - `t`: Binary
  - `a`: Address
  - `c`: Character
  - `f`: Float
  - `s`: String
  - `i`: Instruction

### gdb_info_registers

Show CPU register values.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `register` (string, optional): Specific register name (e.g., "rax", "eip"). If omitted, shows all registers.

### gdb_command

Execute an arbitrary GDB command (escape hatch for advanced use).

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `command` (string, required): The GDB command to execute.

**Warning:** This is a raw escape hatch. Use with caution.

---

## GLib/GObject Tools

### gdb_glib_print_gobject

Pretty-print a GObject instance showing type, reference count, and structure.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `expression` (string, required): Pointer or variable referencing a GObject.

**Example:**
```json
{
  "name": "gdb_glib_print_gobject",
  "arguments": {
    "sessionId": "sess_abc123",
    "expression": "self"
  }
}
```

**Output includes:**
- Type name (e.g., "GtkButton")
- Reference count
- Object structure data

### gdb_glib_print_glist

Pretty-print GList or GSList contents.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `expression` (string, required): Pointer to a GList or GSList.

**Note:** Shows up to 20 items by default.

### gdb_glib_print_ghash

Pretty-print GHashTable information.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `expression` (string, required): Pointer to a GHashTable.

**Output includes:**
- Table size
- Number of entries
- Internal structure

### gdb_glib_type_hierarchy

Show the GType inheritance hierarchy for a GObject.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `expression` (string, required): Pointer to a GObject instance.

**Example output:**
```
Type Hierarchy for: button
GtkButton
  └─ GtkWidget
    └─ GInitiallyUnowned
      └─ GObject
```

### gdb_glib_signal_info

List signals registered on a GObject type.

**Parameters:**
- `sessionId` (string, required): GDB session ID.
- `expression` (string, required): Pointer to a GObject instance.

**Output includes:**
- Type name
- Number of signals
- List of signal names
