/*
 * gdb-tools-session.c - Session management tools for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Tools: gdb_start, gdb_terminate, gdb_list_sessions
 */

#include "gdb-tools-internal.h"

/* ========================================================================== */
/* gdb_start - Start a new GDB session                                       */
/* ========================================================================== */

JsonNode *
gdb_tools_create_gdb_start_schema (void)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();

    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "object");

    json_builder_set_member_name (builder, "properties");
    json_builder_begin_object (builder);

    /* gdbPath property */
    json_builder_set_member_name (builder, "gdbPath");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Path to GDB executable (optional, defaults to 'gdb')");
    json_builder_end_object (builder);

    /* workingDir property */
    json_builder_set_member_name (builder, "workingDir");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "Working directory for GDB (optional)");
    json_builder_end_object (builder);

    json_builder_end_object (builder); /* properties */
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

/*
 * Synchronous session start data
 */
typedef struct {
    GMainLoop *loop;
    gboolean   success;
    GError    *error;
} SyncStartData;

static void
on_session_started (GObject      *source,
                    GAsyncResult *result,
                    gpointer      user_data)
{
    SyncStartData *data = (SyncStartData *)user_data;

    data->success = gdb_session_start_finish (GDB_SESSION (source), result, &data->error);
    g_main_loop_quit (data->loop);
}

static gboolean
on_start_timeout (gpointer user_data)
{
    GMainLoop *loop = (GMainLoop *)user_data;

    g_main_loop_quit (loop);
    return G_SOURCE_REMOVE;
}

McpToolResult *
gdb_tools_handle_gdb_start (McpServer   *server G_GNUC_UNUSED,
                            const gchar *name G_GNUC_UNUSED,
                            JsonObject  *arguments,
                            gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    g_autoptr(GdbSession) session = NULL;
    g_autoptr(GMainLoop) loop = NULL;
    const gchar *gdb_path = NULL;
    const gchar *working_dir = NULL;
    SyncStartData sync_data;

    /* Extract arguments */
    if (arguments != NULL)
    {
        if (json_object_has_member (arguments, "gdbPath"))
        {
            gdb_path = json_object_get_string_member (arguments, "gdbPath");
        }
        if (json_object_has_member (arguments, "workingDir"))
        {
            working_dir = json_object_get_string_member (arguments, "workingDir");
        }
    }

    /* Create session */
    session = gdb_session_manager_create_session (manager, gdb_path, working_dir);

    /* Start session synchronously using a temporary main loop.
     * We create our own context and push it as thread-default so that
     * all async operations and timeouts work correctly.
     */
    {
        g_autoptr(GMainContext) context = g_main_context_new ();
        GSource *timeout_source;

        loop = g_main_loop_new (context, FALSE);

        sync_data.loop = loop;
        sync_data.success = FALSE;
        sync_data.error = NULL;

        g_main_context_push_thread_default (context);

        gdb_session_start_async (session, NULL, on_session_started, &sync_data);

        /* Add timeout to the same context */
        timeout_source = g_timeout_source_new (gdb_session_get_timeout_ms (session) + 1000);
        g_source_set_callback (timeout_source, on_start_timeout, loop, NULL);
        g_source_attach (timeout_source, context);

        g_main_loop_run (loop);

        /* Destroy the timeout source - use g_source_destroy since the source
         * is attached to our local context, not the global default context.
         */
        g_source_destroy (timeout_source);
        g_source_unref (timeout_source);

        g_main_context_pop_thread_default (context);
    }

    if (!sync_data.success)
    {
        const gchar *session_id = gdb_session_get_session_id (session);
        gdb_session_manager_remove_session (manager, session_id);

        if (sync_data.error != NULL)
        {
            McpToolResult *result = gdb_tools_create_error_result (
                "Failed to start GDB: %s", sync_data.error->message);
            g_error_free (sync_data.error);
            return result;
        }

        return gdb_tools_create_error_result ("Failed to start GDB: Timeout");
    }

    /* Build success result */
    {
        McpToolResult *result;
        g_autofree gchar *text = NULL;
        const gchar *session_id = gdb_session_get_session_id (session);

        text = g_strdup_printf ("GDB session started successfully.\n\n"
                                "Session ID: %s\n"
                                "GDB Path: %s\n"
                                "Working Directory: %s",
                                session_id,
                                gdb_session_get_gdb_path (session),
                                gdb_session_get_working_dir (session) ?
                                    gdb_session_get_working_dir (session) : "(current)");

        result = mcp_tool_result_new (FALSE);
        mcp_tool_result_add_text (result, text);

        return result;
    }
}


/* ========================================================================== */
/* gdb_terminate - Terminate a GDB session                                   */
/* ========================================================================== */

JsonNode *
gdb_tools_create_session_id_only_schema (void)
{
    g_autoptr(JsonBuilder) builder = json_builder_new ();

    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "object");

    json_builder_set_member_name (builder, "properties");
    json_builder_begin_object (builder);

    /* sessionId property */
    json_builder_set_member_name (builder, "sessionId");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, "string");
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, "GDB session ID");
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
gdb_tools_handle_gdb_terminate (McpServer   *server G_GNUC_UNUSED,
                                const gchar *name G_GNUC_UNUSED,
                                JsonObject  *arguments,
                                gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    const gchar *session_id;
    gboolean removed;

    /* Get session ID */
    session_id = gdb_tools_get_session_id (arguments);
    if (session_id == NULL)
    {
        return gdb_tools_create_error_result ("Missing required parameter: sessionId");
    }

    /* Remove session */
    removed = gdb_session_manager_remove_session (manager, session_id);

    if (!removed)
    {
        return gdb_tools_create_error_result ("No active GDB session with ID: %s", session_id);
    }

    {
        McpToolResult *result;
        g_autofree gchar *text = NULL;

        text = g_strdup_printf ("GDB session terminated: %s", session_id);

        result = mcp_tool_result_new (FALSE);
        mcp_tool_result_add_text (result, text);

        return result;
    }
}


/* ========================================================================== */
/* gdb_list_sessions - List all active GDB sessions                          */
/* ========================================================================== */

McpToolResult *
gdb_tools_handle_gdb_list_sessions (McpServer   *server G_GNUC_UNUSED,
                                    const gchar *name G_GNUC_UNUSED,
                                    JsonObject  *arguments G_GNUC_UNUSED,
                                    gpointer     user_data)
{
    GdbSessionManager *manager = GDB_SESSION_MANAGER (user_data);
    g_autoptr(GList) sessions = NULL;
    GString *text;
    guint count;
    GList *l;

    sessions = gdb_session_manager_list_sessions (manager);
    count = g_list_length (sessions);

    text = g_string_new (NULL);
    g_string_printf (text, "Active GDB Sessions (%u):\n\n", count);

    if (count == 0)
    {
        g_string_append (text, "No active sessions.");
    }
    else
    {
        for (l = sessions; l != NULL; l = l->next)
        {
            GdbSession *session = GDB_SESSION (l->data);
            const gchar *target = gdb_session_get_target_program (session);
            const gchar *state_str = gdb_session_state_to_string (gdb_session_get_state (session));

            g_string_append_printf (text, "- ID: %s\n", gdb_session_get_session_id (session));
            g_string_append_printf (text, "  Target: %s\n", target ? target : "(none)");
            g_string_append_printf (text, "  State: %s\n", state_str);
            g_string_append_printf (text, "  Working Dir: %s\n",
                                    gdb_session_get_working_dir (session) ?
                                        gdb_session_get_working_dir (session) : "(default)");

            if (l->next != NULL)
            {
                g_string_append_c (text, '\n');
            }
        }
    }

    {
        McpToolResult *result;

        result = mcp_tool_result_new (FALSE);
        mcp_tool_result_add_text (result, text->str);
        g_string_free (text, TRUE);

        return result;
    }
}
