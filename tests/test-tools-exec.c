/*
 * test-tools-exec.c - Unit tests for execution control tools
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include <mcp.h>
#include "mcp-gdb/gdb-session-manager.h"
#include "src/tools/gdb-tools-internal.h"


/* ========================================================================== */
/* gdb_continue Tests                                                         */
/* ========================================================================== */

static void
test_gdb_continue_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");

    result = gdb_tools_handle_gdb_continue (NULL, "gdb_continue", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_continue_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    /* No sessionId */
    result = gdb_tools_handle_gdb_continue (NULL, "gdb_continue", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* gdb_step Tests                                                             */
/* ========================================================================== */

static void
test_gdb_step_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");

    result = gdb_tools_handle_gdb_step (NULL, "gdb_step", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_step_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    /* No sessionId */
    result = gdb_tools_handle_gdb_step (NULL, "gdb_step", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_step_schema_instruction (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_step_schema ();

    g_assert_nonnull (schema);
    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    /* Should have instructions option */
    g_assert_true (json_object_has_member (props, "instructions"));
}


/* ========================================================================== */
/* gdb_next Tests                                                             */
/* ========================================================================== */

static void
test_gdb_next_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");

    result = gdb_tools_handle_gdb_next (NULL, "gdb_next", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_next_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    /* No sessionId */
    result = gdb_tools_handle_gdb_next (NULL, "gdb_next", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_next_schema_instruction (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_next_schema ();

    g_assert_nonnull (schema);
    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    /* Should have instructions option */
    g_assert_true (json_object_has_member (props, "instructions"));
}


/* ========================================================================== */
/* gdb_finish Tests                                                           */
/* ========================================================================== */

static void
test_gdb_finish_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");

    result = gdb_tools_handle_gdb_finish (NULL, "gdb_finish", arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_finish_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    /* No sessionId */
    result = gdb_tools_handle_gdb_finish (NULL, "gdb_finish", arguments, manager);

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

    /* gdb_continue tests */
    g_test_add_func ("/gdb/tools/exec/continue-missing-session", test_gdb_continue_missing_session);
    g_test_add_func ("/gdb/tools/exec/continue-missing-session-id", test_gdb_continue_missing_session_id);

    /* gdb_step tests */
    g_test_add_func ("/gdb/tools/exec/step-missing-session", test_gdb_step_missing_session);
    g_test_add_func ("/gdb/tools/exec/step-missing-session-id", test_gdb_step_missing_session_id);
    g_test_add_func ("/gdb/tools/exec/step-schema-instruction", test_gdb_step_schema_instruction);

    /* gdb_next tests */
    g_test_add_func ("/gdb/tools/exec/next-missing-session", test_gdb_next_missing_session);
    g_test_add_func ("/gdb/tools/exec/next-missing-session-id", test_gdb_next_missing_session_id);
    g_test_add_func ("/gdb/tools/exec/next-schema-instruction", test_gdb_next_schema_instruction);

    /* gdb_finish tests */
    g_test_add_func ("/gdb/tools/exec/finish-missing-session", test_gdb_finish_missing_session);
    g_test_add_func ("/gdb/tools/exec/finish-missing-session-id", test_gdb_finish_missing_session_id);

    return g_test_run ();
}
