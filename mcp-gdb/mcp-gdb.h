/*
 * mcp-gdb.h - Main include header for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * GDB MCP Server - A Model Context Protocol server for GDB debugging.
 * Built on mcp-glib and GLib/GObject.
 */

#ifndef MCP_GDB_H
#define MCP_GDB_H

#define MCP_GDB_INSIDE

#include <mcp-gdb/gdb-enums.h>
#include <mcp-gdb/gdb-error.h>
#include <mcp-gdb/gdb-mi-parser.h>
#include <mcp-gdb/gdb-session.h>
#include <mcp-gdb/gdb-session-manager.h>
#include <mcp-gdb/gdb-mcp-server.h>

#undef MCP_GDB_INSIDE

/**
 * MCP_GDB_VERSION:
 *
 * The version of the mcp-gdb library.
 */
#define MCP_GDB_VERSION "1.0.0"

/**
 * MCP_GDB_VERSION_MAJOR:
 *
 * The major version component.
 */
#define MCP_GDB_VERSION_MAJOR 1

/**
 * MCP_GDB_VERSION_MINOR:
 *
 * The minor version component.
 */
#define MCP_GDB_VERSION_MINOR 0

/**
 * MCP_GDB_VERSION_MICRO:
 *
 * The micro version component.
 */
#define MCP_GDB_VERSION_MICRO 0

#endif /* MCP_GDB_H */
