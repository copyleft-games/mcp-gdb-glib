/*
 * gdb-tools-common.c - Common utilities for GDB MCP tools
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gdb-tools-internal.h"

/* ========================================================================== */
/* Synchronous Command Execution Wrapper                                     */
/* ========================================================================== */

typedef struct {
    GMainLoop *loop;
    gchar     *output;
    GError    *error;
} SyncExecuteData;

static void
on_execute_complete (GObject      *source,
                     GAsyncResult *result,
                     gpointer      user_data)
{
    SyncExecuteData *data = (SyncExecuteData *)user_data;

    data->output = gdb_session_execute_finish (GDB_SESSION (source), result, &data->error);
    g_main_loop_quit (data->loop);
}

gchar *
gdb_tools_execute_command_sync (GdbSession  *session,
                                const gchar *command,
                                GError     **error)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(GMainContext) context = NULL;
    SyncExecuteData data = { NULL, NULL, NULL };
    guint timeout_id;
    gboolean timed_out = FALSE;

    g_return_val_if_fail (GDB_IS_SESSION (session), NULL);
    g_return_val_if_fail (command != NULL, NULL);

    context = g_main_context_new ();
    loop = g_main_loop_new (context, FALSE);
    data.loop = loop;

    g_main_context_push_thread_default (context);

    /* Start async execution */
    gdb_session_execute_async (session, command, NULL, on_execute_complete, &data);

    /* Add timeout to the thread-default context (not the global default).
     * This is important because we're running our own main loop with our
     * own context - timeouts added with g_timeout_add would go to the
     * wrong context and never fire.
     */
    {
        GSource *timeout_source = g_timeout_source_new (gdb_session_get_timeout_ms (session) + 1000);
        g_source_set_callback (timeout_source,
            (GSourceFunc) ({
                gboolean timeout_cb (gpointer user_data) {
                    SyncExecuteData *d = (SyncExecuteData *)user_data;
                    if (d->loop != NULL && g_main_loop_is_running (d->loop))
                    {
                        g_main_loop_quit (d->loop);
                    }
                    return G_SOURCE_REMOVE;
                }
                timeout_cb;
            }),
            &data, NULL);
        timeout_id = g_source_attach (timeout_source, context);
        g_source_unref (timeout_source);
    }

    /* Run the loop */
    g_main_loop_run (loop);

    /* Check if we timed out before callback */
    if (data.output == NULL && data.error == NULL)
    {
        timed_out = TRUE;
    }

    g_source_remove (timeout_id);
    g_main_context_pop_thread_default (context);

    if (timed_out)
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_TIMEOUT,
                     "GDB command timed out: %s", command);
        return NULL;
    }

    if (data.error != NULL)
    {
        g_propagate_error (error, data.error);
        return NULL;
    }

    return data.output;
}
