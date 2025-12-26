/*
 * test-tools-breakpoint.c - Unit tests for breakpoint tools
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
} BreakpointFixture;

static void
breakpoint_fixture_setup (BreakpointFixture *fixture,
                          gconstpointer      user_data G_GNUC_UNUSED)
{
    fixture->manager = gdb_session_manager_new ();
    fixture->session = gdb_session_manager_create_session (fixture->manager, NULL, NULL);
    fixture->session_id = g_strdup (gdb_session_get_session_id (fixture->session));
}

static void
breakpoint_fixture_teardown (BreakpointFixture *fixture,
                             gconstpointer      user_data G_GNUC_UNUSED)
{
    g_free (fixture->session_id);
    g_clear_object (&fixture->session);
    gdb_session_manager_terminate_all (fixture->manager);
    g_clear_object (&fixture->manager);
}


/* ========================================================================== */
/* gdb_set_breakpoint Tests                                                   */
/* ========================================================================== */

static void
test_gdb_breakpoint_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "location", "main");

    result = gdb_tools_handle_gdb_set_breakpoint (NULL, "gdb_set_breakpoint",
                                                   arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_breakpoint_missing_location (BreakpointFixture *fixture,
                                      gconstpointer      user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", fixture->session_id);
    /* Missing "location" */

    result = gdb_tools_handle_gdb_set_breakpoint (NULL, "gdb_set_breakpoint",
                                                   arguments, fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_gdb_breakpoint_missing_session_id (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "location", "main");
    /* Missing "sessionId" */

    result = gdb_tools_handle_gdb_set_breakpoint (NULL, "gdb_set_breakpoint",
                                                   arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}


/* ========================================================================== */
/* Schema Tests                                                               */
/* ========================================================================== */

static void
test_gdb_breakpoint_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_breakpoint_schema ();

    g_assert_nonnull (schema);
    g_assert_true (JSON_NODE_HOLDS_OBJECT (schema));

    obj = json_node_get_object (schema);
    g_assert_true (json_object_has_member (obj, "properties"));

    props = json_object_get_object_member (obj, "properties");
    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "location"));
}

static void
test_gdb_breakpoint_schema_has_condition (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_breakpoint_schema ();

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    /* Should have optional condition parameter */
    g_assert_true (json_object_has_member (props, "condition"));
}

static void
test_gdb_breakpoint_schema_required (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonArray *required;
    guint len;
    guint i;
    gboolean has_session_id = FALSE;
    gboolean has_location = FALSE;

    schema = gdb_tools_create_gdb_breakpoint_schema ();

    obj = json_node_get_object (schema);
    g_assert_true (json_object_has_member (obj, "required"));

    required = json_object_get_array_member (obj, "required");
    len = json_array_get_length (required);

    /* Check that sessionId and location are required */
    for (i = 0; i < len; i++)
    {
        const gchar *name = json_array_get_string_element (required, i);
        if (g_strcmp0 (name, "sessionId") == 0)
        {
            has_session_id = TRUE;
        }
        else if (g_strcmp0 (name, "location") == 0)
        {
            has_location = TRUE;
        }
    }

    g_assert_true (has_session_id);
    g_assert_true (has_location);
}


/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    /* gdb_set_breakpoint tests */
    g_test_add_func ("/gdb/tools/breakpoint/missing-session", test_gdb_breakpoint_missing_session);
    g_test_add ("/gdb/tools/breakpoint/missing-location",
                BreakpointFixture, NULL,
                breakpoint_fixture_setup,
                test_gdb_breakpoint_missing_location,
                breakpoint_fixture_teardown);
    g_test_add_func ("/gdb/tools/breakpoint/missing-session-id", test_gdb_breakpoint_missing_session_id);

    /* Schema tests */
    g_test_add_func ("/gdb/tools/breakpoint/schema", test_gdb_breakpoint_schema);
    g_test_add_func ("/gdb/tools/breakpoint/schema-has-condition", test_gdb_breakpoint_schema_has_condition);
    g_test_add_func ("/gdb/tools/breakpoint/schema-required", test_gdb_breakpoint_schema_required);

    return g_test_run ();
}
