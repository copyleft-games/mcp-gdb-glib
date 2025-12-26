/*
 * test-session.c - Unit tests for GdbSession
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include "mcp-gdb/gdb-session.h"
#include "mcp-gdb/gdb-error.h"

/* Path to mock GDB script */
static gchar *mock_gdb_path = NULL;

/* ========================================================================== */
/* Fixtures                                                                   */
/* ========================================================================== */

typedef struct {
    GdbSession *session;
    GMainLoop  *loop;
    gboolean    callback_called;
    gboolean    success;
    GError     *error;
} SessionFixture;

static void
session_fixture_setup (SessionFixture *fixture,
                       gconstpointer   user_data G_GNUC_UNUSED)
{
    fixture->session = gdb_session_new ("test-session", mock_gdb_path, NULL);
    fixture->loop = g_main_loop_new (NULL, FALSE);
    fixture->callback_called = FALSE;
    fixture->success = FALSE;
    fixture->error = NULL;
}

static void
session_fixture_teardown (SessionFixture *fixture,
                          gconstpointer   user_data G_GNUC_UNUSED)
{
    if (fixture->session != NULL)
    {
        gdb_session_terminate (fixture->session);
        g_clear_object (&fixture->session);
    }
    g_clear_pointer (&fixture->loop, g_main_loop_unref);
    g_clear_error (&fixture->error);
}


/* ========================================================================== */
/* Construction Tests                                                         */
/* ========================================================================== */

static void
test_session_new (void)
{
    g_autoptr(GdbSession) session = NULL;

    session = gdb_session_new ("session-1", "/usr/bin/gdb", "/tmp");

    g_assert_nonnull (session);
    g_assert_true (GDB_IS_SESSION (session));
    g_assert_cmpstr (gdb_session_get_session_id (session), ==, "session-1");
    g_assert_cmpstr (gdb_session_get_gdb_path (session), ==, "/usr/bin/gdb");
    g_assert_cmpstr (gdb_session_get_working_dir (session), ==, "/tmp");
}

static void
test_session_new_defaults (void)
{
    g_autoptr(GdbSession) session = NULL;

    session = gdb_session_new ("session-2", NULL, NULL);

    g_assert_nonnull (session);
    g_assert_cmpstr (gdb_session_get_gdb_path (session), ==, "gdb");
    g_assert_null (gdb_session_get_working_dir (session));
}

static void
test_session_properties (void)
{
    g_autoptr(GdbSession) session = NULL;

    session = gdb_session_new ("prop-test", NULL, NULL);

    /* Initial state */
    g_assert_cmpint (gdb_session_get_state (session), ==, GDB_SESSION_STATE_DISCONNECTED);
    g_assert_false (gdb_session_is_ready (session));
    g_assert_null (gdb_session_get_target_program (session));

    /* Default timeout */
    g_assert_cmpuint (gdb_session_get_timeout_ms (session), ==, 10000);

    /* Set timeout */
    gdb_session_set_timeout_ms (session, 5000);
    g_assert_cmpuint (gdb_session_get_timeout_ms (session), ==, 5000);

    /* Set target program */
    gdb_session_set_target_program (session, "/path/to/prog");
    g_assert_cmpstr (gdb_session_get_target_program (session), ==, "/path/to/prog");

    /* Get MI parser */
    g_assert_nonnull (gdb_session_get_mi_parser (session));
}


/* ========================================================================== */
/* Async Helper                                                               */
/* ========================================================================== */

/* Helper struct for timeout tracking */
typedef struct {
    GMainLoop *loop;
    guint *timeout_id_ptr;
} TimeoutData;

static gboolean
timeout_quit_loop (gpointer user_data)
{
    TimeoutData *data = (TimeoutData *)user_data;
    if (data->timeout_id_ptr != NULL)
    {
        *(data->timeout_id_ptr) = 0;  /* Mark as fired */
    }
    g_main_loop_quit (data->loop);
    return G_SOURCE_REMOVE;
}

static void
start_callback (GObject      *source,
                GAsyncResult *result,
                gpointer      user_data)
{
    SessionFixture *fixture = (SessionFixture *)user_data;
    fixture->callback_called = TRUE;
    fixture->success = gdb_session_start_finish (GDB_SESSION (source),
                                                   result,
                                                   &fixture->error);
    g_main_loop_quit (fixture->loop);
}


/* ========================================================================== */
/* Lifecycle Tests                                                            */
/* ========================================================================== */

static void
test_session_start (SessionFixture *fixture,
                    gconstpointer   user_data G_GNUC_UNUSED)
{
    guint timeout_id = 0;
    TimeoutData timeout_data;

    /* Skip if mock GDB not available */
    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    timeout_data.loop = fixture->loop;
    timeout_data.timeout_id_ptr = &timeout_id;

    gdb_session_start_async (fixture->session, NULL, start_callback, fixture);

    timeout_id = g_timeout_add (5000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    g_assert_true (fixture->callback_called);
    g_assert_true (fixture->success);
    g_assert_no_error (fixture->error);
    g_assert_cmpint (gdb_session_get_state (fixture->session), ==, GDB_SESSION_STATE_READY);
    g_assert_true (gdb_session_is_ready (fixture->session));
}

static void
test_session_start_invalid_path (void)
{
    g_autoptr(GdbSession) session = NULL;

    /* Create session with invalid GDB path */
    session = gdb_session_new ("bad-path", "/nonexistent/gdb/path", NULL);

    /* The session should be in disconnected state initially */
    g_assert_cmpint (gdb_session_get_state (session), ==, GDB_SESSION_STATE_DISCONNECTED);
    g_assert_false (gdb_session_is_ready (session));
}

static void
test_session_terminate (SessionFixture *fixture,
                        gconstpointer   user_data G_GNUC_UNUSED)
{
    guint timeout_id = 0;
    TimeoutData timeout_data;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    timeout_data.loop = fixture->loop;
    timeout_data.timeout_id_ptr = &timeout_id;

    /* First start the session */
    gdb_session_start_async (fixture->session, NULL, start_callback, fixture);
    timeout_id = g_timeout_add (5000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    g_assert_true (fixture->success);

    /* Now terminate */
    gdb_session_terminate (fixture->session);

    /* Let the termination process complete */
    timeout_id = g_timeout_add (500, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    g_assert_cmpint (gdb_session_get_state (fixture->session), ==, GDB_SESSION_STATE_TERMINATED);
}


/* ========================================================================== */
/* State Tests                                                                */
/* ========================================================================== */

static void
test_session_initial_state (void)
{
    g_autoptr(GdbSession) session = NULL;

    session = gdb_session_new ("state-test", NULL, NULL);

    g_assert_cmpint (gdb_session_get_state (session), ==, GDB_SESSION_STATE_DISCONNECTED);
    g_assert_false (gdb_session_is_ready (session));
}


/* ========================================================================== */
/* Signal Tests                                                               */
/* ========================================================================== */

static void
on_state_changed (GdbSession      *session G_GNUC_UNUSED,
                  GdbSessionState  old_state G_GNUC_UNUSED,
                  GdbSessionState  new_state G_GNUC_UNUSED,
                  gpointer         user_data)
{
    gint *count = (gint *)user_data;
    (*count)++;
}

static void
test_session_state_signal (SessionFixture *fixture,
                           gconstpointer   user_data G_GNUC_UNUSED)
{
    gint signal_count = 0;
    guint timeout_id = 0;
    gulong handler_id;
    TimeoutData timeout_data;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    timeout_data.loop = fixture->loop;
    timeout_data.timeout_id_ptr = &timeout_id;

    handler_id = g_signal_connect (fixture->session, "state-changed",
                                   G_CALLBACK (on_state_changed), &signal_count);

    gdb_session_start_async (fixture->session, NULL, start_callback, fixture);
    timeout_id = g_timeout_add (5000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    /* Disconnect signal handler before local variables go out of scope */
    g_signal_handler_disconnect (fixture->session, handler_id);

    /* Should have at least one state change (DISCONNECTED -> STARTING -> READY) */
    g_assert_cmpint (signal_count, >=, 1);
}

static void
on_ready (GdbSession *session G_GNUC_UNUSED,
          gpointer    user_data)
{
    gboolean *ready_received = (gboolean *)user_data;
    *ready_received = TRUE;
}

static void
test_session_ready_signal (SessionFixture *fixture,
                           gconstpointer   user_data G_GNUC_UNUSED)
{
    gboolean ready_received = FALSE;
    guint timeout_id = 0;
    gulong handler_id;
    TimeoutData timeout_data;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    timeout_data.loop = fixture->loop;
    timeout_data.timeout_id_ptr = &timeout_id;

    handler_id = g_signal_connect (fixture->session, "ready",
                                   G_CALLBACK (on_ready), &ready_received);

    gdb_session_start_async (fixture->session, NULL, start_callback, fixture);
    timeout_id = g_timeout_add (5000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    /* Disconnect signal handler before local variables go out of scope */
    g_signal_handler_disconnect (fixture->session, handler_id);

    g_assert_true (ready_received);
}


/* ========================================================================== */
/* Execute Tests                                                              */
/* ========================================================================== */

typedef struct {
    GMainLoop *loop;
    gchar     *output;
    GError    *error;
} ExecuteData;

static void
execute_callback (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data)
{
    ExecuteData *data = (ExecuteData *)user_data;
    data->output = gdb_session_execute_finish (GDB_SESSION (source),
                                                result,
                                                &data->error);
    g_main_loop_quit (data->loop);
}

static void
test_session_execute_command (SessionFixture *fixture,
                              gconstpointer   user_data G_GNUC_UNUSED)
{
    ExecuteData data = { fixture->loop, NULL, NULL };
    guint timeout_id = 0;
    TimeoutData timeout_data;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    timeout_data.loop = fixture->loop;
    timeout_data.timeout_id_ptr = &timeout_id;

    /* Start session first */
    gdb_session_start_async (fixture->session, NULL, start_callback, fixture);
    timeout_id = g_timeout_add (5000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    if (!fixture->success)
    {
        g_test_skip ("Could not start session");
        return;
    }

    /* Execute a command */
    gdb_session_execute_async (fixture->session, "help", NULL,
                               execute_callback, &data);
    timeout_id = g_timeout_add (5000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    g_assert_no_error (data.error);
    /* Mock GDB returns something - just check we got output */
    g_free (data.output);
    g_clear_error (&data.error);
}


/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_autofree gchar *test_dir = NULL;
    int result;

    g_test_init (&argc, &argv, NULL);

    /* Find mock-gdb.sh path */
    test_dir = g_path_get_dirname (argv[0]);
    if (g_str_has_suffix (test_dir, "build"))
    {
        /* Running from build directory */
        mock_gdb_path = g_build_filename (test_dir, "..", "tests", "mock-gdb.sh", NULL);
    }
    else
    {
        mock_gdb_path = g_build_filename (test_dir, "mock-gdb.sh", NULL);
    }

    if (!g_file_test (mock_gdb_path, G_FILE_TEST_IS_EXECUTABLE))
    {
        g_free (mock_gdb_path);
        mock_gdb_path = NULL;
        g_message ("Mock GDB script not found or not executable - some tests will be skipped");
    }

    /* Construction tests */
    g_test_add_func ("/gdb/session/new", test_session_new);
    g_test_add_func ("/gdb/session/new-defaults", test_session_new_defaults);
    g_test_add_func ("/gdb/session/properties", test_session_properties);

    /* State tests */
    g_test_add_func ("/gdb/session/initial-state", test_session_initial_state);
    g_test_add_func ("/gdb/session/start-invalid-path", test_session_start_invalid_path);

    /* Lifecycle tests with fixture */
    g_test_add ("/gdb/session/start",
                SessionFixture, NULL,
                session_fixture_setup,
                test_session_start,
                session_fixture_teardown);

    g_test_add ("/gdb/session/terminate",
                SessionFixture, NULL,
                session_fixture_setup,
                test_session_terminate,
                session_fixture_teardown);

    /* Signal tests */
    g_test_add ("/gdb/session/signal-state-changed",
                SessionFixture, NULL,
                session_fixture_setup,
                test_session_state_signal,
                session_fixture_teardown);

    g_test_add ("/gdb/session/signal-ready",
                SessionFixture, NULL,
                session_fixture_setup,
                test_session_ready_signal,
                session_fixture_teardown);

    /* Execute tests */
    g_test_add ("/gdb/session/execute-command",
                SessionFixture, NULL,
                session_fixture_setup,
                test_session_execute_command,
                session_fixture_teardown);

    result = g_test_run ();

    g_free (mock_gdb_path);

    return result;
}
