# Building GDB MCP Server

## Dependencies

### Fedora (43+)

```bash
sudo dnf install gcc make pkgconfig glib2-devel json-glib-devel libsoup3-devel libdex-devel gdb
```

### Ubuntu/Debian

```bash
sudo apt install build-essential pkg-config libglib2.0-dev libjson-glib-dev libsoup-3.0-dev gdb
```

Note: libdex may need to be built from source on Ubuntu.

### Arch Linux

```bash
sudo pacman -S base-devel glib2 json-glib libsoup3 gdb
```

Note: libdex may need to be installed from AUR.

## Getting the Source

```bash
git clone https://github.com/zachpodbielniak/mcp-gdb-glib.git
cd mcp-gdb-glib

# Initialize submodules (for mcp-glib)
git submodule update --init --recursive
```

## Building

### Quick Build

```bash
make
```

This will:
1. Build the mcp-glib library (if not already built)
2. Compile the gdb-mcp-server

### Build Output

The compiled binary is at:
```
build/gdb-mcp-server
```

### Check Dependencies

```bash
make check-deps
```

### Clean Build

```bash
# Clean gdb-mcp-server only
make clean

# Clean everything including mcp-glib
make distclean
```

## Installation

### System-wide

```bash
sudo make install
```

Installs to `/usr/local/bin/gdb-mcp-server`.

### Uninstall

```bash
sudo make uninstall
```

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make` | Build the server |
| `make clean` | Remove build artifacts |
| `make distclean` | Clean everything including mcp-glib |
| `make run` | Build and run the server |
| `make install` | Install to system |
| `make uninstall` | Remove from system |
| `make check-deps` | Verify dependencies |
| `make help` | Show help message |

## Compiler Flags

The Makefile uses:

- `-std=gnu89` - GNU C89 standard
- `-Wall -Wextra -Werror` - Strict warnings
- `-g` - Debug symbols
- `-O2` - Optimization level 2

## Troubleshooting

### Missing pkg-config dependencies

```
Package glib-2.0 was not found
```

Install the development packages:
```bash
# Fedora
sudo dnf install glib2-devel json-glib-devel

# Ubuntu
sudo apt install libglib2.0-dev libjson-glib-dev
```

### mcp-glib not found

```
mcp-glib not found. Run: git submodule update --init
```

Initialize the git submodule:
```bash
git submodule update --init --recursive
```

### Library not found at runtime

```
error while loading shared libraries: libmcp-1.0.so
```

The Makefile sets rpath for the build directory. If you move the binary, you may need to:

1. Install mcp-glib system-wide, or
2. Set `LD_LIBRARY_PATH`:
   ```bash
   export LD_LIBRARY_PATH=/path/to/mcp-glib/build:$LD_LIBRARY_PATH
   ```

## Development

### Rebuilding after changes

```bash
make
```

The Makefile tracks header dependencies automatically.

### Adding new source files

Edit the `SRCS` variable in `Makefile` to include new files:

```makefile
SRCS := \
    $(SRCDIR)/main.c \
    $(SRCDIR)/new-file.c \
    ...
```
