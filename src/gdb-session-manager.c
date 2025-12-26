/*
 * gdb-session-manager.c - GDB session manager implementation for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "mcp-gdb/gdb-session-manager.h"

#include <string.h>

#define DEFAULT_TIMEOUT_MS 10000
#define DEFAULT_GDB_PATH   "gdb"

/* ========================================================================== */
/* GdbSessionManager Structure                                                */
/* ========================================================================== */

struct _GdbSessionManager
{
    GObject      parent_instance;

    /* Session storage */
    GHashTable  *sessions;          /* session_id -> GdbSession */
    GMutex       mutex;             /* Thread safety */

    /* Configuration */
    gchar       *default_gdb_path;
    guint        default_timeout_ms;

    /* Session ID generation */
    guint64      session_counter;
};

/* ========================================================================== */
/* Property IDs                                                               */
/* ========================================================================== */

enum {
    PROP_0,
    PROP_DEFAULT_GDB_PATH,
    PROP_DEFAULT_TIMEOUT_MS,
    PROP_SESSION_COUNT,
    N_PROPS
};

static GParamSpec *properties[N_PROPS];

/* ========================================================================== */
/* Signal IDs                                                                 */
/* ========================================================================== */

enum {
    SIGNAL_SESSION_ADDED,
    SIGNAL_SESSION_REMOVED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

/* Singleton instance */
static GdbSessionManager *default_manager = NULL;

G_DEFINE_TYPE (GdbSessionManager, gdb_session_manager, G_TYPE_OBJECT)

/* ========================================================================== */
/* GObject Implementation                                                     */
/* ========================================================================== */

static void
gdb_session_manager_dispose (GObject *object)
{
    GdbSessionManager *self = GDB_SESSION_MANAGER (object);

    gdb_session_manager_terminate_all (self);

    G_OBJECT_CLASS (gdb_session_manager_parent_class)->dispose (object);
}

static void
gdb_session_manager_finalize (GObject *object)
{
    GdbSessionManager *self = GDB_SESSION_MANAGER (object);

    g_clear_pointer (&self->sessions, g_hash_table_unref);
    g_clear_pointer (&self->default_gdb_path, g_free);
    g_mutex_clear (&self->mutex);

    if (default_manager == self)
    {
        default_manager = NULL;
    }

    G_OBJECT_CLASS (gdb_session_manager_parent_class)->finalize (object);
}

static void
gdb_session_manager_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
    GdbSessionManager *self = GDB_SESSION_MANAGER (object);

    switch (prop_id)
    {
        case PROP_DEFAULT_GDB_PATH:
            g_value_set_string (value, self->default_gdb_path);
            break;
        case PROP_DEFAULT_TIMEOUT_MS:
            g_value_set_uint (value, self->default_timeout_ms);
            break;
        case PROP_SESSION_COUNT:
            g_value_set_uint (value, gdb_session_manager_get_session_count (self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdb_session_manager_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
    GdbSessionManager *self = GDB_SESSION_MANAGER (object);

    switch (prop_id)
    {
        case PROP_DEFAULT_GDB_PATH:
            gdb_session_manager_set_default_gdb_path (self, g_value_get_string (value));
            break;
        case PROP_DEFAULT_TIMEOUT_MS:
            gdb_session_manager_set_default_timeout_ms (self, g_value_get_uint (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdb_session_manager_class_init (GdbSessionManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = gdb_session_manager_dispose;
    object_class->finalize = gdb_session_manager_finalize;
    object_class->get_property = gdb_session_manager_get_property;
    object_class->set_property = gdb_session_manager_set_property;

    /**
     * GdbSessionManager:default-gdb-path:
     *
     * Default path to GDB for new sessions.
     */
    properties[PROP_DEFAULT_GDB_PATH] =
        g_param_spec_string ("default-gdb-path",
                             "Default GDB Path",
                             "Default path to GDB for new sessions",
                             DEFAULT_GDB_PATH,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

    /**
     * GdbSessionManager:default-timeout-ms:
     *
     * Default command timeout for new sessions.
     */
    properties[PROP_DEFAULT_TIMEOUT_MS] =
        g_param_spec_uint ("default-timeout-ms",
                           "Default Timeout",
                           "Default command timeout in milliseconds",
                           0, G_MAXUINT, DEFAULT_TIMEOUT_MS,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

    /**
     * GdbSessionManager:session-count:
     *
     * Number of active sessions.
     */
    properties[PROP_SESSION_COUNT] =
        g_param_spec_uint ("session-count",
                           "Session Count",
                           "Number of active sessions",
                           0, G_MAXUINT, 0,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    /**
     * GdbSessionManager::session-added:
     * @manager: the #GdbSessionManager
     * @session: the added #GdbSession
     *
     * Emitted when a new session is added.
     */
    signals[SIGNAL_SESSION_ADDED] =
        g_signal_new ("session-added",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 1,
                      GDB_TYPE_SESSION);

    /**
     * GdbSessionManager::session-removed:
     * @manager: the #GdbSessionManager
     * @session_id: the removed session ID
     *
     * Emitted when a session is removed.
     */
    signals[SIGNAL_SESSION_REMOVED] =
        g_signal_new ("session-removed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 1,
                      G_TYPE_STRING);
}

static void
gdb_session_manager_init (GdbSessionManager *self)
{
    g_mutex_init (&self->mutex);
    self->sessions = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            g_free, g_object_unref);
    self->default_gdb_path = g_strdup (DEFAULT_GDB_PATH);
    self->default_timeout_ms = DEFAULT_TIMEOUT_MS;
    self->session_counter = 0;
}

/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

GdbSessionManager *
gdb_session_manager_new (void)
{
    return (GdbSessionManager *)g_object_new (GDB_TYPE_SESSION_MANAGER, NULL);
}

GdbSessionManager *
gdb_session_manager_get_default (void)
{
    if (default_manager == NULL)
    {
        default_manager = gdb_session_manager_new ();
    }

    return default_manager;
}

const gchar *
gdb_session_manager_get_default_gdb_path (GdbSessionManager *self)
{
    g_return_val_if_fail (GDB_IS_SESSION_MANAGER (self), NULL);
    return self->default_gdb_path;
}

void
gdb_session_manager_set_default_gdb_path (GdbSessionManager *self,
                                          const gchar       *gdb_path)
{
    g_return_if_fail (GDB_IS_SESSION_MANAGER (self));

    g_free (self->default_gdb_path);
    self->default_gdb_path = g_strdup (gdb_path ? gdb_path : DEFAULT_GDB_PATH);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DEFAULT_GDB_PATH]);
}

guint
gdb_session_manager_get_default_timeout_ms (GdbSessionManager *self)
{
    g_return_val_if_fail (GDB_IS_SESSION_MANAGER (self), DEFAULT_TIMEOUT_MS);
    return self->default_timeout_ms;
}

void
gdb_session_manager_set_default_timeout_ms (GdbSessionManager *self,
                                            guint              timeout_ms)
{
    g_return_if_fail (GDB_IS_SESSION_MANAGER (self));

    self->default_timeout_ms = timeout_ms;
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DEFAULT_TIMEOUT_MS]);
}

guint
gdb_session_manager_get_session_count (GdbSessionManager *self)
{
    guint count;

    g_return_val_if_fail (GDB_IS_SESSION_MANAGER (self), 0);

    g_mutex_lock (&self->mutex);
    count = g_hash_table_size (self->sessions);
    g_mutex_unlock (&self->mutex);

    return count;
}

GdbSession *
gdb_session_manager_create_session (GdbSessionManager *self,
                                    const gchar       *gdb_path,
                                    const gchar       *working_dir)
{
    GdbSession *session;
    g_autofree gchar *session_id = NULL;

    g_return_val_if_fail (GDB_IS_SESSION_MANAGER (self), NULL);

    g_mutex_lock (&self->mutex);

    /* Generate unique session ID using timestamp + counter */
    session_id = g_strdup_printf ("%" G_GINT64_FORMAT "-%lu",
                                  g_get_real_time (),
                                  (gulong) self->session_counter++);

    /* Use default GDB path if not specified */
    if (gdb_path == NULL)
    {
        gdb_path = self->default_gdb_path;
    }

    /* Create session */
    session = gdb_session_new (session_id, gdb_path, working_dir);
    gdb_session_set_timeout_ms (session, self->default_timeout_ms);

    /* Store in hash table */
    g_hash_table_insert (self->sessions,
                         g_strdup (session_id),
                         g_object_ref (session));

    g_mutex_unlock (&self->mutex);

    g_signal_emit (self, signals[SIGNAL_SESSION_ADDED], 0, session);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SESSION_COUNT]);

    return session;
}

GdbSession *
gdb_session_manager_get_session (GdbSessionManager *self,
                                 const gchar       *session_id)
{
    GdbSession *session;

    g_return_val_if_fail (GDB_IS_SESSION_MANAGER (self), NULL);
    g_return_val_if_fail (session_id != NULL, NULL);

    g_mutex_lock (&self->mutex);
    session = (GdbSession *)g_hash_table_lookup (self->sessions, session_id);
    g_mutex_unlock (&self->mutex);

    return session;
}

gboolean
gdb_session_manager_remove_session (GdbSessionManager *self,
                                    const gchar       *session_id)
{
    GdbSession *session;
    gboolean removed;

    g_return_val_if_fail (GDB_IS_SESSION_MANAGER (self), FALSE);
    g_return_val_if_fail (session_id != NULL, FALSE);

    g_mutex_lock (&self->mutex);

    session = (GdbSession *)g_hash_table_lookup (self->sessions, session_id);
    if (session != NULL)
    {
        gdb_session_terminate (session);
        removed = g_hash_table_remove (self->sessions, session_id);
    }
    else
    {
        removed = FALSE;
    }

    g_mutex_unlock (&self->mutex);

    if (removed)
    {
        g_signal_emit (self, signals[SIGNAL_SESSION_REMOVED], 0, session_id);
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SESSION_COUNT]);
    }

    return removed;
}

GList *
gdb_session_manager_list_sessions (GdbSessionManager *self)
{
    GList *sessions;
    GHashTableIter iter;
    gpointer value;

    g_return_val_if_fail (GDB_IS_SESSION_MANAGER (self), NULL);

    sessions = NULL;

    g_mutex_lock (&self->mutex);

    g_hash_table_iter_init (&iter, self->sessions);
    while (g_hash_table_iter_next (&iter, NULL, &value))
    {
        sessions = g_list_prepend (sessions, g_object_ref (value));
    }

    g_mutex_unlock (&self->mutex);

    return sessions;
}

void
gdb_session_manager_terminate_all (GdbSessionManager *self)
{
    GList *session_ids;
    GHashTableIter iter;
    gpointer key;

    g_return_if_fail (GDB_IS_SESSION_MANAGER (self));

    g_mutex_lock (&self->mutex);

    /* Collect all session IDs first */
    session_ids = NULL;
    g_hash_table_iter_init (&iter, self->sessions);
    while (g_hash_table_iter_next (&iter, &key, NULL))
    {
        session_ids = g_list_prepend (session_ids, g_strdup ((gchar *)key));
    }

    g_mutex_unlock (&self->mutex);

    /* Remove each session (this will emit signals) */
    {
        GList *l;
        for (l = session_ids; l != NULL; l = l->next)
        {
            gdb_session_manager_remove_session (self, (gchar *)l->data);
        }
    }

    g_list_free_full (session_ids, g_free);
}
