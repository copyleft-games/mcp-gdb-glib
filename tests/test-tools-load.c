/*
 * test-tools-load.c - Unit tests for program loading tools
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include <mcp/mcp.h>
#include "mcp-gdb/gdb-session-manager.h"
#include "src/tools/gdb-tools-internal.h"


/* ========================================================================== */
/* Fixture                                                                    */
/* ========================================================================== */

typedef struct {
    GdbSessionManager *manager;
    GdbSession *session;
    gchar *session_id;
} LoadToolsFixture;

static void
load_fixture_setup (LoadToolsFixture *fixture,
                    gconstpointer     user_data G_GNUC_UNUSED)
{
    fixture->manager = gdb_session_manager_new ();
    fixture->session = gdb_session_manager_create_session (fixture->manager, NULL, NULL);
    fixture->session_id = g_strdup (gdb_session_get_session_id (fixture->session));
}

static void
load_fixture_teardown (LoadToolsFixture *fixture,
                       gconstpointer     user_data G_GNUC_UNUSED)
{
    g_free (fixture->session_id);
    g_clear_object (&fixture->session);
    gdb_session_manager_terminate_all (fixture->manager);
    g_clear_object (&fixture->manager);
}


/* ========================================================================== */
/* gdb_load Tests                                                             */
/* ========================================================================== */

static void
test_gdb_load_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "program", "/bin/true");

    result = gdb_tools_handle_gdb_load (NULL, "gdb_load", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_load_missing_program (LoadToolsFixture *fixture,
                               gconstpointer     user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", fixture->session_id);
    /* Missing "program" parameter */

    result = gdb_tools_handle_gdb_load (NULL, "gdb_load", arguments,
                                         fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_load_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "program", "/bin/true");
    /* Missing "sessionId" */

    result = gdb_tools_handle_gdb_load (NULL, "gdb_load", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* gdb_attach Tests                                                           */
/* ========================================================================== */

static void
test_gdb_attach_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_int_member (arguments, "pid", 12345);

    result = gdb_tools_handle_gdb_attach (NULL, "gdb_attach", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_attach_missing_pid (LoadToolsFixture *fixture,
                             gconstpointer     user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", fixture->session_id);
    /* Missing "pid" */

    result = gdb_tools_handle_gdb_attach (NULL, "gdb_attach", arguments,
                                           fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_attach_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_int_member (arguments, "pid", 12345);
    /* Missing "sessionId" */

    result = gdb_tools_handle_gdb_attach (NULL, "gdb_attach", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* gdb_load_core Tests                                                        */
/* ========================================================================== */

static void
test_gdb_load_core_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "coreFile", "/tmp/core");

    result = gdb_tools_handle_gdb_load_core (NULL, "gdb_load_core", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_load_core_missing_core_file (LoadToolsFixture *fixture,
                                      gconstpointer     user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", fixture->session_id);
    /* Missing "coreFile" */

    result = gdb_tools_handle_gdb_load_core (NULL, "gdb_load_core", arguments,
                                              fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_load_core_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "coreFile", "/tmp/core");
    /* Missing "sessionId" */

    result = gdb_tools_handle_gdb_load_core (NULL, "gdb_load_core", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    /* gdb_load tests */
    g_test_add_func ("/gdb/tools/load/load-missing-session", test_gdb_load_missing_session);
    g_test_add ("/gdb/tools/load/load-missing-program",
                LoadToolsFixture, NULL,
                load_fixture_setup,
                test_gdb_load_missing_program,
                load_fixture_teardown);
    g_test_add_func ("/gdb/tools/load/load-missing-session-id", test_gdb_load_missing_session_id);

    /* gdb_attach tests */
    g_test_add_func ("/gdb/tools/load/attach-missing-session", test_gdb_attach_missing_session);
    g_test_add ("/gdb/tools/load/attach-missing-pid",
                LoadToolsFixture, NULL,
                load_fixture_setup,
                test_gdb_attach_missing_pid,
                load_fixture_teardown);
    g_test_add_func ("/gdb/tools/load/attach-missing-session-id", test_gdb_attach_missing_session_id);

    /* gdb_load_core tests */
    g_test_add_func ("/gdb/tools/load/load-core-missing-session", test_gdb_load_core_missing_session);
    g_test_add ("/gdb/tools/load/load-core-missing-file",
                LoadToolsFixture, NULL,
                load_fixture_setup,
                test_gdb_load_core_missing_core_file,
                load_fixture_teardown);
    g_test_add_func ("/gdb/tools/load/load-core-missing-session-id", test_gdb_load_core_missing_session_id);

    return g_test_run ();
}
