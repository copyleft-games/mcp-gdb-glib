/*
 * gdb-session.h - GDB session management for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * GdbSession represents a single GDB debugging session.
 * It manages the GDB subprocess lifecycle, communication,
 * and provides async methods for executing commands.
 */

#ifndef GDB_SESSION_H
#define GDB_SESSION_H

#include <glib-object.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>

#include "gdb-enums.h"
#include "gdb-mi-parser.h"

/* Default delay (ms) after writing command before reading output.
 * Override with GDB_MCP_POST_COMMAND_DELAY_MS environment variable.
 */
#define DEFAULT_POST_COMMAND_DELAY_MS 2000

G_BEGIN_DECLS

#define GDB_TYPE_SESSION (gdb_session_get_type ())

G_DECLARE_FINAL_TYPE (GdbSession, gdb_session, GDB, SESSION, GObject)

/**
 * gdb_session_new:
 * @session_id: unique identifier for this session
 * @gdb_path: (nullable): path to GDB executable, or %NULL for "gdb"
 * @working_dir: (nullable): working directory for GDB
 *
 * Creates a new GDB session. The session is not started until
 * gdb_session_start_async() is called.
 *
 * Returns: (transfer full): a new #GdbSession
 */
GdbSession *gdb_session_new (const gchar *session_id,
                             const gchar *gdb_path,
                             const gchar *working_dir);

/**
 * gdb_session_get_session_id:
 * @self: a #GdbSession
 *
 * Gets the session identifier.
 *
 * Returns: (transfer none): the session ID
 */
const gchar *gdb_session_get_session_id (GdbSession *self);

/**
 * gdb_session_get_gdb_path:
 * @self: a #GdbSession
 *
 * Gets the path to the GDB executable.
 *
 * Returns: (transfer none): the GDB path
 */
const gchar *gdb_session_get_gdb_path (GdbSession *self);

/**
 * gdb_session_get_working_dir:
 * @self: a #GdbSession
 *
 * Gets the working directory.
 *
 * Returns: (transfer none) (nullable): the working directory
 */
const gchar *gdb_session_get_working_dir (GdbSession *self);

/**
 * gdb_session_get_target_program:
 * @self: a #GdbSession
 *
 * Gets the currently loaded program.
 *
 * Returns: (transfer none) (nullable): the program path
 */
const gchar *gdb_session_get_target_program (GdbSession *self);

/**
 * gdb_session_set_target_program:
 * @self: a #GdbSession
 * @program: (nullable): the program path
 *
 * Sets the target program path.
 */
void gdb_session_set_target_program (GdbSession  *self,
                                     const gchar *program);

/**
 * gdb_session_get_state:
 * @self: a #GdbSession
 *
 * Gets the current session state.
 *
 * Returns: the #GdbSessionState
 */
GdbSessionState gdb_session_get_state (GdbSession *self);

/**
 * gdb_session_is_ready:
 * @self: a #GdbSession
 *
 * Checks if the session is ready to accept commands.
 *
 * Returns: %TRUE if ready
 */
gboolean gdb_session_is_ready (GdbSession *self);

/**
 * gdb_session_get_timeout_ms:
 * @self: a #GdbSession
 *
 * Gets the command timeout in milliseconds.
 *
 * Returns: the timeout value
 */
guint gdb_session_get_timeout_ms (GdbSession *self);

/**
 * gdb_session_set_timeout_ms:
 * @self: a #GdbSession
 * @timeout_ms: the timeout in milliseconds
 *
 * Sets the command timeout.
 */
void gdb_session_set_timeout_ms (GdbSession *self,
                                 guint       timeout_ms);

/**
 * gdb_session_start_async:
 * @self: a #GdbSession
 * @cancellable: (nullable): a #GCancellable
 * @callback: callback to call when complete
 * @user_data: user data for @callback
 *
 * Starts the GDB subprocess and waits for it to be ready.
 */
void gdb_session_start_async (GdbSession          *self,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data);

/**
 * gdb_session_start_finish:
 * @self: a #GdbSession
 * @result: the #GAsyncResult
 * @error: (nullable): return location for a #GError
 *
 * Completes an asynchronous start operation.
 *
 * Returns: %TRUE on success
 */
gboolean gdb_session_start_finish (GdbSession    *self,
                                   GAsyncResult  *result,
                                   GError       **error);

/**
 * gdb_session_execute_async:
 * @self: a #GdbSession
 * @command: the GDB command to execute
 * @cancellable: (nullable): a #GCancellable
 * @callback: callback to call when complete
 * @user_data: user data for @callback
 *
 * Executes a GDB command and returns the parsed MI output.
 * The session must be in the READY or STOPPED state.
 */
void gdb_session_execute_async (GdbSession          *self,
                                const gchar         *command,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data);

/**
 * gdb_session_execute_finish:
 * @self: a #GdbSession
 * @result: the #GAsyncResult
 * @error: (nullable): return location for a #GError
 *
 * Completes an asynchronous execute operation.
 *
 * Returns: (transfer full) (nullable): the output as a string, or %NULL on error
 */
gchar *gdb_session_execute_finish (GdbSession    *self,
                                   GAsyncResult  *result,
                                   GError       **error);

/**
 * gdb_session_execute_mi_async:
 * @self: a #GdbSession
 * @command: the GDB/MI command to execute
 * @cancellable: (nullable): a #GCancellable
 * @callback: callback to call when complete
 * @user_data: user data for @callback
 *
 * Executes a GDB/MI command and returns parsed MI records.
 */
void gdb_session_execute_mi_async (GdbSession          *self,
                                   const gchar         *command,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);

/**
 * gdb_session_execute_mi_finish:
 * @self: a #GdbSession
 * @result: the #GAsyncResult
 * @error: (nullable): return location for a #GError
 *
 * Completes an asynchronous MI execute operation.
 *
 * Returns: (transfer full) (element-type GdbMiRecord) (nullable): list of records
 */
GList *gdb_session_execute_mi_finish (GdbSession    *self,
                                      GAsyncResult  *result,
                                      GError       **error);

/**
 * gdb_session_terminate:
 * @self: a #GdbSession
 *
 * Terminates the GDB subprocess.
 * Sends "quit" command first, then force kills if needed.
 */
void gdb_session_terminate (GdbSession *self);

/**
 * gdb_session_get_mi_parser:
 * @self: a #GdbSession
 *
 * Gets the MI parser used by this session.
 *
 * Returns: (transfer none): the #GdbMiParser
 */
GdbMiParser *gdb_session_get_mi_parser (GdbSession *self);

G_END_DECLS

#endif /* GDB_SESSION_H */
