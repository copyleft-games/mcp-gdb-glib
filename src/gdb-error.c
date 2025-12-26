/*
 * gdb-error.c - Error domain and codes implementation for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "mcp-gdb/gdb-error.h"

GQuark
gdb_error_quark (void)
{
    return g_quark_from_static_string ("gdb-error-quark");
}

const gchar *
gdb_error_code_to_string (GdbErrorCode code)
{
    switch (code)
    {
        case GDB_ERROR_SESSION_NOT_FOUND:
            return "Session not found";
        case GDB_ERROR_SESSION_NOT_READY:
            return "Session not ready for commands";
        case GDB_ERROR_SESSION_LIMIT:
            return "Maximum session count reached";
        case GDB_ERROR_SPAWN_FAILED:
            return "Failed to spawn GDB process";
        case GDB_ERROR_TIMEOUT:
            return "Command timed out";
        case GDB_ERROR_COMMAND_FAILED:
            return "GDB command failed";
        case GDB_ERROR_PARSE_ERROR:
            return "Failed to parse MI output";
        case GDB_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";
        case GDB_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case GDB_ERROR_ATTACH_FAILED:
            return "Failed to attach to process";
        case GDB_ERROR_ALREADY_RUNNING:
            return "Session already has a running program";
        case GDB_ERROR_NOT_RUNNING:
            return "No program is running";
        case GDB_ERROR_INTERNAL:
            return "Internal error";
        default:
            return "Unknown error";
    }
}
