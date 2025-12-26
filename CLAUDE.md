# GDB MCP Server Development Guide

## Project Overview

gdb-mcp-server is a GLib/GObject implementation of a Model Context Protocol (MCP) server for GDB debugging. It allows AI assistants to debug programs through structured tool calls.

**License:** AGPLv3
**Language:** C (gnu89)
**Dependencies:** glib-2.0, gobject-2.0, gio-2.0, gio-unix-2.0, json-glib-1.0, mcp-glib (submodule)

## Building

```bash
# Build everything (including mcp-glib dependency)
make

# Clean build artifacts
make clean

# Clean everything including mcp-glib
make distclean

# Check dependencies
make check-deps
```

## Directory Structure

```
mcp-gdb-glib/
├── mcp-gdb/          # Public headers
├── src/              # Implementation files
│   └── tools/        # Tool handlers by category
├── deps/
│   └── mcp-glib/     # MCP library (git submodule)
├── docs/             # Documentation
├── tests/            # Test files (future)
└── build/            # Build output
```

## Code Style

- Use gnu89 C standard
- 4-space indentation with TABs
- Block comments only: `/* comment */`
- All public symbols prefixed with `gdb_` or `Gdb`
- Properties use kebab-case: `session-id`
- Signals use kebab-case: `state-changed`
- Use `g_autoptr()` for automatic cleanup
- Use `g_steal_pointer()` for ownership transfer
- Full GObject Introspection annotations on public APIs

## Key Types

### GObject Classes

| Class | Description |
|-------|-------------|
| `GdbMcpServer` | Main server, owns McpServer and registers tools |
| `GdbSessionManager` | Manages multiple concurrent GDB sessions |
| `GdbSession` | Single GDB subprocess lifecycle |
| `GdbMiParser` | Parses GDB/MI structured output |

### Boxed Types

| Type | Description |
|------|-------------|
| `GdbMiRecord` | Parsed MI output record |

### Enumerations

- `GdbSessionState` - Session states (DISCONNECTED, STARTING, READY, RUNNING, STOPPED, TERMINATED, ERROR)
- `GdbStopReason` - Why program stopped (BREAKPOINT, WATCHPOINT, SIGNAL, etc.)
- `GdbMiRecordType` - MI record types (RESULT, EXEC_ASYNC, CONSOLE, etc.)
- `GdbMiResultClass` - MI result classes (DONE, RUNNING, ERROR, EXIT)
- `GdbErrorCode` - Error codes for GDB_ERROR domain

## Tool Handler Pattern

```c
McpToolResult *
gdb_tools_handle_tool_name (McpServer   *server,
                            const gchar *name,
                            JsonObject  *arguments,
                            gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;

    /* Get session from arguments */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Execute GDB command */
    g_autofree gchar *output = gdb_tools_execute_command_sync (session, "command", NULL);

    /* Return result */
    return gdb_tools_create_success_result ("Output: %s", output);
}
```

## Schema Creation Pattern

```c
JsonNode *
gdb_tools_create_tool_schema (void)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();

    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "object");

    json_builder_set_member_name (builder, "properties");
    json_builder_begin_object (builder);

    /* Add sessionId property */
    json_builder_set_member_name (builder, "sessionId");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "GDB session ID");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}
```

## Tool Categories

| File | Tools |
|------|-------|
| `gdb-tools-session.c` | gdb_start, gdb_terminate, gdb_list_sessions |
| `gdb-tools-load.c` | gdb_load, gdb_attach, gdb_load_core |
| `gdb-tools-exec.c` | gdb_continue, gdb_step, gdb_next, gdb_finish |
| `gdb-tools-breakpoint.c` | gdb_set_breakpoint |
| `gdb-tools-inspect.c` | gdb_backtrace, gdb_print, gdb_examine, gdb_info_registers, gdb_command |
| `gdb-tools-glib.c` | gdb_glib_print_gobject, gdb_glib_print_glist, gdb_glib_print_ghash, gdb_glib_type_hierarchy, gdb_glib_signal_info |

## Adding a New Tool

1. Add handler function to appropriate `src/tools/gdb-tools-*.c`
2. Add schema function if needed
3. Declare in `src/tools/gdb-tools-internal.h`
4. Register in `src/gdb-mcp-server.c` in the appropriate `register_*_tools()` function
5. Document in `docs/tools-reference.md`

## Common Patterns

### Getting Session from Arguments

```c
GdbSession *session;
McpToolResult *error_result = NULL;

session = gdb_tools_get_session (manager, arguments, &error_result);
if (session == NULL)
{
    return error_result;  /* Already formatted error */
}
```

### Executing GDB Commands

```c
g_autofree gchar *output = NULL;
g_autoptr(GError) error = NULL;

output = gdb_tools_execute_command_sync (session, "backtrace", &error);
if (error != NULL)
{
    return gdb_tools_create_error_result ("Failed: %s", error->message);
}
```

### Creating Results

```c
/* Success result */
return gdb_tools_create_success_result ("Done: %s", output);

/* Error result */
return gdb_tools_create_error_result ("Missing parameter: %s", param_name);
```

## Testing

```bash
# Run the server
./build/gdb-mcp-server

# With custom GDB path
./build/gdb-mcp-server --gdb-path=/usr/bin/gdb-15
```

## Dependencies (Fedora)

```bash
sudo dnf install gcc make pkgconfig glib2-devel json-glib-devel libsoup3-devel libdex-devel gdb
```

## File Header Template

```c
/*
 * filename.c - Description
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
```
