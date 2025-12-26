/*
 * gdb-session-manager.h - GDB session manager for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * GdbSessionManager manages multiple concurrent GDB debugging sessions.
 * It provides session creation, lookup, and cleanup functionality.
 */

#ifndef GDB_SESSION_MANAGER_H
#define GDB_SESSION_MANAGER_H

#include <glib-object.h>

#include "gdb-session.h"

G_BEGIN_DECLS

#define GDB_TYPE_SESSION_MANAGER (gdb_session_manager_get_type ())

G_DECLARE_FINAL_TYPE (GdbSessionManager, gdb_session_manager, GDB, SESSION_MANAGER, GObject)

/**
 * gdb_session_manager_new:
 *
 * Creates a new session manager.
 *
 * Returns: (transfer full): a new #GdbSessionManager
 */
GdbSessionManager *gdb_session_manager_new (void);

/**
 * gdb_session_manager_get_default:
 *
 * Gets the default session manager singleton.
 *
 * Returns: (transfer none): the default #GdbSessionManager
 */
GdbSessionManager *gdb_session_manager_get_default (void);

/**
 * gdb_session_manager_get_default_gdb_path:
 * @self: a #GdbSessionManager
 *
 * Gets the default path to GDB for new sessions.
 *
 * Returns: (transfer none): the default GDB path
 */
const gchar *gdb_session_manager_get_default_gdb_path (GdbSessionManager *self);

/**
 * gdb_session_manager_set_default_gdb_path:
 * @self: a #GdbSessionManager
 * @gdb_path: (nullable): the default GDB path, or %NULL for "gdb"
 *
 * Sets the default path to GDB for new sessions.
 */
void gdb_session_manager_set_default_gdb_path (GdbSessionManager *self,
                                               const gchar       *gdb_path);

/**
 * gdb_session_manager_get_default_timeout_ms:
 * @self: a #GdbSessionManager
 *
 * Gets the default command timeout for new sessions.
 *
 * Returns: the default timeout in milliseconds
 */
guint gdb_session_manager_get_default_timeout_ms (GdbSessionManager *self);

/**
 * gdb_session_manager_set_default_timeout_ms:
 * @self: a #GdbSessionManager
 * @timeout_ms: the default timeout in milliseconds
 *
 * Sets the default command timeout for new sessions.
 */
void gdb_session_manager_set_default_timeout_ms (GdbSessionManager *self,
                                                 guint              timeout_ms);

/**
 * gdb_session_manager_get_session_count:
 * @self: a #GdbSessionManager
 *
 * Gets the number of active sessions.
 *
 * Returns: the session count
 */
guint gdb_session_manager_get_session_count (GdbSessionManager *self);

/**
 * gdb_session_manager_create_session:
 * @self: a #GdbSessionManager
 * @gdb_path: (nullable): path to GDB, or %NULL for default
 * @working_dir: (nullable): working directory
 *
 * Creates a new session with a unique ID.
 * The session is not started; call gdb_session_start_async() on it.
 *
 * Returns: (transfer full): a new #GdbSession
 */
GdbSession *gdb_session_manager_create_session (GdbSessionManager *self,
                                                const gchar       *gdb_path,
                                                const gchar       *working_dir);

/**
 * gdb_session_manager_get_session:
 * @self: a #GdbSessionManager
 * @session_id: the session ID
 *
 * Gets a session by its ID.
 *
 * Returns: (transfer none) (nullable): the #GdbSession, or %NULL if not found
 */
GdbSession *gdb_session_manager_get_session (GdbSessionManager *self,
                                             const gchar       *session_id);

/**
 * gdb_session_manager_remove_session:
 * @self: a #GdbSessionManager
 * @session_id: the session ID
 *
 * Removes and terminates a session.
 *
 * Returns: %TRUE if the session was found and removed
 */
gboolean gdb_session_manager_remove_session (GdbSessionManager *self,
                                             const gchar       *session_id);

/**
 * gdb_session_manager_list_sessions:
 * @self: a #GdbSessionManager
 *
 * Lists all active sessions.
 *
 * Returns: (transfer full) (element-type GdbSession): list of sessions
 */
GList *gdb_session_manager_list_sessions (GdbSessionManager *self);

/**
 * gdb_session_manager_terminate_all:
 * @self: a #GdbSessionManager
 *
 * Terminates and removes all sessions.
 */
void gdb_session_manager_terminate_all (GdbSessionManager *self);

G_END_DECLS

#endif /* GDB_SESSION_MANAGER_H */
