# GDB MCP Server

A Model Context Protocol (MCP) server for debugging programs with GDB. Built with GLib/GObject and the [mcp-glib](https://gitlab.com/copyleft-games/mcp-glib) library.

## Features

- **Full GDB Integration** - Load programs, set breakpoints, step through code, inspect memory
- **Multiple Sessions** - Run concurrent debugging sessions
- **GLib/GObject Tools** - Specialized debugging for GLib applications
- **MI Protocol** - Uses GDB's Machine Interface for structured output
- **MCP Compatible** - Works with any MCP client (Claude Code, etc.)

## Quick Start

### Build

```bash
# Clone with submodules
git clone --recursive https://github.com/zachpodbielniak/mcp-gdb-glib.git
cd mcp-gdb-glib

# Build
make
```

### Run

```bash
./build/gdb-mcp-server
```

### Configure MCP Client

Add to your MCP configuration:

```json
{
  "mcpServers": {
    "gdb": {
      "command": "/path/to/gdb-mcp-server"
    }
  }
}
```

## Using with Claude Code

### Configuration

Add the GDB MCP server to your Claude Code settings. You can configure it globally or per-project.

**Global configuration** (`~/.claude/settings.json`):

```json
{
  "mcpServers": {
    "gdb": {
      "command": "/absolute/path/to/gdb-mcp-server",
      "args": []
    }
  }
}
```

**Project configuration** (`.claude/settings.json` in project root):

```json
{
  "mcpServers": {
    "gdb": {
      "command": "./build/gdb-mcp-server",
      "args": []
    }
  }
}
```

**With custom GDB path** (useful for specific GDB versions):

```json
{
  "mcpServers": {
    "gdb": {
      "command": "/path/to/gdb-mcp-server",
      "args": ["--gdb-path", "/usr/bin/gdb-15"]
    }
  }
}
```

### Building the Example

An example GLib/GObject application is provided for testing:

```bash
make examples
```

This builds `./build/gobject-demo` with full debug symbols.

### Quick Test

Once configured, ask Claude Code to debug the example:

> "Start a GDB session and load ./build/gobject-demo, set a breakpoint on main, and run it"

Or test individual tools:

> "Use gdb_start to create a new debugging session"

> "Use gdb_list_sessions to show all active sessions"

### Testing Workflow

See [docs/testing-workflow.md](docs/testing-workflow.md) for a comprehensive guide to testing all 21 tools.

## Available Tools

### Session Management
- `gdb_start` - Start a new GDB session
- `gdb_terminate` - End a session
- `gdb_list_sessions` - List active sessions

### Program Loading
- `gdb_load` - Load a program
- `gdb_attach` - Attach to a running process
- `gdb_load_core` - Load a core dump

### Execution Control
- `gdb_continue` - Continue execution
- `gdb_step` - Step into
- `gdb_next` - Step over
- `gdb_finish` - Run until return

### Breakpoints
- `gdb_set_breakpoint` - Set a breakpoint

### Inspection
- `gdb_backtrace` - Show call stack
- `gdb_print` - Evaluate expression
- `gdb_examine` - Examine memory
- `gdb_info_registers` - Show registers
- `gdb_command` - Raw GDB command

### GLib/GObject
- `gdb_glib_print_gobject` - Print GObject instance
- `gdb_glib_print_glist` - Print GList contents
- `gdb_glib_print_ghash` - Print GHashTable
- `gdb_glib_type_hierarchy` - Show type inheritance
- `gdb_glib_signal_info` - List signals

## Example Session

```
1. Start session:     gdb_start
2. Load program:      gdb_load(sessionId, "/path/to/program")
3. Set breakpoint:    gdb_set_breakpoint(sessionId, "main")
4. Run:               gdb_continue(sessionId)
5. Inspect:           gdb_backtrace(sessionId)
6. Step:              gdb_step(sessionId)
7. End:               gdb_terminate(sessionId)
```

## Dependencies

- GCC
- GLib 2.0, GObject 2.0, GIO 2.0
- json-glib 1.0
- GDB

### Fedora

```bash
sudo dnf install gcc make pkgconfig glib2-devel json-glib-devel libsoup3-devel libdex-devel gdb
```

### Ubuntu/Debian

```bash
sudo apt install build-essential pkg-config libglib2.0-dev libjson-glib-dev libsoup-3.0-dev gdb
```

Note: libdex may need to be built from source on Ubuntu.

## Documentation

- [Architecture](docs/architecture.md) - System design
- [Tools Reference](docs/tools-reference.md) - Complete tool documentation
- [Building](docs/building.md) - Build instructions
- [Usage](docs/usage.md) - How to use
- [MI Parser](docs/mi-parser.md) - GDB/MI protocol details
- [GLib Tools](docs/glib-tools.md) - GLib debugging guide

## Command-Line Options

```
gdb-mcp-server [OPTIONS]

Options:
  --gdb-path=PATH   Path to GDB binary (default: gdb)
  --version, -v     Show version
  --license, -l     Show license
  --help, -h        Show help
```

## License

Copyright (C) 2025 Zach Podbielniak

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See [LICENSE](LICENSE) for details.

## Related Projects

- [mcp-glib](https://gitlab.com/copyleft-games/mcp-glib) - GLib implementation of MCP
- [GDB](https://www.gnu.org/software/gdb/) - GNU Debugger
