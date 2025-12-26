# Using GDB MCP Server

## Quick Start

### 1. Start the Server

```bash
./build/gdb-mcp-server
```

The server runs on stdio and waits for MCP client connections.

### 2. Configure Your MCP Client

Add to your MCP client configuration (e.g., Claude Code):

```json
{
  "mcpServers": {
    "gdb": {
      "command": "/path/to/gdb-mcp-server"
    }
  }
}
```

## Command-Line Options

```
gdb-mcp-server [OPTIONS]
```

| Option | Description |
|--------|-------------|
| `--gdb-path=PATH` | Path to GDB binary (default: `gdb` from PATH) |
| `--version`, `-v` | Show version information |
| `--license`, `-l` | Show AGPLv3 license |
| `--help`, `-h` | Show usage help |

### Examples

```bash
# Use default GDB
./gdb-mcp-server

# Use specific GDB version
./gdb-mcp-server --gdb-path=/usr/bin/gdb-15

# Show version
./gdb-mcp-server --version
```

## Typical Debugging Workflow

### 1. Start a Session

```json
{
  "tool": "gdb_start",
  "arguments": {}
}
```

Returns a `sessionId` like `gdb-abc12345`.

### 2. Load Your Program

```json
{
  "tool": "gdb_load",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "program": "/path/to/myprogram",
    "arguments": ["--debug", "input.txt"]
  }
}
```

### 3. Set Breakpoints

```json
{
  "tool": "gdb_set_breakpoint",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "location": "main"
  }
}
```

### 4. Run the Program

```json
{
  "tool": "gdb_continue",
  "arguments": {
    "sessionId": "gdb-abc12345"
  }
}
```

### 5. Inspect State

When stopped at a breakpoint:

```json
{
  "tool": "gdb_backtrace",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "full": true
  }
}
```

```json
{
  "tool": "gdb_print",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "expression": "my_variable"
  }
}
```

### 6. Step Through Code

```json
{
  "tool": "gdb_step",
  "arguments": {
    "sessionId": "gdb-abc12345"
  }
}
```

### 7. End the Session

```json
{
  "tool": "gdb_terminate",
  "arguments": {
    "sessionId": "gdb-abc12345"
  }
}
```

## Debugging Core Dumps

```json
{
  "tool": "gdb_start",
  "arguments": {}
}
```

```json
{
  "tool": "gdb_load_core",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "program": "/path/to/crashed_program",
    "corePath": "/path/to/core.12345"
  }
}
```

The initial backtrace is included automatically.

## Attaching to Running Processes

```json
{
  "tool": "gdb_attach",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "pid": 12345
  }
}
```

**Note:** Requires appropriate permissions. On Linux, you may need to adjust ptrace settings:

```bash
# Temporarily allow ptrace
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope

# Or run as root
sudo ./gdb-mcp-server
```

## GLib Application Debugging

When debugging GLib/GObject applications, use the specialized tools:

### Inspect GObject Instances

```json
{
  "tool": "gdb_glib_print_gobject",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "expression": "self"
  }
}
```

### View Type Hierarchy

```json
{
  "tool": "gdb_glib_type_hierarchy",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "expression": "widget"
  }
}
```

### List Signals

```json
{
  "tool": "gdb_glib_signal_info",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "expression": "button"
  }
}
```

## Multiple Sessions

You can run multiple debugging sessions simultaneously:

```json
// Start session 1
{ "tool": "gdb_start", "arguments": {} }
// Returns: sessionId = "gdb-sess1"

// Start session 2
{ "tool": "gdb_start", "arguments": {} }
// Returns: sessionId = "gdb-sess2"

// List all sessions
{ "tool": "gdb_list_sessions", "arguments": {} }
```

Each session runs an independent GDB process.

## Advanced: Raw GDB Commands

For commands not covered by specific tools:

```json
{
  "tool": "gdb_command",
  "arguments": {
    "sessionId": "gdb-abc12345",
    "command": "info threads"
  }
}
```

## Troubleshooting

### Session Not Found

```
Error: No active GDB session with ID: gdb-abc12345
```

The session may have timed out or been terminated. Start a new session.

### Program Not Stopped

```
Error: Cannot inspect - program is running
```

Set a breakpoint and use `gdb_continue`, or use Ctrl+C equivalent.

### GDB Not Found

```
Error: Failed to start GDB
```

Ensure GDB is installed and in PATH, or use `--gdb-path` option.
