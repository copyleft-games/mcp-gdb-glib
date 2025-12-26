/*
 * test-session-manager.c - Unit tests for GdbSessionManager
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include "mcp-gdb/gdb-session-manager.h"

/* ========================================================================== */
/* Construction Tests                                                         */
/* ========================================================================== */

static void
test_session_manager_new (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;

    manager = gdb_session_manager_new ();

    g_assert_nonnull (manager);
    g_assert_true (GDB_IS_SESSION_MANAGER (manager));
}

static void
test_session_manager_singleton (void)
{
    GdbSessionManager *manager1;
    GdbSessionManager *manager2;

    manager1 = gdb_session_manager_get_default ();
    manager2 = gdb_session_manager_get_default ();

    g_assert_nonnull (manager1);
    g_assert_true (manager1 == manager2);
}


/* ========================================================================== */
/* Properties Tests                                                           */
/* ========================================================================== */

static void
test_session_manager_properties (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;

    manager = gdb_session_manager_new ();

    /* Default GDB path */
    g_assert_cmpstr (gdb_session_manager_get_default_gdb_path (manager), ==, "gdb");

    gdb_session_manager_set_default_gdb_path (manager, "/usr/bin/gdb");
    g_assert_cmpstr (gdb_session_manager_get_default_gdb_path (manager), ==, "/usr/bin/gdb");

    /* Reset to default */
    gdb_session_manager_set_default_gdb_path (manager, NULL);
    g_assert_cmpstr (gdb_session_manager_get_default_gdb_path (manager), ==, "gdb");

    /* Default timeout */
    g_assert_cmpuint (gdb_session_manager_get_default_timeout_ms (manager), ==, 10000);

    gdb_session_manager_set_default_timeout_ms (manager, 5000);
    g_assert_cmpuint (gdb_session_manager_get_default_timeout_ms (manager), ==, 5000);

    /* Session count (should be 0 initially) */
    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 0);
}


/* ========================================================================== */
/* Session Creation Tests                                                     */
/* ========================================================================== */

static void
test_session_manager_create_session (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *session1;
    GdbSession *session2;
    const gchar *id1;
    const gchar *id2;

    manager = gdb_session_manager_new ();

    session1 = gdb_session_manager_create_session (manager, NULL, NULL);
    g_assert_nonnull (session1);
    id1 = gdb_session_get_session_id (session1);
    g_assert_nonnull (id1);

    session2 = gdb_session_manager_create_session (manager, NULL, NULL);
    g_assert_nonnull (session2);
    id2 = gdb_session_get_session_id (session2);
    g_assert_nonnull (id2);

    /* IDs should be unique */
    g_assert_cmpstr (id1, !=, id2);

    /* Session count should be 2 */
    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 2);

    g_object_unref (session1);
    g_object_unref (session2);
}

static void
test_session_manager_create_with_options (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *session;

    manager = gdb_session_manager_new ();

    session = gdb_session_manager_create_session (manager, "/custom/gdb", "/working/dir");
    g_assert_nonnull (session);
    g_assert_cmpstr (gdb_session_get_gdb_path (session), ==, "/custom/gdb");
    g_assert_cmpstr (gdb_session_get_working_dir (session), ==, "/working/dir");

    g_object_unref (session);
}


/* ========================================================================== */
/* Session Lookup Tests                                                       */
/* ========================================================================== */

static void
test_session_manager_get_session (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *session;
    GdbSession *found;
    const gchar *id;

    manager = gdb_session_manager_new ();

    session = gdb_session_manager_create_session (manager, NULL, NULL);
    id = gdb_session_get_session_id (session);

    found = gdb_session_manager_get_session (manager, id);
    g_assert_true (found == session);

    g_object_unref (session);
}

static void
test_session_manager_get_session_not_found (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *found;

    manager = gdb_session_manager_new ();

    found = gdb_session_manager_get_session (manager, "nonexistent-session");
    g_assert_null (found);
}


/* ========================================================================== */
/* Session Removal Tests                                                      */
/* ========================================================================== */

static void
test_session_manager_remove_session (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *session;
    const gchar *id;
    gboolean removed;

    manager = gdb_session_manager_new ();

    session = gdb_session_manager_create_session (manager, NULL, NULL);
    id = g_strdup (gdb_session_get_session_id (session));
    g_object_unref (session);

    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 1);

    removed = gdb_session_manager_remove_session (manager, id);
    g_assert_true (removed);
    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 0);

    /* Should not be found anymore */
    g_assert_null (gdb_session_manager_get_session (manager, id));

    g_free ((gchar *)id);
}

static void
test_session_manager_remove_not_found (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    gboolean removed;

    manager = gdb_session_manager_new ();

    removed = gdb_session_manager_remove_session (manager, "nonexistent");
    g_assert_false (removed);
}


/* ========================================================================== */
/* Session List Tests                                                         */
/* ========================================================================== */

static void
test_session_manager_list_sessions_empty (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GList *sessions;

    manager = gdb_session_manager_new ();

    sessions = gdb_session_manager_list_sessions (manager);
    g_assert_null (sessions);
}

static void
test_session_manager_list_sessions (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *s1;
    GdbSession *s2;
    GdbSession *s3;
    GList *sessions;

    manager = gdb_session_manager_new ();

    s1 = gdb_session_manager_create_session (manager, NULL, NULL);
    s2 = gdb_session_manager_create_session (manager, NULL, NULL);
    s3 = gdb_session_manager_create_session (manager, NULL, NULL);

    sessions = gdb_session_manager_list_sessions (manager);
    g_assert_cmpuint (g_list_length (sessions), ==, 3);

    g_list_free_full (sessions, g_object_unref);
    g_object_unref (s1);
    g_object_unref (s2);
    g_object_unref (s3);
}


/* ========================================================================== */
/* Signal Tests                                                               */
/* ========================================================================== */

static void
on_session_added (GdbSessionManager *manager G_GNUC_UNUSED,
                  GdbSession        *session G_GNUC_UNUSED,
                  gpointer           user_data)
{
    gint *count = (gint *)user_data;
    (*count)++;
}

static void
test_session_manager_signal_added (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *session;
    gint add_count = 0;

    manager = gdb_session_manager_new ();

    g_signal_connect (manager, "session-added",
                      G_CALLBACK (on_session_added), &add_count);

    session = gdb_session_manager_create_session (manager, NULL, NULL);
    g_assert_cmpint (add_count, ==, 1);

    g_object_unref (session);
}

static void
on_session_removed (GdbSessionManager *manager G_GNUC_UNUSED,
                    const gchar       *session_id G_GNUC_UNUSED,
                    gpointer           user_data)
{
    gint *count = (gint *)user_data;
    (*count)++;
}

static void
test_session_manager_signal_removed (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *session;
    const gchar *id;
    gint remove_count = 0;

    manager = gdb_session_manager_new ();

    g_signal_connect (manager, "session-removed",
                      G_CALLBACK (on_session_removed), &remove_count);

    session = gdb_session_manager_create_session (manager, NULL, NULL);
    id = g_strdup (gdb_session_get_session_id (session));
    g_object_unref (session);

    gdb_session_manager_remove_session (manager, id);
    g_assert_cmpint (remove_count, ==, 1);

    g_free ((gchar *)id);
}


/* ========================================================================== */
/* Terminate All Tests                                                        */
/* ========================================================================== */

static void
test_session_manager_terminate_all (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *s1;
    GdbSession *s2;

    manager = gdb_session_manager_new ();

    s1 = gdb_session_manager_create_session (manager, NULL, NULL);
    s2 = gdb_session_manager_create_session (manager, NULL, NULL);

    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 2);

    gdb_session_manager_terminate_all (manager);

    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 0);

    g_object_unref (s1);
    g_object_unref (s2);
}


/* ========================================================================== */
/* Concurrent Sessions Tests                                                  */
/* ========================================================================== */

static void
test_session_manager_concurrent (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *sessions[10];
    gint i;

    manager = gdb_session_manager_new ();

    /* Create 10 sessions */
    for (i = 0; i < 10; i++)
    {
        sessions[i] = gdb_session_manager_create_session (manager, NULL, NULL);
        g_assert_nonnull (sessions[i]);
    }

    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 10);

    /* Verify all are unique */
    for (i = 0; i < 10; i++)
    {
        gint j;
        const gchar *id_i = gdb_session_get_session_id (sessions[i]);
        for (j = i + 1; j < 10; j++)
        {
            const gchar *id_j = gdb_session_get_session_id (sessions[j]);
            g_assert_cmpstr (id_i, !=, id_j);
        }
    }

    /* Cleanup */
    for (i = 0; i < 10; i++)
    {
        g_object_unref (sessions[i]);
    }
}


/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    /* Construction tests */
    g_test_add_func ("/gdb/session-manager/new", test_session_manager_new);
    g_test_add_func ("/gdb/session-manager/singleton", test_session_manager_singleton);

    /* Properties tests */
    g_test_add_func ("/gdb/session-manager/properties", test_session_manager_properties);

    /* Creation tests */
    g_test_add_func ("/gdb/session-manager/create-session", test_session_manager_create_session);
    g_test_add_func ("/gdb/session-manager/create-with-options", test_session_manager_create_with_options);

    /* Lookup tests */
    g_test_add_func ("/gdb/session-manager/get-session", test_session_manager_get_session);
    g_test_add_func ("/gdb/session-manager/get-session-not-found", test_session_manager_get_session_not_found);

    /* Removal tests */
    g_test_add_func ("/gdb/session-manager/remove-session", test_session_manager_remove_session);
    g_test_add_func ("/gdb/session-manager/remove-not-found", test_session_manager_remove_not_found);

    /* List tests */
    g_test_add_func ("/gdb/session-manager/list-sessions-empty", test_session_manager_list_sessions_empty);
    g_test_add_func ("/gdb/session-manager/list-sessions", test_session_manager_list_sessions);

    /* Signal tests */
    g_test_add_func ("/gdb/session-manager/signal-added", test_session_manager_signal_added);
    g_test_add_func ("/gdb/session-manager/signal-removed", test_session_manager_signal_removed);

    /* Terminate all */
    g_test_add_func ("/gdb/session-manager/terminate-all", test_session_manager_terminate_all);

    /* Concurrent */
    g_test_add_func ("/gdb/session-manager/concurrent", test_session_manager_concurrent);

    return g_test_run ();
}
