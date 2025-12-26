/*
 * gdb-tools-load.c - Program loading tools for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Tools: gdb_load, gdb_attach, gdb_load_core
 */

#include "gdb-tools-internal.h"

/* ========================================================================== */
/* gdb_load - Load a program into GDB                                        */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_load_schema (void)
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

    /* program */
    json_builder_set_member_name (builder, "program");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Path to the program to debug");
    json_builder_end_object (builder);

    /* arguments (array of strings) */
    json_builder_set_member_name (builder, "arguments");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "array");
    json_builder_set_member_name (builder, "items");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_end_object (builder);
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Command-line arguments for the program (optional)");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_add_string_value (builder, "program");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

McpToolResult *
gdb_tools_handle_gdb_load (McpServer   *server G_GNUC_UNUSED,
                           const gchar *name G_GNUC_UNUSED,
                           JsonObject  *arguments,
                           gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *program;
    g_autofree gchar *output = NULL;
    g_autofree gchar *args_output = NULL;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *load_cmd = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get program path */
    if (!json_object_has_member (arguments, "program"))
    {
        return gdb_tools_create_error_result ("Missing required parameter: program");
    }
    program = json_object_get_string_member (arguments, "program");

    /* Load program */
    load_cmd = g_strdup_printf ("file \"%s\"", program);
    output = gdb_tools_execute_command_sync (session, load_cmd, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to load program: %s", error->message);
    }

    /* Update session target */
    gdb_session_set_target_program (session, program);

    /* Set arguments if provided */
    if (json_object_has_member (arguments, "arguments"))
    {
        JsonArray *args_array = json_object_get_array_member (arguments, "arguments");
        guint len = json_array_get_length (args_array);

        if (len > 0)
        {
            GString *args_str = g_string_new ("set args");
            guint i;

            for (i = 0; i < len; i++)
            {
                const gchar *arg = json_array_get_string_element (args_array, i);
                g_string_append_printf (args_str, " %s", arg);
            }

            args_output = gdb_tools_execute_command_sync (session, args_str->str, NULL);
            g_string_free (args_str, TRUE);
        }
    }

    return gdb_tools_create_success_result (
        "Program loaded: %s\n\nOutput:\n%s%s%s",
        program,
        output,
        args_output ? "\n" : "",
        args_output ? args_output : "");
}


/* ========================================================================== */
/* gdb_attach - Attach to a running process                                  */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_attach_schema (void)
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

    /* pid */
    json_builder_set_member_name (builder, "pid");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "integer");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Process ID to attach to");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_add_string_value (builder, "pid");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

McpToolResult *
gdb_tools_handle_gdb_attach (McpServer   *server G_GNUC_UNUSED,
                             const gchar *name G_GNUC_UNUSED,
                             JsonObject  *arguments,
                             gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    gint64 pid;
    g_autofree gchar *output = NULL;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *attach_cmd = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get PID */
    if (!json_object_has_member (arguments, "pid"))
    {
        return gdb_tools_create_error_result ("Missing required parameter: pid");
    }
    pid = json_object_get_int_member (arguments, "pid");

    /* Attach to process */
    attach_cmd = g_strdup_printf ("attach %ld", (long)pid);
    output = gdb_tools_execute_command_sync (session, attach_cmd, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to attach to process: %s", error->message);
    }

    return gdb_tools_create_success_result (
        "Attached to process %ld\n\nOutput:\n%s",
        (long)pid, output);
}


/* ========================================================================== */
/* gdb_load_core - Load a core dump file                                     */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_load_core_schema (void)
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

    /* program */
    json_builder_set_member_name (builder, "program");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Path to the program executable");
    json_builder_end_object (builder);

    /* corePath */
    json_builder_set_member_name (builder, "corePath");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Path to the core dump file");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_add_string_value (builder, "program");
    json_builder_add_string_value (builder, "corePath");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

McpToolResult *
gdb_tools_handle_gdb_load_core (McpServer   *server G_GNUC_UNUSED,
                                const gchar *name G_GNUC_UNUSED,
                                JsonObject  *arguments,
                                gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *program;
    const gchar *core_path;
    g_autofree gchar *file_output = NULL;
    g_autofree gchar *core_output = NULL;
    g_autofree gchar *bt_output = NULL;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *file_cmd = NULL;
    g_autofree gchar *core_cmd = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get program path */
    if (!json_object_has_member (arguments, "program"))
    {
        return gdb_tools_create_error_result ("Missing required parameter: program");
    }
    program = json_object_get_string_member (arguments, "program");

    /* Get core path */
    if (!json_object_has_member (arguments, "corePath"))
    {
        return gdb_tools_create_error_result ("Missing required parameter: corePath");
    }
    core_path = json_object_get_string_member (arguments, "corePath");

    /* Load program first */
    file_cmd = g_strdup_printf ("file \"%s\"", program);
    file_output = gdb_tools_execute_command_sync (session, file_cmd, &error);
    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to load program: %s", error->message);
    }

    /* Load core file */
    core_cmd = g_strdup_printf ("core-file \"%s\"", core_path);
    core_output = gdb_tools_execute_command_sync (session, core_cmd, &error);
    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to load core file: %s", error->message);
    }

    /* Update session target */
    gdb_session_set_target_program (session, program);

    /* Get initial backtrace */
    bt_output = gdb_tools_execute_command_sync (session, "backtrace", NULL);

    return gdb_tools_create_success_result (
        "Core file loaded: %s\n\n"
        "Program: %s\n\n"
        "Output:\n%s\n%s\n\n"
        "Initial Backtrace:\n%s",
        core_path, program, file_output, core_output, bt_output ? bt_output : "(unavailable)");
}
