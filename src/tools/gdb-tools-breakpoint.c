/*
 * gdb-tools-breakpoint.c - Breakpoint management tools for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Tools: gdb_set_breakpoint
 */

#include "gdb-tools-internal.h"
#include <string.h>

/* ========================================================================== */
/* gdb_set_breakpoint - Set a breakpoint                                     */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_breakpoint_schema (void)
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

    /* location */
    json_builder_set_member_name (builder, "location");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Breakpoint location (e.g., function name, file:line, *address)");
    json_builder_end_object (builder);

    /* condition (optional) */
    json_builder_set_member_name (builder, "condition");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Breakpoint condition expression (optional)");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */

    json_builder_set_member_name (builder, "required");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "sessionId");
    json_builder_add_string_value (builder, "location");
    json_builder_end_array (builder);

    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

/*
 * extract_breakpoint_number:
 * @output: GDB output from break command
 *
 * Extracts the breakpoint number from output like "Breakpoint 1 at 0x..."
 *
 * Returns: the breakpoint number, or -1 if not found
 */
static gint
extract_breakpoint_number (const gchar *output)
{
    const gchar *p;
    gint bp_num = -1;

    if (output == NULL)
    {
        return -1;
    }

    /* Look for "Breakpoint N" pattern */
    p = strstr (output, "Breakpoint ");
    if (p != NULL)
    {
        p += strlen ("Breakpoint ");
        if (g_ascii_isdigit (*p))
        {
            bp_num = (gint) g_ascii_strtoll (p, NULL, 10);
        }
    }

    return bp_num;
}

McpToolResult *
gdb_tools_handle_gdb_set_breakpoint (McpServer   *server G_GNUC_UNUSED,
                                     const gchar *name G_GNUC_UNUSED,
                                     JsonObject  *arguments,
                                     gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *location;
    const gchar *condition = NULL;
    g_autofree gchar *output = NULL;
    g_autofree gchar *cond_output = NULL;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *break_cmd = NULL;

    /* Get session */
    session = gdb_tools_get_session (manager, arguments, &error_result);
    if (session == NULL)
    {
        return error_result;
    }

    /* Get location */
    if (!json_object_has_member (arguments, "location"))
    {
        return gdb_tools_create_error_result ("Missing required parameter: location");
    }
    location = json_object_get_string_member (arguments, "location");

    /* Get optional condition */
    if (json_object_has_member (arguments, "condition"))
    {
        condition = json_object_get_string_member (arguments, "condition");
    }

    /* Set breakpoint */
    break_cmd = g_strdup_printf ("break %s", location);
    output = gdb_tools_execute_command_sync (session, break_cmd, &error);

    if (error != NULL)
    {
        return gdb_tools_create_error_result ("Failed to set breakpoint: %s", error->message);
    }

    /* Set condition if provided */
    if (condition != NULL && condition[0] != '\0')
    {
        gint bp_num = extract_breakpoint_number (output);

        if (bp_num > 0)
        {
            g_autofree gchar *cond_cmd = NULL;

            cond_cmd = g_strdup_printf ("condition %d %s", bp_num, condition);
            cond_output = gdb_tools_execute_command_sync (session, cond_cmd, NULL);
        }
    }

    return gdb_tools_create_success_result (
        "Breakpoint set at: %s%s%s\n\nOutput:\n%s%s%s",
        location,
        condition ? " with condition: " : "",
        condition ? condition : "",
        output,
        cond_output ? "\n" : "",
        cond_output ? cond_output : "");
}
