# GDB MCP Server Architecture

## Overview

The GDB MCP Server is built using GLib/GObject patterns and the mcp-glib library. It provides a Model Context Protocol (MCP) interface for debugging programs with GDB.

## Class Hierarchy

```
GObject
├── GdbMcpServer              /* Main application, owns McpServer */
├── GdbSessionManager         /* Manages multiple concurrent sessions */
├── GdbSession                /* Single GDB subprocess lifecycle */
└── GdbMiParser               /* Parses GDB/MI structured output */

Boxed Types (Reference Counted)
└── GdbMiRecord               /* Single MI output record */
```

## Components

### GdbMcpServer

The main server class that:

- Owns the McpServer instance from mcp-glib
- Registers all GDB debugging tools
- Manages the GdbSessionManager
- Handles server lifecycle (run/stop)

**Properties:**
- `name` - Server name (construct-only)
- `version` - Server version (construct-only)
- `default-gdb-path` - Default GDB binary path
- `session-manager` - The GdbSessionManager (read-only)

### GdbSessionManager

Manages multiple concurrent GDB sessions:

- Thread-safe session storage using GMutex
- Creates and tracks sessions by unique ID
- Provides singleton access via `gdb_session_manager_get_default()`

**Properties:**
- `default-gdb-path` - Default GDB path for new sessions
- `default-timeout-ms` - Default command timeout
- `session-count` - Number of active sessions (read-only)

**Signals:**
- `session-added` - Emitted when a new session is created
- `session-removed` - Emitted when a session is terminated

### GdbSession

Manages a single GDB subprocess:

- Spawns GDB as a subprocess using GSubprocess
- Communicates via stdin/stdout pipes
- Parses output using GdbMiParser
- Handles async command execution with GTask

**Properties:**
- `session-id` - Unique session identifier (construct-only)
- `gdb-path` - Path to GDB binary
- `working-dir` - Working directory for GDB
- `target-program` - Currently loaded program (read-only)
- `state` - Current session state (read-only)
- `timeout-ms` - Command timeout in milliseconds

**Signals:**
- `state-changed` - Emitted when state changes
- `stopped` - Emitted when program stops (breakpoint, signal, etc.)
- `console-output` - Emitted for GDB console output
- `ready` - Emitted when session is ready for commands
- `terminated` - Emitted when session terminates

**States (GdbSessionState):**
- `DISCONNECTED` - Not started
- `STARTING` - Starting GDB subprocess
- `READY` - Ready for commands
- `RUNNING` - Program is running
- `STOPPED` - Program stopped (breakpoint, etc.)
- `TERMINATED` - Session ended
- `ERROR` - Error state

### GdbMiParser

Parses GDB Machine Interface (MI) output:

- Tokenizes MI output lines
- Extracts structured data into JsonNode
- Handles all MI record types (result, async, stream)

**Record Types (GdbMiRecordType):**
- `RESULT` - Synchronous command results (^done, ^error)
- `EXEC_ASYNC` - Execution state changes (*stopped, *running)
- `STATUS_ASYNC` - Status notifications (+)
- `NOTIFY_ASYNC` - Async notifications (=)
- `CONSOLE` - Console stream output (~)
- `TARGET` - Target output (@)
- `LOG` - Log output (&)

### GdbMiRecord

Boxed type containing parsed MI record data:

- Reference counted for memory management
- Contains record type, result class, and data

## Tool Organization

Tools are organized by category in separate files:

```
src/tools/
├── gdb-tools-internal.h    # Internal header with helpers
├── gdb-tools-common.c      # Shared utility functions
├── gdb-tools-session.c     # Session management tools
├── gdb-tools-load.c        # Program loading tools
├── gdb-tools-exec.c        # Execution control tools
├── gdb-tools-breakpoint.c  # Breakpoint tools
├── gdb-tools-inspect.c     # Inspection tools
└── gdb-tools-glib.c        # GLib-specific tools
```

## Data Flow

1. MCP client sends tool request
2. McpServer dispatches to registered handler
3. Handler retrieves GdbSession from GdbSessionManager
4. Handler sends GDB command via GdbSession
5. GdbSession sends to GDB subprocess
6. GdbMiParser parses response
7. Handler formats result as McpToolResult
8. Result returned to MCP client

## Memory Management

The codebase uses GLib memory management patterns:

- `g_autoptr()` - Automatic cleanup at scope exit
- `g_autofree` - Automatic string cleanup
- `g_steal_pointer()` - Ownership transfer
- Reference counting for boxed types

## Thread Safety

- GdbSessionManager uses GMutex for thread-safe session access
- Tool handlers are designed to be thread-safe
- GMainLoop handles async operations

## Error Handling

Errors use the GError pattern with GDB_ERROR domain:

- `GDB_ERROR_SESSION_NOT_FOUND` - Invalid session ID
- `GDB_ERROR_SESSION_NOT_READY` - Session not in ready state
- `GDB_ERROR_SPAWN_FAILED` - Failed to start GDB
- `GDB_ERROR_TIMEOUT` - Command timeout
- `GDB_ERROR_COMMAND_FAILED` - GDB command failed
- `GDB_ERROR_PARSE_ERROR` - MI parsing error
- `GDB_ERROR_INVALID_ARGUMENT` - Invalid argument
- `GDB_ERROR_ATTACH_FAILED` - Failed to attach to process
