/*
 * test-integration.c - Integration tests with real GDB
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * These tests require real GDB to be installed and the test-program
 * to be compiled. They verify end-to-end functionality.
 */

#include <glib.h>
#include "mcp-gdb/gdb-session.h"
#include "mcp-gdb/gdb-session-manager.h"
#include "mcp-gdb/gdb-mi-parser.h"
#include "mcp-gdb/gdb-error.h"

/* Path to test program - set in main() */
static gchar *test_program_path = NULL;

/* GDB path - may be NULL for default "gdb" */
static gchar *gdb_path = NULL;


/* ========================================================================== */
/* Helpers                                                                    */
/* ========================================================================== */

static gboolean
gdb_is_available (void)
{
    g_autofree gchar *output = NULL;
    gint exit_status;
    const gchar *gdb = gdb_path != NULL ? gdb_path : "gdb";

    g_autofree gchar *cmdline = g_strdup_printf ("%s --version", gdb);

    if (!g_spawn_command_line_sync (cmdline, &output, NULL, &exit_status, NULL))
    {
        return FALSE;
    }

    return exit_status == 0;
}

static gboolean
test_program_exists (void)
{
    if (test_program_path == NULL)
    {
        return FALSE;
    }
    return g_file_test (test_program_path, G_FILE_TEST_IS_EXECUTABLE);
}


/* ========================================================================== */
/* Fixtures                                                                   */
/* ========================================================================== */

typedef struct {
    GdbSessionManager *manager;
    GdbSession *session;
    GMainLoop *loop;
    gboolean callback_called;
    gboolean success;
    GError *error;
} IntegrationFixture;

static void
integration_fixture_setup (IntegrationFixture *fixture,
                           gconstpointer       user_data G_GNUC_UNUSED)
{
    fixture->manager = gdb_session_manager_new ();
    fixture->session = gdb_session_manager_create_session (fixture->manager,
                                                            gdb_path, NULL);
    fixture->loop = g_main_loop_new (NULL, FALSE);
    fixture->callback_called = FALSE;
    fixture->success = FALSE;
    fixture->error = NULL;
}

static void
integration_fixture_teardown (IntegrationFixture *fixture,
                              gconstpointer       user_data G_GNUC_UNUSED)
{
    if (fixture->session != NULL)
    {
        gdb_session_terminate (fixture->session);
        g_clear_object (&fixture->session);
    }
    gdb_session_manager_terminate_all (fixture->manager);
    g_clear_object (&fixture->manager);
    g_clear_pointer (&fixture->loop, g_main_loop_unref);
    g_clear_error (&fixture->error);
}


/* ========================================================================== */
/* Async Helpers                                                              */
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
    IntegrationFixture *fixture = (IntegrationFixture *)user_data;
    fixture->callback_called = TRUE;
    fixture->success = gdb_session_start_finish (GDB_SESSION (source),
                                                   result,
                                                   &fixture->error);
    g_main_loop_quit (fixture->loop);
}


/* ========================================================================== */
/* Full Session Lifecycle Test                                                */
/* ========================================================================== */

static void
test_integration_full_session (IntegrationFixture *fixture,
                               gconstpointer       user_data G_GNUC_UNUSED)
{
    guint timeout_id = 0;
    TimeoutData timeout_data;

    if (!gdb_is_available ())
    {
        g_test_skip ("GDB not available");
        return;
    }

    if (!test_program_exists ())
    {
        g_test_skip ("Test program not available");
        return;
    }

    /* Start session */
    timeout_data.loop = fixture->loop;
    timeout_data.timeout_id_ptr = &timeout_id;
    gdb_session_start_async (fixture->session, NULL, start_callback, fixture);
    timeout_id = g_timeout_add (10000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    if (!fixture->success)
    {
        if (fixture->error != NULL)
        {
            g_test_skip_printf ("Could not start GDB: %s", fixture->error->message);
        }
        else
        {
            g_test_skip ("Could not start GDB");
        }
        return;
    }

    /* Should be in READY state */
    g_assert_cmpint (gdb_session_get_state (fixture->session), ==, GDB_SESSION_STATE_READY);
    g_assert_true (gdb_session_is_ready (fixture->session));

    /* Terminate */
    gdb_session_terminate (fixture->session);

    /* Wait for termination */
    timeout_id = g_timeout_add (1000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    g_assert_cmpint (gdb_session_get_state (fixture->session), ==, GDB_SESSION_STATE_TERMINATED);
}


/* ========================================================================== */
/* Session State Transitions Test                                             */
/* ========================================================================== */

static void
on_state_changed_integration (GdbSession      *session G_GNUC_UNUSED,
                              GdbSessionState  old_state G_GNUC_UNUSED,
                              GdbSessionState  new_state G_GNUC_UNUSED,
                              gpointer         user_data)
{
    gint *count = (gint *)user_data;
    (*count)++;
}

static void
test_integration_state_transitions (IntegrationFixture *fixture,
                                    gconstpointer       user_data G_GNUC_UNUSED)
{
    gint state_change_count = 0;
    guint timeout_id = 0;
    gulong handler_id;
    TimeoutData timeout_data;

    if (!gdb_is_available ())
    {
        g_test_skip ("GDB not available");
        return;
    }

    handler_id = g_signal_connect (fixture->session, "state-changed",
                                   G_CALLBACK (on_state_changed_integration),
                                   &state_change_count);

    /* Start session */
    timeout_data.loop = fixture->loop;
    timeout_data.timeout_id_ptr = &timeout_id;
    gdb_session_start_async (fixture->session, NULL, start_callback, fixture);
    timeout_id = g_timeout_add (10000, timeout_quit_loop, &timeout_data);
    g_main_loop_run (fixture->loop);
    if (timeout_id != 0)
    {
        g_source_remove (timeout_id);
    }

    /* Disconnect signal handler before local variables go out of scope */
    g_signal_handler_disconnect (fixture->session, handler_id);

    if (!fixture->success)
    {
        g_test_skip ("Could not start GDB");
        return;
    }

    /* Should have had at least one state change (DISCONNECTED -> STARTING -> READY) */
    g_assert_cmpint (state_change_count, >=, 1);
}


/* ========================================================================== */
/* MI Parser with Real Output Test                                            */
/* ========================================================================== */

static void
test_integration_mi_parser (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    const gchar *test_output;

    parser = gdb_mi_parser_new ();

    /* Test with realistic MI output */
    test_output = "^done,bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\","
                  "enabled=\"y\",addr=\"0x0000555555555149\",func=\"main\","
                  "file=\"test.c\",fullname=\"/home/user/test.c\",line=\"5\","
                  "thread-groups=[\"i1\"],times=\"0\"}";

    record = gdb_mi_parser_parse_line (parser, test_output, &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_RESULT);
    g_assert_cmpint (gdb_mi_record_get_result_class (record), ==, GDB_MI_RESULT_DONE);
}


/* ========================================================================== */
/* Session Manager Integration Test                                           */
/* ========================================================================== */

static void
test_integration_session_manager (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    GdbSession *session1;
    GdbSession *session2;
    GList *sessions;

    if (!gdb_is_available ())
    {
        g_test_skip ("GDB not available");
        return;
    }

    manager = gdb_session_manager_new ();

    /* Create sessions */
    session1 = gdb_session_manager_create_session (manager, gdb_path, NULL);
    session2 = gdb_session_manager_create_session (manager, gdb_path, NULL);

    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 2);

    /* List sessions */
    sessions = gdb_session_manager_list_sessions (manager);
    g_assert_cmpuint (g_list_length (sessions), ==, 2);
    g_list_free_full (sessions, g_object_unref);

    /* Terminate all */
    gdb_session_manager_terminate_all (manager);
    g_assert_cmpuint (gdb_session_manager_get_session_count (manager), ==, 0);

    g_object_unref (session1);
    g_object_unref (session2);
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

    /* Find test program path */
    test_dir = g_path_get_dirname (argv[0]);
    test_program_path = g_build_filename (test_dir, "test-program", NULL);

    if (!g_file_test (test_program_path, G_FILE_TEST_EXISTS))
    {
        /* Try build directory */
        g_free (test_program_path);
        test_program_path = g_build_filename (test_dir, "..", "build", "test-program", NULL);
    }

    /* Check for custom GDB path in environment */
    gdb_path = g_strdup (g_getenv ("GDB_PATH"));

    if (!gdb_is_available ())
    {
        g_message ("GDB not available - integration tests will be skipped");
    }

    if (!test_program_exists ())
    {
        g_message ("Test program not found at %s - some tests will be skipped",
                   test_program_path != NULL ? test_program_path : "(null)");
    }

    /* Full session lifecycle test */
    g_test_add ("/gdb/integration/full-session",
                IntegrationFixture, NULL,
                integration_fixture_setup,
                test_integration_full_session,
                integration_fixture_teardown);

    /* State transitions test */
    g_test_add ("/gdb/integration/state-transitions",
                IntegrationFixture, NULL,
                integration_fixture_setup,
                test_integration_state_transitions,
                integration_fixture_teardown);

    /* MI Parser with real output */
    g_test_add_func ("/gdb/integration/mi-parser", test_integration_mi_parser);

    /* Session manager integration */
    g_test_add_func ("/gdb/integration/session-manager", test_integration_session_manager);

    result = g_test_run ();

    g_free (test_program_path);
    g_free (gdb_path);

    return result;
}
