/*
 * test-tools-common.c - Unit tests for common tool utilities
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
/* Create Error Result Tests                                                  */
/* ========================================================================== */

static void
test_create_error_result (void)
{
    g_autoptr(McpToolResult) result = NULL;

    result = gdb_tools_create_error_result ("Test error message");

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_create_error_result_formatted (void)
{
    g_autoptr(McpToolResult) result = NULL;

    result = gdb_tools_create_error_result ("Error: %s (%d)", "test", 42);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* Create Success Result Tests                                                */
/* ========================================================================== */

static void
test_create_success_result (void)
{
    g_autoptr(McpToolResult) result = NULL;

    result = gdb_tools_create_success_result ("Test success message");

    g_assert_nonnull (result);
    g_assert_false (mcp_tool_result_get_is_error (result));
}

static void
test_create_success_result_formatted (void)
{
    g_autoptr(McpToolResult) result = NULL;

    result = gdb_tools_create_success_result ("Result: %s = %d", "value", 100);

    g_assert_nonnull (result);
    g_assert_false (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* Get Session ID Tests                                                       */
/* ========================================================================== */

static void
test_get_session_id (void)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    const gchar *id;

    json_object_set_string_member (arguments, "sessionId", "test-session-123");

    id = gdb_tools_get_session_id (arguments);

    g_assert_cmpstr (id, ==, "test-session-123");
}

static void
test_get_session_id_null_arguments (void)
{
    const gchar *id;

    id = gdb_tools_get_session_id (NULL);

    g_assert_null (id);
}

static void
test_get_session_id_missing (void)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    const gchar *id;

    /* Add some other member, but not sessionId */
    json_object_set_string_member (arguments, "otherParam", "value");

    id = gdb_tools_get_session_id (arguments);

    g_assert_null (id);
}

static void
test_get_session_id_empty (void)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    const gchar *id;

    json_object_set_string_member (arguments, "sessionId", "");

    id = gdb_tools_get_session_id (arguments);

    /* Should return empty string, not NULL */
    g_assert_nonnull (id);
    g_assert_cmpstr (id, ==, "");
}


/* ========================================================================== */
/* Get Session Tests                                                          */
/* ========================================================================== */

static void
test_get_session_not_found (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    g_autoptr(JsonObject) arguments = json_object_new ();
    McpToolResult *error_result = NULL;
    GdbSession *session;

    manager = gdb_session_manager_new ();
    json_object_set_string_member (arguments, "sessionId", "nonexistent");

    session = gdb_tools_get_session (manager, arguments, &error_result);

    g_assert_null (session);
    g_assert_nonnull (error_result);
    g_assert_true (mcp_tool_result_get_is_error (error_result));

    mcp_tool_result_unref (error_result);
}

static void
test_get_session_missing_id (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    g_autoptr(JsonObject) arguments = json_object_new ();
    McpToolResult *error_result = NULL;
    GdbSession *session;

    manager = gdb_session_manager_new ();
    /* Don't add sessionId */

    session = gdb_tools_get_session (manager, arguments, &error_result);

    g_assert_null (session);
    g_assert_nonnull (error_result);
    g_assert_true (mcp_tool_result_get_is_error (error_result));

    mcp_tool_result_unref (error_result);
}

static void
test_get_session_null_error_result (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    g_autoptr(JsonObject) arguments = json_object_new ();
    GdbSession *session;

    manager = gdb_session_manager_new ();
    json_object_set_string_member (arguments, "sessionId", "nonexistent");

    /* Pass NULL for error_result - should not crash */
    session = gdb_tools_get_session (manager, arguments, NULL);

    g_assert_null (session);
}

static void
test_get_session_found (void)
{
    g_autoptr(GdbSessionManager) manager = NULL;
    g_autoptr(JsonObject) arguments = json_object_new ();
    GdbSession *created_session;
    GdbSession *found_session;
    const gchar *session_id;
    McpToolResult *error_result = NULL;

    manager = gdb_session_manager_new ();

    /* Create a session */
    created_session = gdb_session_manager_create_session (manager, NULL, NULL);
    session_id = gdb_session_get_session_id (created_session);

    /* Add the session ID to arguments */
    json_object_set_string_member (arguments, "sessionId", session_id);

    /* Should find the session */
    found_session = gdb_tools_get_session (manager, arguments, &error_result);

    g_assert_nonnull (found_session);
    g_assert_null (error_result);
    g_assert_true (found_session == created_session);

    g_object_unref (created_session);
}


/* ========================================================================== */
/* Schema Tests                                                               */
/* ========================================================================== */

static void
test_schema_gdb_start (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;

    schema = gdb_tools_create_gdb_start_schema ();

    g_assert_nonnull (schema);
    g_assert_true (JSON_NODE_HOLDS_OBJECT (schema));

    obj = json_node_get_object (schema);
    g_assert_true (json_object_has_member (obj, "type"));
    g_assert_cmpstr (json_object_get_string_member (obj, "type"), ==, "object");
    g_assert_true (json_object_has_member (obj, "properties"));
}

static void
test_schema_session_id_only (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_session_id_only_schema ();

    g_assert_nonnull (schema);
    g_assert_true (JSON_NODE_HOLDS_OBJECT (schema));

    obj = json_node_get_object (schema);
    g_assert_true (json_object_has_member (obj, "properties"));

    props = json_object_get_object_member (obj, "properties");
    g_assert_true (json_object_has_member (props, "sessionId"));
}

static void
test_schema_gdb_load (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_load_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "program"));
}

static void
test_schema_gdb_attach (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_attach_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "pid"));
}

static void
test_schema_gdb_breakpoint (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_breakpoint_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "location"));
}

static void
test_schema_gdb_print (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_print_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "expression"));
}

static void
test_schema_gdb_examine (void)
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

static void
test_schema_gdb_command (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_command_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "command"));
}


/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    /* Error result tests */
    g_test_add_func ("/gdb/tools/common/create-error-result", test_create_error_result);
    g_test_add_func ("/gdb/tools/common/create-error-result-formatted", test_create_error_result_formatted);

    /* Success result tests */
    g_test_add_func ("/gdb/tools/common/create-success-result", test_create_success_result);
    g_test_add_func ("/gdb/tools/common/create-success-result-formatted", test_create_success_result_formatted);

    /* Get session ID tests */
    g_test_add_func ("/gdb/tools/common/get-session-id", test_get_session_id);
    g_test_add_func ("/gdb/tools/common/get-session-id-null", test_get_session_id_null_arguments);
    g_test_add_func ("/gdb/tools/common/get-session-id-missing", test_get_session_id_missing);
    g_test_add_func ("/gdb/tools/common/get-session-id-empty", test_get_session_id_empty);

    /* Get session tests */
    g_test_add_func ("/gdb/tools/common/get-session-not-found", test_get_session_not_found);
    g_test_add_func ("/gdb/tools/common/get-session-missing-id", test_get_session_missing_id);
    g_test_add_func ("/gdb/tools/common/get-session-null-error", test_get_session_null_error_result);
    g_test_add_func ("/gdb/tools/common/get-session-found", test_get_session_found);

    /* Schema tests */
    g_test_add_func ("/gdb/tools/common/schema-gdb-start", test_schema_gdb_start);
    g_test_add_func ("/gdb/tools/common/schema-session-id-only", test_schema_session_id_only);
    g_test_add_func ("/gdb/tools/common/schema-gdb-load", test_schema_gdb_load);
    g_test_add_func ("/gdb/tools/common/schema-gdb-attach", test_schema_gdb_attach);
    g_test_add_func ("/gdb/tools/common/schema-gdb-breakpoint", test_schema_gdb_breakpoint);
    g_test_add_func ("/gdb/tools/common/schema-gdb-print", test_schema_gdb_print);
    g_test_add_func ("/gdb/tools/common/schema-gdb-examine", test_schema_gdb_examine);
    g_test_add_func ("/gdb/tools/common/schema-gdb-command", test_schema_gdb_command);

    return g_test_run ();
}
