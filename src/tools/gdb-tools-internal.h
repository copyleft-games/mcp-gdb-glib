/*
 * gdb-tools-internal.h - Internal header for GDB MCP tools
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef GDB_TOOLS_INTERNAL_H
#define GDB_TOOLS_INTERNAL_H

#include <mcp.h>
#include <json-glib/json-glib.h>

#include "mcp-gdb/gdb-session.h"
#include "mcp-gdb/gdb-session-manager.h"
#include "mcp-gdb/gdb-enums.h"
#include "mcp-gdb/gdb-error.h"

G_BEGIN_DECLS

/* ========================================================================== */
/* Common Helper Functions                                                    */
/* ========================================================================== */

/**
 * gdb_tools_create_error_result:
 * @format: printf-style format string
 * @...: format arguments
 *
 * Creates an error result with formatted message.
 *
 * Returns: (transfer full): the error result
 */
static inline McpToolResult *
gdb_tools_create_error_result (const gchar *format, ...)
{
    McpToolResult *result;
    g_autofree gchar *message = NULL;
    va_list args;

    va_start (args, format);
    message = g_strdup_vprintf (format, args);
    va_end (args);

    result = mcp_tool_result_new (TRUE);
    mcp_tool_result_add_text (result, message);

    return result;
}

/**
 * gdb_tools_create_success_result:
 * @format: printf-style format string
 * @...: format arguments
 *
 * Creates a success result with formatted message.
 *
 * Returns: (transfer full): the success result
 */
static inline McpToolResult *
gdb_tools_create_success_result (const gchar *format, ...)
{
    McpToolResult *result;
    g_autofree gchar *message = NULL;
    va_list args;

    va_start (args, format);
    message = g_strdup_vprintf (format, args);
    va_end (args);

    result = mcp_tool_result_new (FALSE);
    mcp_tool_result_add_text (result, message);

    return result;
}

/**
 * gdb_tools_get_session_id:
 * @arguments: the tool arguments
 *
 * Extracts the sessionId from arguments.
 *
 * Returns: (transfer none) (nullable): the session ID, or %NULL
 */
static inline const gchar *
gdb_tools_get_session_id (JsonObject *arguments)
{
    if (arguments == NULL)
    {
        return NULL;
    }

    if (!json_object_has_member (arguments, "sessionId"))
    {
        return NULL;
    }

    return json_object_get_string_member (arguments, "sessionId");
}

/**
 * gdb_tools_get_session:
 * @manager: the session manager
 * @arguments: the tool arguments
 * @error_result: (out) (optional): location for error result
 *
 * Gets a session from arguments. If session not found, creates error result.
 *
 * Returns: (transfer none) (nullable): the session, or %NULL with error result
 */
static inline GdbSession *
gdb_tools_get_session (GdbSessionManager  *manager,
                       JsonObject         *arguments,
                       McpToolResult     **error_result)
{
    const gchar *session_id;
    GdbSession *session;

    session_id = gdb_tools_get_session_id (arguments);
    if (session_id == NULL)
    {
        if (error_result != NULL)
        {
            *error_result = gdb_tools_create_error_result ("Missing required parameter: sessionId");
        }
        return NULL;
    }

    session = gdb_session_manager_get_session (manager, session_id);
    if (session == NULL)
    {
        if (error_result != NULL)
        {
            *error_result = gdb_tools_create_error_result ("No active GDB session with ID: %s", session_id);
        }
        return NULL;
    }

    return session;
}

/**
 * gdb_tools_execute_command_sync:
 * @session: the GDB session
 * @command: the command to execute
 * @error: (out) (optional): return location for error
 *
 * Executes a GDB command synchronously.
 *
 * Returns: (transfer full) (nullable): the output, or %NULL on error
 */
gchar *gdb_tools_execute_command_sync (GdbSession  *session,
                                       const gchar *command,
                                       GError     **error);


/* ========================================================================== */
/* Schema Creation Functions                                                  */
/* ========================================================================== */

/* Session tools schemas */
JsonNode *gdb_tools_create_gdb_start_schema       (void);
JsonNode *gdb_tools_create_session_id_only_schema (void);

/* Load tools schemas */
JsonNode *gdb_tools_create_gdb_load_schema        (void);
JsonNode *gdb_tools_create_gdb_attach_schema      (void);
JsonNode *gdb_tools_create_gdb_load_core_schema   (void);

/* Execution tools schemas */
JsonNode *gdb_tools_create_gdb_step_schema        (void);
JsonNode *gdb_tools_create_gdb_next_schema        (void);

/* Breakpoint tools schemas */
JsonNode *gdb_tools_create_gdb_breakpoint_schema  (void);

/* Inspection tools schemas */
JsonNode *gdb_tools_create_gdb_backtrace_schema      (void);
JsonNode *gdb_tools_create_gdb_print_schema          (void);
JsonNode *gdb_tools_create_gdb_examine_schema        (void);
JsonNode *gdb_tools_create_gdb_info_registers_schema (void);
JsonNode *gdb_tools_create_gdb_command_schema        (void);

/* GLib tools schemas */
JsonNode *gdb_tools_create_gdb_glib_print_gobject_schema  (void);
JsonNode *gdb_tools_create_gdb_glib_print_glist_schema    (void);
JsonNode *gdb_tools_create_gdb_glib_print_ghash_schema    (void);
JsonNode *gdb_tools_create_gdb_glib_type_hierarchy_schema (void);
JsonNode *gdb_tools_create_gdb_glib_signal_info_schema    (void);


/* ========================================================================== */
/* Tool Handler Functions                                                     */
/* ========================================================================== */

/* Session tools */
McpToolResult *gdb_tools_handle_gdb_start        (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_terminate    (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_list_sessions(McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);

/* Load tools */
McpToolResult *gdb_tools_handle_gdb_load         (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_attach       (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_load_core    (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);

/* Execution tools */
McpToolResult *gdb_tools_handle_gdb_continue     (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_step         (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_next         (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_finish       (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);

/* Breakpoint tools */
McpToolResult *gdb_tools_handle_gdb_set_breakpoint (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);

/* Inspection tools */
McpToolResult *gdb_tools_handle_gdb_backtrace    (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_print        (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_examine      (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_info_registers(McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_command      (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);

/* GLib-specific tools */
McpToolResult *gdb_tools_handle_gdb_glib_print_gobject   (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_glib_print_glist     (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_glib_print_ghash     (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_glib_type_hierarchy  (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);
McpToolResult *gdb_tools_handle_gdb_glib_signal_info     (McpServer *server, const gchar *name, JsonObject *arguments, gpointer user_data);

G_END_DECLS

#endif /* GDB_TOOLS_INTERNAL_H */
