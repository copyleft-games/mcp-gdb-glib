/*
 * gdb-tools-exec.c - Execution control tools for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Tools: gdb_continue, gdb_step, gdb_next, gdb_finish
 */

#include "gdb-tools-internal.h"

/* ========================================================================== */
/* gdb_continue - Continue program execution                                 */
/* ========================================================================== */

McpToolResult *
gdb_tools_handle_gdb_continue (McpServer   *server G_GNUC_UNUSED,
                               const gchar *name G_GNUC_UNUSED,
                               JsonObject  *arguments,
                               gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Execute continue */
    output = gdb_tools_execute_command_sync (session, "continue", &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to continue: %s", error->message);
    }

    return gdb_tools_create_success_result ("Continued execution\n\nOutput:\n%s", output);
}


/* ========================================================================== */
/* gdb_step - Step into functions                                            */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_step_schema (void)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();

    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "object");

    json_builder_set_member_name (builder, "properties");
    json_builder_begin_object (builder);

    /* sessionId */
    json_builder_set_member_name (builder, "sessionId");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "GDB session ID");
    json_builder_end_object (builder);

    /* instructions */
    json_builder_set_member_name (builder, "instructions");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "boolean");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Step by instructions instead of source lines (optional)");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

McpToolResult *
gdb_tools_handle_gdb_step (McpServer   *server G_GNUC_UNUSED,
                           const gchar *name G_GNUC_UNUSED,
                           JsonObject  *arguments,
                           gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    gboolean instructions = FALSE;
    const gchar *command;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Check for instruction-level stepping */
    if (arguments != NULL && json_object_has_member (arguments, "instructions"))
    {
        instructions = json_object_get_boolean_member (arguments, "instructions");
    }

    command = instructions ? "stepi" : "step";
    output = gdb_tools_execute_command_sync (session, command, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to step: %s", error->message);
    }

    return gdb_tools_create_success_result (
        "Stepped %s\n\nOutput:\n%s",
        instructions ? "instruction" : "line",
        output);
}


/* ========================================================================== */
/* gdb_next - Step over function calls                                       */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_next_schema (void)
{
    /* Same schema as step */
    return gdb_tools_create_gdb_step_schema ();
}

McpToolResult *
gdb_tools_handle_gdb_next (McpServer   *server G_GNUC_UNUSED,
                           const gchar *name G_GNUC_UNUSED,
                           JsonObject  *arguments,
                           gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    gboolean instructions = FALSE;
    const gchar *command;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Check for instruction-level stepping */
    if (arguments != NULL && json_object_has_member (arguments, "instructions"))
    {
        instructions = json_object_get_boolean_member (arguments, "instructions");
    }

    command = instructions ? "nexti" : "next";
    output = gdb_tools_execute_command_sync (session, command, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to step over: %s", error->message);
    }

    return gdb_tools_create_success_result (
        "Stepped over %s\n\nOutput:\n%s",
        instructions ? "instruction" : "function call",
        output);
}


/* ========================================================================== */
/* gdb_finish - Execute until current function returns                       */
/* ========================================================================== */

McpToolResult *
gdb_tools_handle_gdb_finish (McpServer   *server G_GNUC_UNUSED,
                             const gchar *name G_GNUC_UNUSED,
                             JsonObject  *arguments,
                             gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Execute finish */
    output = gdb_tools_execute_command_sync (session, "finish", &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to finish: %s", error->message);
    }

    return gdb_tools_create_success_result ("Finished current function\n\nOutput:\n%s", output);
}
