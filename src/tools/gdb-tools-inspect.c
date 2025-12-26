/*
 * gdb-tools-inspect.c - Inspection and analysis tools for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Tools: gdb_backtrace, gdb_print, gdb_examine, gdb_info_registers, gdb_command
 */

#include "gdb-tools-internal.h"

/* ========================================================================== */
/* gdb_backtrace - Show call stack                                           */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_backtrace_schema (void)
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

    /* full (optional) */
    json_builder_set_member_name (builder, "full");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "boolean");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Show local variables in each frame (optional)");
    json_builder_end_object (builder);

    /* limit (optional) */
    json_builder_set_member_name (builder, "limit");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "integer");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Maximum number of frames to show (optional)");
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
gdb_tools_handle_gdb_backtrace (McpServer   *server G_GNUC_UNUSED,
                                const gchar *name G_GNUC_UNUSED,
                                JsonObject  *arguments,
                                gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    gboolean full = FALSE;
    gint64 limit = -1;
    GString *cmd;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get options */
    if (arguments != NULL)
    {
        if (json_object_has_member (arguments, "full"))
        {
            full = json_object_get_boolean_member (arguments, "full");
        }
        if (json_object_has_member (arguments, "limit"))
        {
            limit = json_object_get_int_member (arguments, "limit");
        }
    }

    /* Build command */
    cmd = g_string_new ("backtrace");
    if (full)
    {
        g_string_append (cmd, " full");
    }
    if (limit >= 0)
    {
        g_string_append_printf (cmd, " %ld", (long)limit);
    }

    output = gdb_tools_execute_command_sync (session, cmd->str, &error);
    g_string_free (cmd, TRUE);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to get backtrace: %s", error->message);
    }

    return gdb_tools_create_success_result (
        "Backtrace%s%s:\n\n%s",
        full ? " (full)" : "",
        limit >= 0 ? g_strdup_printf (" (limit: %ld)", (long)limit) : "",
        output);
}


/* ========================================================================== */
/* gdb_print - Print value of expression                                     */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_print_schema (void)
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

    /* expression */
    json_builder_set_member_name (builder, "expression");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Expression to evaluate");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_add_string_value (builder, "expression");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

McpToolResult *
gdb_tools_handle_gdb_print (McpServer   *server G_GNUC_UNUSED,
                            const gchar *name G_GNUC_UNUSED,
                            JsonObject  *arguments,
                            gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *expression;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *print_cmd = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get expression */
    if (!json_object_has_member (arguments, "expression"))
    {
        return gdb_tools_create_error_result ("Missing required parameter: expression");
    }
    expression = json_object_get_string_member (arguments, "expression");

    /* Execute print */
    print_cmd = g_strdup_printf ("print %s", expression);
    output = gdb_tools_execute_command_sync (session, print_cmd, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to print expression: %s", error->message);
    }

    return gdb_tools_create_success_result ("Print %s:\n\n%s", expression, output);
}


/* ========================================================================== */
/* gdb_examine - Examine memory                                              */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_examine_schema (void)
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

    /* expression */
    json_builder_set_member_name (builder, "expression");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Memory address or expression");
    json_builder_end_object (builder);

    /* format (optional) */
    json_builder_set_member_name (builder, "format");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Display format: x(hex), d(decimal), u(unsigned), o(octal), t(binary), a(address), c(char), f(float), s(string), i(instruction)");
    json_builder_end_object (builder);

    /* count (optional) */
    json_builder_set_member_name (builder, "count");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "integer");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Number of units to display (optional, default 1)");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_add_string_value (builder, "expression");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

McpToolResult *
gdb_tools_handle_gdb_examine (McpServer   *server G_GNUC_UNUSED,
                              const gchar *name G_GNUC_UNUSED,
                              JsonObject  *arguments,
                              gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *expression;
    const gchar *format = "x";
    gint64 count = 1;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *examine_cmd = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get expression */
    if (!json_object_has_member (arguments, "expression"))
    {
        return gdb_tools_create_error_result ("Missing required parameter: expression");
    }
    expression = json_object_get_string_member (arguments, "expression");

    /* Get optional format and count */
    if (json_object_has_member (arguments, "format"))
    {
        format = json_object_get_string_member (arguments, "format");
    }
    if (json_object_has_member (arguments, "count"))
    {
        count = json_object_get_int_member (arguments, "count");
    }

    /* Build examine command: x/[count][format] [expression] */
    examine_cmd = g_strdup_printf ("x/%ld%s %s", (long)count, format, expression);
    output = gdb_tools_execute_command_sync (session, examine_cmd, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to examine memory: %s", error->message);
    }

    return gdb_tools_create_success_result (
        "Examine %s (format: %s, count: %ld):\n\n%s",
        expression, format, (long)count, output);
}


/* ========================================================================== */
/* gdb_info_registers - Display CPU registers                                */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_info_registers_schema (void)
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

    /* register (optional) */
    json_builder_set_member_name (builder, "register");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Specific register name to display (optional, shows all if omitted)");
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
gdb_tools_handle_gdb_info_registers (McpServer   *server G_GNUC_UNUSED,
                                     const gchar *name G_GNUC_UNUSED,
                                     JsonObject  *arguments,
                                     gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *reg_name = NULL;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *info_cmd = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get optional register name */
    if (arguments != NULL && json_object_has_member (arguments, "register"))
    {
        reg_name = json_object_get_string_member (arguments, "register");
    }

    /* Build command */
    if (reg_name != NULL && reg_name[0] != '\0')
    {
        info_cmd = g_strdup_printf ("info registers %s", reg_name);
    }
    else
    {
        info_cmd = g_strdup ("info registers");
    }

    output = gdb_tools_execute_command_sync (session, info_cmd, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to get register info: %s", error->message);
    }

    return gdb_tools_create_success_result (
        "Register info%s%s:\n\n%s",
        reg_name ? " for " : "",
        reg_name ? reg_name : "",
        output);
}


/* ========================================================================== */
/* gdb_command - Execute arbitrary GDB command                               */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_command_schema (void)
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

    /* command */
    json_builder_set_member_name (builder, "command");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "GDB command to execute");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_add_string_value (builder, "command");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

McpToolResult *
gdb_tools_handle_gdb_command (McpServer   *server G_GNUC_UNUSED,
                              const gchar *name G_GNUC_UNUSED,
                              JsonObject  *arguments,
                              gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *command;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get command */
    if (!json_object_has_member (arguments, "command"))
    {
        return gdb_tools_create_error_result ("Missing required parameter: command");
    }
    command = json_object_get_string_member (arguments, "command");

    /* Execute command */
    output = gdb_tools_execute_command_sync (session, command, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to execute command: %s", error->message);
    }

    return gdb_tools_create_success_result ("Command: %s\n\nOutput:\n%s", command, output);
}
