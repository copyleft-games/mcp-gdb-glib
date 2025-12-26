/*
 * test-tools-session.c - Unit tests for session management tools
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include <mcp/mcp.h>
#include "mcp-gdb/gdb-session-manager.h"
#include "src/tools/gdb-tools-internal.h"


/* Path to mock GDB script */
static gchar *mock_gdb_path = NULL;


/* ========================================================================== */
/* Fixture                                                                    */
/* ========================================================================== */

typedef struct {
    GdbSessionManager *manager;
} ToolsFixture;

static void
tools_fixture_setup (ToolsFixture  *fixture,
                     gconstpointer  user_data G_GNUC_UNUSED)
{
    fixture->manager = gdb_session_manager_new ();

    /* Set default GDB path to mock GDB for testing */
    if (mock_gdb_path != NULL)
    {
        gdb_session_manager_set_default_gdb_path (fixture->manager, mock_gdb_path);
    }
}

static void
tools_fixture_teardown (ToolsFixture  *fixture,
                        gconstpointer  user_data G_GNUC_UNUSED)
{
    gdb_session_manager_terminate_all (fixture->manager);
    g_clear_object (&fixture->manager);
}


/* ========================================================================== */
/* gdb_start Tests                                                            */
/* ========================================================================== */

static void
test_gdb_start (ToolsFixture  *fixture,
                gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    /* gdb_start with no arguments should create a session */
    result = gdb_tools_handle_gdb_start (NULL, "gdb_start", arguments,
                                          fixture->manager);

    g_assert_nonnull (result);
    g_assert_false (mcp_tool_result_get_is_error (result));

    /* Should have created a session */
    g_assert_cmpuint (gdb_session_manager_get_session_count (fixture->manager), ==, 1);
}

static void
test_gdb_start_with_gdb_path (ToolsFixture  *fixture,
                              gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;
    GList *sessions;
    GdbSession *session;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    /* Use mock GDB path as the "custom" path - tests that gdbPath is respected */
    json_object_set_string_member (arguments, "gdbPath", mock_gdb_path);

    result = gdb_tools_handle_gdb_start (NULL, "gdb_start", arguments,
                                          fixture->manager);

    g_assert_nonnull (result);
    g_assert_false (mcp_tool_result_get_is_error (result));

    /* Get the created session and verify gdb path */
    sessions = gdb_session_manager_list_sessions (fixture->manager);
    g_assert_nonnull (sessions);
    session = GDB_SESSION (sessions->data);
    g_assert_cmpstr (gdb_session_get_gdb_path (session), ==, mock_gdb_path);

    g_list_free_full (sessions, g_object_unref);
}

static void
test_gdb_start_with_working_dir (ToolsFixture  *fixture,
                                 gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;
    GList *sessions;
    GdbSession *session;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    /* Use /tmp which always exists */
    json_object_set_string_member (arguments, "workingDir", "/tmp");

    result = gdb_tools_handle_gdb_start (NULL, "gdb_start", arguments,
                                          fixture->manager);

    g_assert_nonnull (result);
    g_assert_false (mcp_tool_result_get_is_error (result));

    /* Get the created session and verify working dir */
    sessions = gdb_session_manager_list_sessions (fixture->manager);
    g_assert_nonnull (sessions);
    session = GDB_SESSION (sessions->data);
    g_assert_cmpstr (gdb_session_get_working_dir (session), ==, "/tmp");

    g_list_free_full (sessions, g_object_unref);
}

static void
test_gdb_start_multiple (ToolsFixture  *fixture,
                         gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    McpToolResult *result1;
    McpToolResult *result2;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    result1 = gdb_tools_handle_gdb_start (NULL, "gdb_start", arguments,
                                           fixture->manager);
    result2 = gdb_tools_handle_gdb_start (NULL, "gdb_start", arguments,
                                           fixture->manager);

    g_assert_nonnull (result1);
    g_assert_nonnull (result2);
    g_assert_false (mcp_tool_result_get_is_error (result1));
    g_assert_false (mcp_tool_result_get_is_error (result2));

    g_assert_cmpuint (gdb_session_manager_get_session_count (fixture->manager), ==, 2);

    mcp_tool_result_unref (result1);
    mcp_tool_result_unref (result2);
}


/* ========================================================================== */
/* gdb_terminate Tests                                                        */
/* ========================================================================== */

static void
test_gdb_terminate (ToolsFixture  *fixture,
                    gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) start_args = json_object_new ();
    g_autoptr(JsonObject) term_args = json_object_new ();
    g_autoptr(McpToolResult) start_result = NULL;
    g_autoptr(McpToolResult) term_result = NULL;
    GList *sessions;
    GdbSession *session;
    const gchar *session_id;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    /* First create a session */
    start_result = gdb_tools_handle_gdb_start (NULL, "gdb_start", start_args,
                                                fixture->manager);
    g_assert_false (mcp_tool_result_get_is_error (start_result));

    /* Get the session ID */
    sessions = gdb_session_manager_list_sessions (fixture->manager);
    session = GDB_SESSION (sessions->data);
    session_id = gdb_session_get_session_id (session);

    json_object_set_string_member (term_args, "sessionId", session_id);

    g_list_free_full (sessions, g_object_unref);

    /* Now terminate */
    term_result = gdb_tools_handle_gdb_terminate (NULL, "gdb_terminate", term_args,
                                                   fixture->manager);

    g_assert_nonnull (term_result);
    g_assert_false (mcp_tool_result_get_is_error (term_result));
}

static void
test_gdb_terminate_not_found (ToolsFixture  *fixture,
                              gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent-session");

    result = gdb_tools_handle_gdb_terminate (NULL, "gdb_terminate", arguments,
                                              fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_terminate_missing_session_id (ToolsFixture  *fixture,
                                       gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    /* No sessionId in arguments */
    result = gdb_tools_handle_gdb_terminate (NULL, "gdb_terminate", arguments,
                                              fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* gdb_list_sessions Tests                                                    */
/* ========================================================================== */

static void
test_gdb_list_sessions_empty (ToolsFixture  *fixture,
                              gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    result = gdb_tools_handle_gdb_list_sessions (NULL, "gdb_list_sessions",
                                                  arguments, fixture->manager);

    g_assert_nonnull (result);
    g_assert_false (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_list_sessions (ToolsFixture  *fixture,
                        gconstpointer  user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) start_args = json_object_new ();
    g_autoptr(JsonObject) list_args = json_object_new ();
    g_autoptr(McpToolResult) list_result = NULL;
    McpToolResult *start1;
    McpToolResult *start2;

    if (mock_gdb_path == NULL)
    {
        g_test_skip ("Mock GDB not available");
        return;
    }

    /* Create two sessions */
    start1 = gdb_tools_handle_gdb_start (NULL, "gdb_start", start_args,
                                          fixture->manager);
    start2 = gdb_tools_handle_gdb_start (NULL, "gdb_start", start_args,
                                          fixture->manager);

    g_assert_false (mcp_tool_result_get_is_error (start1));
    g_assert_false (mcp_tool_result_get_is_error (start2));

    mcp_tool_result_unref (start1);
    mcp_tool_result_unref (start2);

    /* Now list sessions */
    list_result = gdb_tools_handle_gdb_list_sessions (NULL, "gdb_list_sessions",
                                                       list_args, fixture->manager);

    g_assert_nonnull (list_result);
    g_assert_false (mcp_tool_result_get_is_error (list_result));
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
    else
    {
        /* Convert to absolute path for tests that change working directory */
        g_autofree gchar *relative_path = mock_gdb_path;
        mock_gdb_path = g_canonicalize_filename (relative_path, NULL);
    }

    /* gdb_start tests */
    g_test_add ("/gdb/tools/session/start",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_start,
                tools_fixture_teardown);

    g_test_add ("/gdb/tools/session/start-with-gdb-path",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_start_with_gdb_path,
                tools_fixture_teardown);

    g_test_add ("/gdb/tools/session/start-with-working-dir",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_start_with_working_dir,
                tools_fixture_teardown);

    g_test_add ("/gdb/tools/session/start-multiple",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_start_multiple,
                tools_fixture_teardown);

    /* gdb_terminate tests */
    g_test_add ("/gdb/tools/session/terminate",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_terminate,
                tools_fixture_teardown);

    g_test_add ("/gdb/tools/session/terminate-not-found",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_terminate_not_found,
                tools_fixture_teardown);

    g_test_add ("/gdb/tools/session/terminate-missing-session-id",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_terminate_missing_session_id,
                tools_fixture_teardown);

    /* gdb_list_sessions tests */
    g_test_add ("/gdb/tools/session/list-sessions-empty",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_list_sessions_empty,
                tools_fixture_teardown);

    g_test_add ("/gdb/tools/session/list-sessions",
                ToolsFixture, NULL,
                tools_fixture_setup,
                test_gdb_list_sessions,
                tools_fixture_teardown);

    result = g_test_run ();

    g_free (mock_gdb_path);

    return result;
}
