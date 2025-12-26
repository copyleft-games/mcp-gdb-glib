/*
 * test-tools-inspect.c - Unit tests for inspection tools
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
} InspectFixture;

static void
inspect_fixture_setup (InspectFixture *fixture,
                       gconstpointer   user_data G_GNUC_UNUSED)
{
    fixture->manager = gdb_session_manager_new ();
    fixture->session = gdb_session_manager_create_session (fixture->manager, NULL, NULL);
    fixture->session_id = g_strdup (gdb_session_get_session_id (fixture->session));
}

static void
inspect_fixture_teardown (InspectFixture *fixture,
                          gconstpointer   user_data G_GNUC_UNUSED)
{
    g_free (fixture->session_id);
    g_clear_object (&fixture->session);
    gdb_session_manager_terminate_all (fixture->manager);
    g_clear_object (&fixture->manager);
}


/* ========================================================================== */
/* gdb_backtrace Tests                                                        */
/* ========================================================================== */

static void
test_gdb_backtrace_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");

    result = gdb_tools_handle_gdb_backtrace (NULL, "gdb_backtrace", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_backtrace_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    /* No sessionId */
    result = gdb_tools_handle_gdb_backtrace (NULL, "gdb_backtrace", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_backtrace_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_backtrace_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
}


/* ========================================================================== */
/* gdb_print Tests                                                            */
/* ========================================================================== */

static void
test_gdb_print_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "expression", "x");

    result = gdb_tools_handle_gdb_print (NULL, "gdb_print", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_print_missing_expression (InspectFixture *fixture,
                                   gconstpointer   user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", fixture->session_id);
    /* Missing "expression" */

    result = gdb_tools_handle_gdb_print (NULL, "gdb_print", arguments,
                                          fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_print_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "expression", "x");
    /* Missing sessionId */

    result = gdb_tools_handle_gdb_print (NULL, "gdb_print", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* gdb_examine Tests                                                          */
/* ========================================================================== */

static void
test_gdb_examine_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "expression", "0x12345678");

    result = gdb_tools_handle_gdb_examine (NULL, "gdb_examine", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_examine_missing_expression (InspectFixture *fixture,
                                     gconstpointer   user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", fixture->session_id);
    /* Missing "expression" */

    result = gdb_tools_handle_gdb_examine (NULL, "gdb_examine", arguments,
                                            fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_examine_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_examine_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "expression"));
}


/* ========================================================================== */
/* gdb_info_registers Tests                                                   */
/* ========================================================================== */

static void
test_gdb_info_registers_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");

    result = gdb_tools_handle_gdb_info_registers (NULL, "gdb_info_registers",
                                                   arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_info_registers_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    /* No sessionId */
    result = gdb_tools_handle_gdb_info_registers (NULL, "gdb_info_registers",
                                                   arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_info_registers_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_info_registers_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
}


/* ========================================================================== */
/* gdb_command Tests                                                          */
/* ========================================================================== */

static void
test_gdb_command_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "command", "info threads");

    result = gdb_tools_handle_gdb_command (NULL, "gdb_command", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_command_missing_command (InspectFixture *fixture,
                                  gconstpointer   user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", fixture->session_id);
    /* Missing "command" */

    result = gdb_tools_handle_gdb_command (NULL, "gdb_command", arguments,
                                            fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_command_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "command", "info threads");
    /* Missing sessionId */

    result = gdb_tools_handle_gdb_command (NULL, "gdb_command", arguments, manager);

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

    /* gdb_backtrace tests */
    g_test_add_func ("/gdb/tools/inspect/backtrace-missing-session", test_gdb_backtrace_missing_session);
    g_test_add_func ("/gdb/tools/inspect/backtrace-missing-session-id", test_gdb_backtrace_missing_session_id);
    g_test_add_func ("/gdb/tools/inspect/backtrace-schema", test_gdb_backtrace_schema);

    /* gdb_print tests */
    g_test_add_func ("/gdb/tools/inspect/print-missing-session", test_gdb_print_missing_session);
    g_test_add ("/gdb/tools/inspect/print-missing-expression",
                InspectFixture, NULL,
                inspect_fixture_setup,
                test_gdb_print_missing_expression,
                inspect_fixture_teardown);
    g_test_add_func ("/gdb/tools/inspect/print-missing-session-id", test_gdb_print_missing_session_id);

    /* gdb_examine tests */
    g_test_add_func ("/gdb/tools/inspect/examine-missing-session", test_gdb_examine_missing_session);
    g_test_add ("/gdb/tools/inspect/examine-missing-expression",
                InspectFixture, NULL,
                inspect_fixture_setup,
                test_gdb_examine_missing_expression,
                inspect_fixture_teardown);
    g_test_add_func ("/gdb/tools/inspect/examine-schema", test_gdb_examine_schema);

    /* gdb_info_registers tests */
    g_test_add_func ("/gdb/tools/inspect/info-registers-missing-session", test_gdb_info_registers_missing_session);
    g_test_add_func ("/gdb/tools/inspect/info-registers-missing-session-id", test_gdb_info_registers_missing_session_id);
    g_test_add_func ("/gdb/tools/inspect/info-registers-schema", test_gdb_info_registers_schema);

    /* gdb_command tests */
    g_test_add_func ("/gdb/tools/inspect/command-missing-session", test_gdb_command_missing_session);
    g_test_add ("/gdb/tools/inspect/command-missing-command",
                InspectFixture, NULL,
                inspect_fixture_setup,
                test_gdb_command_missing_command,
                inspect_fixture_teardown);
    g_test_add_func ("/gdb/tools/inspect/command-missing-session-id", test_gdb_command_missing_session_id);

    return g_test_run ();
}
