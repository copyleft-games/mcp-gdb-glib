# Makefile for gdb-mcp-server
#
# Copyright (C) 2025 Zach Podbielniak
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# GDB MCP Server - A Model Context Protocol server for GDB debugging.

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -std=gnu89 -g -O2
LDFLAGS :=

# Directories
SRCDIR := src
TOOLSDIR := src/tools
BUILDDIR := build
INCDIR := .
MCP_GLIB_DIR := deps/mcp-glib

# Binary name
TARGET := $(BUILDDIR)/gdb-mcp-server

# pkg-config dependencies
PKGS := glib-2.0 gobject-2.0 gio-2.0 gio-unix-2.0 json-glib-1.0 libsoup-3.0 libdex-1

# Get pkg-config flags
PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS := $(shell pkg-config --libs $(PKGS))

# mcp-glib paths
MCP_GLIB_CFLAGS := -I$(MCP_GLIB_DIR)
MCP_GLIB_LIBS := -L$(MCP_GLIB_DIR)/build -lmcp-glib-1.0 -Wl,-rpath,$(CURDIR)/$(MCP_GLIB_DIR)/build

# Include paths
INCLUDES := -I$(INCDIR) $(MCP_GLIB_CFLAGS) $(PKG_CFLAGS)

# Libraries
LIBS := $(MCP_GLIB_LIBS) $(PKG_LIBS)

# Source files
SRCS := \
	$(SRCDIR)/main.c \
	$(SRCDIR)/gdb-mcp-server.c \
	$(SRCDIR)/gdb-enums.c \
	$(SRCDIR)/gdb-error.c \
	$(SRCDIR)/gdb-mi-parser.c \
	$(SRCDIR)/gdb-session.c \
	$(SRCDIR)/gdb-session-manager.c \
	$(TOOLSDIR)/gdb-tools-common.c \
	$(TOOLSDIR)/gdb-tools-session.c \
	$(TOOLSDIR)/gdb-tools-load.c \
	$(TOOLSDIR)/gdb-tools-exec.c \
	$(TOOLSDIR)/gdb-tools-breakpoint.c \
	$(TOOLSDIR)/gdb-tools-inspect.c \
	$(TOOLSDIR)/gdb-tools-glib.c

# Object files
OBJS := $(SRCS:%.c=$(BUILDDIR)/%.o)

# Header files for dependency tracking
HEADERS := $(wildcard mcp-gdb/*.h) $(wildcard $(TOOLSDIR)/*.h)

# Default target
.PHONY: all
all: mcp-glib $(TARGET)

# Build mcp-glib first
.PHONY: mcp-glib
mcp-glib:
	@echo "Building mcp-glib..."
	@$(MAKE) -C $(MCP_GLIB_DIR) --no-print-directory

# Create build directories
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(SRCDIR):
	@mkdir -p $(BUILDDIR)/$(SRCDIR)

$(BUILDDIR)/$(TOOLSDIR):
	@mkdir -p $(BUILDDIR)/$(TOOLSDIR)

# Compile source files
$(BUILDDIR)/%.o: %.c $(HEADERS) | $(BUILDDIR)/$(SRCDIR) $(BUILDDIR)/$(TOOLSDIR)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link the executable
$(TARGET): $(OBJS)
	@echo "  LINK  $@"
	@$(CC) $(LDFLAGS) $^ $(LIBS) -o $@
	@echo ""
	@echo "Build complete: $@"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILDDIR)

# Clean everything including mcp-glib
.PHONY: distclean
distclean: clean
	@echo "Cleaning mcp-glib..."
	@$(MAKE) -C $(MCP_GLIB_DIR) clean --no-print-directory

# Run the server
.PHONY: run
run: all
	@echo "Running gdb-mcp-server..."
	@./$(TARGET)

# Show help
.PHONY: help
help:
	@echo "GDB MCP Server - Build Targets"
	@echo ""
	@echo "Build:"
	@echo "  make           - Build the server (and mcp-glib if needed)"
	@echo "  make examples  - Build example applications"
	@echo "  make test      - Run unit tests"
	@echo "  make clean     - Remove build artifacts"
	@echo "  make distclean - Remove all build artifacts including mcp-glib"
	@echo ""
	@echo "Claude Code:"
	@echo "  make mcp-add   - Add MCP server to project's Claude Code config"
	@echo ""
	@echo "Run:"
	@echo "  make run       - Build and run the server"
	@echo ""
	@echo "Dependencies (Fedora):"
	@echo "  sudo dnf install gcc make pkgconfig glib2-devel json-glib-devel"
	@echo ""
	@echo "Usage:"
	@echo "  $(TARGET) [OPTIONS]"
	@echo ""
	@echo "Options:"
	@echo "  --gdb-path=PATH  Path to GDB binary (default: gdb from PATH)"
	@echo "  --version        Show version information"
	@echo "  --license        Show license information"
	@echo "  --help           Show usage help"

# Install target (optional)
.PHONY: install
install: all
	@echo "Installing gdb-mcp-server..."
	@install -d $(DESTDIR)/usr/local/bin
	@install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/
	@echo "Installed to $(DESTDIR)/usr/local/bin/gdb-mcp-server"

# Uninstall target
.PHONY: uninstall
uninstall:
	@echo "Uninstalling gdb-mcp-server..."
	@rm -f $(DESTDIR)/usr/local/bin/gdb-mcp-server

# Check for required dependencies
.PHONY: check-deps
check-deps:
	@echo "Checking dependencies..."
	@pkg-config --exists $(PKGS) && echo "All pkg-config dependencies found" || \
		(echo "Missing dependencies. Install with:" && \
		 echo "  sudo dnf install glib2-devel json-glib-devel" && exit 1)
	@test -d $(MCP_GLIB_DIR) && echo "mcp-glib submodule found" || \
		(echo "mcp-glib not found. Run: git submodule update --init" && exit 1)

# ============================================================================
# Test targets
# ============================================================================

TESTDIR := tests
TEST_SRCS := $(wildcard $(TESTDIR)/test-*.c)
TEST_BINS := $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/%,$(TEST_SRCS))

# Object files without main.o for tests
TEST_OBJS := $(filter-out $(BUILDDIR)/$(SRCDIR)/main.o,$(OBJS))

# Test program for integration tests
TEST_PROGRAM := $(BUILDDIR)/test-program

# Build and run all tests
.PHONY: test
test: $(TARGET) $(TEST_BINS) $(TEST_PROGRAM)
	@echo ""
	@echo "Running tests..."
	@echo "==============="
	@passed=0; failed=0; \
	for t in $(TEST_BINS); do \
		name=$$(basename $$t); \
		echo -n "  $$name: "; \
		if $$t > /dev/null 2>&1; then \
			echo "PASSED"; \
			passed=$$((passed + 1)); \
		else \
			echo "FAILED"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Results: $$passed passed, $$failed failed"; \
	if [ $$failed -gt 0 ]; then exit 1; fi
	@echo ""
	@echo "All tests passed."

# Build tests only (don't run)
.PHONY: test-build
test-build: $(TEST_BINS) $(TEST_PROGRAM)

# Run tests with verbose output
.PHONY: test-verbose
test-verbose: $(TARGET) $(TEST_BINS) $(TEST_PROGRAM)
	@echo ""
	@echo "Running tests (verbose)..."
	@echo "=========================="
	@for t in $(TEST_BINS); do \
		name=$$(basename $$t); \
		echo ""; \
		echo "=== $$name ==="; \
		G_TEST_VERBOSE=1 $$t || exit 1; \
	done
	@echo ""
	@echo "All tests passed."

# Build individual test binaries
$(BUILDDIR)/test-%: $(TESTDIR)/test-%.c $(TEST_OBJS) | $(BUILDDIR)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(TEST_OBJS) $(LIBS)

# Build test program for integration tests
$(TEST_PROGRAM): $(TESTDIR)/test-program.c | $(BUILDDIR)
	@echo "  CC    $< (test program)"
	@$(CC) -g -O0 -o $@ $<

# Clean test binaries
.PHONY: test-clean
test-clean:
	@echo "Cleaning test binaries..."
	@rm -f $(TEST_BINS) $(TEST_PROGRAM)

# ============================================================================
# Example targets
# ============================================================================

EXAMPLEDIR := examples
EXAMPLE_GOBJECT := $(BUILDDIR)/gobject-demo

# Build all examples
.PHONY: examples
examples: $(EXAMPLE_GOBJECT)
	@echo ""
	@echo "Examples built successfully."
	@echo "Run with: ./$(EXAMPLE_GOBJECT)"

# Build gobject-demo example with debug symbols
$(EXAMPLE_GOBJECT): $(EXAMPLEDIR)/gobject-demo.c | $(BUILDDIR)
	@echo "  CC    $< (example with debug symbols)"
	@$(CC) -g -O0 $(PKG_CFLAGS) -o $@ $< $(PKG_LIBS)

# Clean examples
.PHONY: examples-clean
examples-clean:
	@echo "Cleaning examples..."
	@rm -f $(EXAMPLE_GOBJECT)

# ============================================================================
# Claude Code MCP configuration
# ============================================================================

# Add GDB MCP server to project's Claude Code config
.PHONY: mcp-add
mcp-add: $(TARGET)
	claude mcp add gdb $(CURDIR)/$(TARGET) -s project
