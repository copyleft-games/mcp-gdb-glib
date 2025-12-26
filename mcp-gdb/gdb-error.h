/*
 * gdb-error.h - Error domain and codes for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef GDB_ERROR_H
#define GDB_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * GDB_ERROR:
 *
 * Error domain for GDB MCP server errors.
 * Errors in this domain will be from the #GdbErrorCode enumeration.
 * Use g_error_matches() to check for specific error codes.
 */
#define GDB_ERROR (gdb_error_quark ())

/**
 * gdb_error_quark:
 *
 * Gets the error quark for GDB MCP server errors.
 *
 * Returns: the error quark
 */
GQuark gdb_error_quark (void);

/**
 * GdbErrorCode:
 * @GDB_ERROR_SESSION_NOT_FOUND: Session ID not found
 * @GDB_ERROR_SESSION_NOT_READY: Session not ready for commands
 * @GDB_ERROR_SESSION_LIMIT: Maximum session count reached
 * @GDB_ERROR_SPAWN_FAILED: Failed to spawn GDB process
 * @GDB_ERROR_TIMEOUT: Command timed out
 * @GDB_ERROR_COMMAND_FAILED: GDB command returned error
 * @GDB_ERROR_PARSE_ERROR: Failed to parse MI output
 * @GDB_ERROR_INVALID_ARGUMENT: Invalid argument provided
 * @GDB_ERROR_FILE_NOT_FOUND: File not found
 * @GDB_ERROR_ATTACH_FAILED: Failed to attach to process
 * @GDB_ERROR_ALREADY_RUNNING: Session already has a running process
 * @GDB_ERROR_NOT_RUNNING: No program is running
 * @GDB_ERROR_INTERNAL: Internal error
 *
 * Error codes for GDB MCP server operations.
 */
typedef enum {
    GDB_ERROR_SESSION_NOT_FOUND,
    GDB_ERROR_SESSION_NOT_READY,
    GDB_ERROR_SESSION_LIMIT,
    GDB_ERROR_SPAWN_FAILED,
    GDB_ERROR_TIMEOUT,
    GDB_ERROR_COMMAND_FAILED,
    GDB_ERROR_PARSE_ERROR,
    GDB_ERROR_INVALID_ARGUMENT,
    GDB_ERROR_FILE_NOT_FOUND,
    GDB_ERROR_ATTACH_FAILED,
    GDB_ERROR_ALREADY_RUNNING,
    GDB_ERROR_NOT_RUNNING,
    GDB_ERROR_INTERNAL
} GdbErrorCode;

/**
 * gdb_error_code_to_string:
 * @code: a #GdbErrorCode
 *
 * Converts an error code to a human-readable string.
 *
 * Returns: (transfer none): the error description
 */
const gchar *gdb_error_code_to_string (GdbErrorCode code);

G_END_DECLS

#endif /* GDB_ERROR_H */
