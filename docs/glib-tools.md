# GLib/GObject Debugging Tools

## Overview

The GDB MCP Server includes specialized tools for debugging GLib and GObject-based applications. These tools understand GLib's type system and data structures, providing formatted output that's easier to understand than raw memory dumps.

## Available Tools

### gdb_glib_print_gobject

Pretty-print a GObject instance with type information and reference count.

**What it shows:**
- GType name (e.g., "GtkButton", "McpServer")
- Reference count
- Object structure data

**Example usage:**
```json
{
  "tool": "gdb_glib_print_gobject",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "self"
  }
}
```

**Example output:**
```
GObject Analysis: self

Type: $1 = 0x55f3a2b4c8d0 "GtkButton"
Reference Count: $2 = 3

Object Data:
$3 = {
  parent_instance = {
    g_type_instance = {g_class = 0x55f3a2b50000},
    ref_count = 3,
    qdata = 0x55f3a2b60010
  },
  priv = 0x55f3a2b4d000
}
```

### gdb_glib_print_glist

Iterate through and print GList or GSList contents.

**Features:**
- Shows up to 20 items
- Displays index and data pointer for each element
- Works with both GList and GSList

**Example usage:**
```json
{
  "tool": "gdb_glib_print_glist",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "my_list"
  }
}
```

**Example output:**
```
GList Contents: my_list

[0]: $1 = (void *) 0x55f3a2b4c000
[1]: $2 = (void *) 0x55f3a2b4c100
[2]: $3 = (void *) 0x55f3a2b4c200

Total items shown: 3
```

**Tip:** To see the actual data, use `gdb_print` with casting:
```json
{
  "tool": "gdb_print",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "*(MyStruct*)my_list->data"
  }
}
```

### gdb_glib_print_ghash

Display GHashTable information and structure.

**What it shows:**
- Table size (bucket count)
- Number of entries (nnodes)
- Internal structure details

**Example usage:**
```json
{
  "tool": "gdb_glib_print_ghash",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "hash_table"
  }
}
```

**Example output:**
```
GHashTable Analysis: hash_table

Size: $1 = 64
Number of entries: $2 = 12

Structure:
$3 = {
  size = 64,
  mod = 63,
  nnodes = 12,
  noccupied = 12,
  ...
}

Note: To iterate entries, use gdb_command with:
  'call g_hash_table_foreach(table, print_func, NULL)'
```

### gdb_glib_type_hierarchy

Show the complete GType inheritance chain for an object.

**Features:**
- Walks up the type hierarchy from the object's type
- Shows inheritance with visual tree formatting
- Stops at GObject (or the fundamental type)

**Example usage:**
```json
{
  "tool": "gdb_glib_type_hierarchy",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "widget"
  }
}
```

**Example output:**
```
Type Hierarchy for: widget

$1 = 0x7f1234567890 "GtkButton"
  └─ $2 = 0x7f1234567800 "GtkWidget"
    └─ $3 = 0x7f1234567700 "GInitiallyUnowned"
      └─ $4 = 0x7f1234567600 "GObject"
```

### gdb_glib_signal_info

List all signals registered on a GObject type.

**What it shows:**
- Type name of the object
- Number of signals
- List of signal names

**Example usage:**
```json
{
  "tool": "gdb_glib_signal_info",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "button"
  }
}
```

**Example output:**
```
Signal Information for: button

Type: $1 = 0x55f3a2b4c8d0 "GtkButton"

Number of signals: $2 = 5

Signals:
  - $3 = 0x7f12345 "clicked"
  - $4 = 0x7f12346 "activate"
  - $5 = 0x7f12347 "enter"
  - $6 = 0x7f12348 "leave"
  - $7 = 0x7f12349 "pressed"
```

## Tips for GLib Debugging

### 1. Check Reference Counts

Memory leaks often show up as incorrect reference counts:

```json
{
  "tool": "gdb_print",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "((GObject*)self)->ref_count"
  }
}
```

### 2. Inspect GValue

```json
{
  "tool": "gdb_print",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "g_value_peek_pointer(&value)"
  }
}
```

### 3. Check GType at Runtime

```json
{
  "tool": "gdb_command",
  "arguments": {
    "sessionId": "gdb-abc123",
    "command": "print g_type_name(G_TYPE_FROM_INSTANCE(object))"
  }
}
```

### 4. Examine GError

```json
{
  "tool": "gdb_print",
  "arguments": {
    "sessionId": "gdb-abc123",
    "expression": "*error"
  }
}
```

This shows domain, code, and message.

### 5. GLib Debugging Environment Variables

Set these before running your program:

```bash
G_DEBUG=fatal-warnings     # Make warnings fatal
G_DEBUG=fatal-criticals    # Make criticals fatal
G_MESSAGES_DEBUG=all       # Show all debug messages
```

## Requirements

These tools require:

1. **Debug symbols** - Compile with `-g` flag
2. **GLib headers** - Type information needs to be available
3. **GObject applications** - Tools use GObject API functions

## Limitations

- Hash table iteration requires a callback function
- Large data structures may be truncated
- Some internal structures may vary between GLib versions
