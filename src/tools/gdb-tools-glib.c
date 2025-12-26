/*
 * gdb-tools-glib.c - GLib/GObject-specific debugging tools for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Tools for debugging GLib/GObject/GIO applications:
 *   - gdb_glib_print_gobject: Pretty-print GObject instance
 *   - gdb_glib_print_glist: Pretty-print GList/GSList
 *   - gdb_glib_print_ghash: Pretty-print GHashTable
 *   - gdb_glib_type_hierarchy: Show GType inheritance chain
 *   - gdb_glib_signal_info: List signals on a GObject
 */

#include "gdb-tools-internal.h"

/* ========================================================================== */
/* Common schema for expression-based GLib tools                             */
/* ========================================================================== */

static JsonNode *
create_expression_schema (const gchar *description)
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
    json_builder_add_string_value (builder, description);
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


/* ========================================================================== */
/* gdb_glib_print_gobject - Pretty-print GObject instance                    */
/* ========================================================================== */

McpToolResult *
gdb_tools_handle_gdb_glib_print_gobject (McpServer   *server G_GNUC_UNUSED,
                                         const gchar *name G_GNUC_UNUSED,
                                         JsonObject  *arguments,
                                         gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *expression;
    g_autofree gchar *type_output = NULL;
    g_autofree gchar *ref_output = NULL;
    g_autofree gchar *data_output = NULL;
    GString *result_text;

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

    result_text = g_string_new (NULL);
    g_string_printf (result_text, "GObject Analysis: %s\n\n", expression);

    /* Get type name */
    {
        g_autofree gchar *cmd = g_strdup_printf ("print g_type_name(G_OBJECT_TYPE(%s))", expression);
        type_output = gdb_tools_execute_command_sync (session, cmd, NULL);
        if (type_output != NULL)
        {
            g_string_append_printf (result_text, "Type: %s\n", type_output);
        }
    }

    /* Get reference count */
    {
        g_autofree gchar *cmd = g_strdup_printf ("print ((GObject*)%s)->ref_count", expression);
        ref_output = gdb_tools_execute_command_sync (session, cmd, NULL);
        if (ref_output != NULL)
        {
            g_string_append_printf (result_text, "Reference Count: %s\n", ref_output);
        }
    }

    /* Print the object data */
    {
        g_autofree gchar *cmd = g_strdup_printf ("print *(%s)", expression);
        data_output = gdb_tools_execute_command_sync (session, cmd, NULL);
        if (data_output != NULL)
        {
            g_string_append_printf (result_text, "\nObject Data:\n%s", data_output);
        }
    }

    {
        McpToolResult *result = mcp_tool_result_new (FALSE);
        mcp_tool_result_add_text (result, result_text->str);
        g_string_free (result_text, TRUE);
        return result;
    }
}


/* ========================================================================== */
/* gdb_glib_print_glist - Pretty-print GList/GSList                          */
/* ========================================================================== */

McpToolResult *
gdb_tools_handle_gdb_glib_print_glist (McpServer   *server G_GNUC_UNUSED,
                                       const gchar *name G_GNUC_UNUSED,
                                       JsonObject  *arguments,
                                       gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *expression;
    GString *result_text;
    gint max_items = 20;
    gint count = 0;

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

    result_text = g_string_new (NULL);
    g_string_printf (result_text, "GList Contents: %s\n\n", expression);

    /* Iterate through the list */
    {
        g_autofree gchar *list_ptr_cmd = g_strdup_printf ("print (GList*)%s", expression);
        g_autofree gchar *ptr_output = gdb_tools_execute_command_sync (session, list_ptr_cmd, NULL);

        /* Use a GDB convenience variable to iterate */
        gdb_tools_execute_command_sync (session, "set $glist_iter = $", NULL);

        while (count < max_items)
        {
            g_autofree gchar *check_cmd = g_strdup ("print $glist_iter");
            g_autofree gchar *check_output = gdb_tools_execute_command_sync (session, check_cmd, NULL);

            /* Check if we've reached the end (NULL pointer) */
            if (check_output == NULL || g_strstr_len (check_output, -1, "0x0") != NULL ||
                g_strstr_len (check_output, -1, "(nil)") != NULL)
            {
                break;
            }

            /* Print current element */
            g_autofree gchar *data_cmd = g_strdup ("print $glist_iter->data");
            g_autofree gchar *data_output = gdb_tools_execute_command_sync (session, data_cmd, NULL);

            if (data_output != NULL)
            {
                g_string_append_printf (result_text, "[%d]: %s\n", count, data_output);
            }

            /* Move to next */
            gdb_tools_execute_command_sync (session, "set $glist_iter = $glist_iter->next", NULL);
            count++;
        }
    }

    if (count == 0)
    {
        g_string_append (result_text, "(empty list or NULL)\n");
    }
    else if (count >= max_items)
    {
        g_string_append_printf (result_text, "\n... (showing first %d items)\n", max_items);
    }

    g_string_append_printf (result_text, "\nTotal items shown: %d\n", count);

    {
        McpToolResult *result = mcp_tool_result_new (FALSE);
        mcp_tool_result_add_text (result, result_text->str);
        g_string_free (result_text, TRUE);
        return result;
    }
}


/* ========================================================================== */
/* gdb_glib_print_ghash - Pretty-print GHashTable                            */
/* ========================================================================== */

McpToolResult *
gdb_tools_handle_gdb_glib_print_ghash (McpServer   *server G_GNUC_UNUSED,
                                       const gchar *name G_GNUC_UNUSED,
                                       JsonObject  *arguments,
                                       gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *expression;
    GString *result_text;

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

    result_text = g_string_new (NULL);
    g_string_printf (result_text, "GHashTable Analysis: %s\n\n", expression);

    /* Get hash table size */
    {
        g_autofree gchar *cmd = g_strdup_printf ("print ((GHashTable*)%s)->size", expression);
        g_autofree gchar *size_output = gdb_tools_execute_command_sync (session, cmd, NULL);
        if (size_output != NULL)
        {
            g_string_append_printf (result_text, "Size: %s\n", size_output);
        }
    }

    /* Get number of nodes (nnodes) */
    {
        g_autofree gchar *cmd = g_strdup_printf ("print ((GHashTable*)%s)->nnodes", expression);
        g_autofree gchar *nnodes_output = gdb_tools_execute_command_sync (session, cmd, NULL);
        if (nnodes_output != NULL)
        {
            g_string_append_printf (result_text, "Number of entries: %s\n", nnodes_output);
        }
    }

    /* Print the hash table structure */
    {
        g_autofree gchar *cmd = g_strdup_printf ("print *(GHashTable*)%s", expression);
        g_autofree gchar *struct_output = gdb_tools_execute_command_sync (session, cmd, NULL);
        if (struct_output != NULL)
        {
            g_string_append_printf (result_text, "\nStructure:\n%s\n", struct_output);
        }
    }

    g_string_append (result_text, "\nNote: To iterate entries, use gdb_command with:\n");
    g_string_append (result_text, "  'call g_hash_table_foreach(table, print_func, NULL)'\n");

    {
        McpToolResult *result = mcp_tool_result_new (FALSE);
        mcp_tool_result_add_text (result, result_text->str);
        g_string_free (result_text, TRUE);
        return result;
    }
}


/* ========================================================================== */
/* gdb_glib_type_hierarchy - Show GType inheritance chain                    */
/* ========================================================================== */

McpToolResult *
gdb_tools_handle_gdb_glib_type_hierarchy (McpServer   *server G_GNUC_UNUSED,
                                          const gchar *name G_GNUC_UNUSED,
                                          JsonObject  *arguments,
                                          gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *expression;
    GString *result_text;
    gint depth = 0;

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

    result_text = g_string_new (NULL);
    g_string_printf (result_text, "Type Hierarchy for: %s\n\n", expression);

    /* Get the GType and walk up the hierarchy */
    {
        g_autofree gchar *type_cmd = g_strdup_printf ("set $gtype = G_OBJECT_TYPE(%s)", expression);
        gdb_tools_execute_command_sync (session, type_cmd, NULL);

        while (depth < 20) /* Safety limit */
        {
            /* Get type name */
            g_autofree gchar *name_cmd = g_strdup ("print g_type_name($gtype)");
            g_autofree gchar *name_output = gdb_tools_execute_command_sync (session, name_cmd, NULL);

            if (name_output == NULL || g_strstr_len (name_output, -1, "0x0") != NULL)
            {
                break;
            }

            /* Print with indentation */
            {
                gint i;
                for (i = 0; i < depth; i++)
                {
                    g_string_append (result_text, "  ");
                }
            }
            if (depth > 0)
            {
                g_string_append (result_text, "└─ ");
            }
            g_string_append_printf (result_text, "%s\n", name_output);

            /* Get parent type */
            gdb_tools_execute_command_sync (session, "set $gtype = g_type_parent($gtype)", NULL);

            /* Check if we've reached GObject or a fundamental type */
            g_autofree gchar *check_cmd = g_strdup ("print $gtype");
            g_autofree gchar *check_output = gdb_tools_execute_command_sync (session, check_cmd, NULL);

            if (check_output != NULL && g_strstr_len (check_output, -1, " = 0") != NULL)
            {
                break;
            }

            depth++;
        }
    }

    {
        McpToolResult *result = mcp_tool_result_new (FALSE);
        mcp_tool_result_add_text (result, result_text->str);
        g_string_free (result_text, TRUE);
        return result;
    }
}


/* ========================================================================== */
/* gdb_glib_signal_info - List signals on a GObject                          */
/* ========================================================================== */

McpToolResult *
gdb_tools_handle_gdb_glib_signal_info (McpServer   *server G_GNUC_UNUSED,
                                       const gchar *name G_GNUC_UNUSED,
                                       JsonObject  *arguments,
                                       gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    McpToolResult *error_result = NULL;
    GdbSession *session;
    const gchar *expression;
    GString *result_text;

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

    result_text = g_string_new (NULL);
    g_string_printf (result_text, "Signal Information for: %s\n\n", expression);

    /* Get the GType */
    {
        g_autofree gchar *type_cmd = g_strdup_printf ("set $gtype = G_OBJECT_TYPE(%s)", expression);
        gdb_tools_execute_command_sync (session, type_cmd, NULL);
    }

    /* Get type name */
    {
        g_autofree gchar *name_output = gdb_tools_execute_command_sync (session, "print g_type_name($gtype)", NULL);
        if (name_output != NULL)
        {
            g_string_append_printf (result_text, "Type: %s\n\n", name_output);
        }
    }

    /* List signals using g_signal_list_ids */
    {
        g_autofree gchar *list_cmd = g_strdup (
            "set $n_ids = 0\n"
            "set $signal_ids = g_signal_list_ids($gtype, &$n_ids)"
        );
        gdb_tools_execute_command_sync (session, "set $n_ids = 0", NULL);
        gdb_tools_execute_command_sync (session, "set $signal_ids = (guint*)g_signal_list_ids($gtype, &$n_ids)", NULL);

        g_autofree gchar *count_output = gdb_tools_execute_command_sync (session, "print $n_ids", NULL);
        if (count_output != NULL)
        {
            g_string_append_printf (result_text, "Number of signals: %s\n", count_output);
        }

        g_string_append (result_text, "\nSignals:\n");

        /* Iterate through signals */
        {
            gint i;
            for (i = 0; i < 50; i++) /* Safety limit */
            {
                g_autofree gchar *idx_check = g_strdup_printf ("print $n_ids > %d", i);
                g_autofree gchar *check_output = gdb_tools_execute_command_sync (session, idx_check, NULL);

                if (check_output == NULL || g_strstr_len (check_output, -1, " = 0") != NULL)
                {
                    break;
                }

                {
                    g_autofree gchar *sig_cmd = g_strdup_printf ("print g_signal_name($signal_ids[%d])", i);
                    g_autofree gchar *sig_output = gdb_tools_execute_command_sync (session, sig_cmd, NULL);

                    if (sig_output != NULL)
                    {
                        g_string_append_printf (result_text, "  - %s\n", sig_output);
                    }
                }
            }
        }
    }

    {
        McpToolResult *result = mcp_tool_result_new (FALSE);
        mcp_tool_result_add_text (result, result_text->str);
        g_string_free (result_text, TRUE);
        return result;
    }
}


/* ========================================================================== */
/* Schema Creation Functions                                                  */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_glib_print_gobject_schema (void)
{
    return create_expression_schema (
        "Pointer or variable referencing a GObject instance");
}

JsonNode *
gdb_tools_create_gdb_glib_print_glist_schema (void)
{
    return create_expression_schema (
        "Pointer or variable referencing a GList or GSList");
}

JsonNode *
gdb_tools_create_gdb_glib_print_ghash_schema (void)
{
    return create_expression_schema (
        "Pointer or variable referencing a GHashTable");
}

JsonNode *
gdb_tools_create_gdb_glib_type_hierarchy_schema (void)
{
    return create_expression_schema (
        "Pointer or variable referencing a GObject instance");
}

JsonNode *
gdb_tools_create_gdb_glib_signal_info_schema (void)
{
    return create_expression_schema (
        "Pointer or variable referencing a GObject instance");
}
