/*
 * test-tools-glib.c - Unit tests for GLib-specific tools
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
} GlibToolsFixture;

static void
glib_fixture_setup (GlibToolsFixture *fixture,
                    gconstpointer     user_data G_GNUC_UNUSED)
{
    fixture->manager = gdb_session_manager_new ();
    fixture->session = gdb_session_manager_create_session (fixture->manager, NULL, NULL);
    fixture->session_id = g_strdup (gdb_session_get_session_id (fixture->session));
}

static void
glib_fixture_teardown (GlibToolsFixture *fixture,
                       gconstpointer     user_data G_GNUC_UNUSED)
{
    g_free (fixture->session_id);
    g_clear_object (&fixture->session);
    gdb_session_manager_terminate_all (fixture->manager);
    g_clear_object (&fixture->manager);
}


/* ========================================================================== */
/* gdb_glib_print_gobject Tests                                               */
/* ========================================================================== */

static void
test_glib_print_gobject_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "expression", "obj");

    result = gdb_tools_handle_gdb_glib_print_gobject (NULL, "gdb_glib_print_gobject",
                                                       arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_glib_print_gobject_missing_expression (GlibToolsFixture *fixture,
                                            gconstpointer     user_data G_GNUC_UNUSED)
{
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", fixture->session_id);
    /* Missing expression */

    result = gdb_tools_handle_gdb_glib_print_gobject (NULL, "gdb_glib_print_gobject",
                                                       arguments, fixture->manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_glib_print_gobject_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_glib_print_gobject_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "expression"));
}


/* ========================================================================== */
/* gdb_glib_print_glist Tests                                                 */
/* ========================================================================== */

static void
test_glib_print_glist_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "expression", "my_list");

    result = gdb_tools_handle_gdb_glib_print_glist (NULL, "gdb_glib_print_glist",
                                                     arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_glib_print_glist_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_glib_print_glist_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "expression"));
}


/* ========================================================================== */
/* gdb_glib_print_ghash Tests                                                 */
/* ========================================================================== */

static void
test_glib_print_ghash_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "expression", "my_hash");

    result = gdb_tools_handle_gdb_glib_print_ghash (NULL, "gdb_glib_print_ghash",
                                                     arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_glib_print_ghash_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_glib_print_ghash_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "expression"));
}


/* ========================================================================== */
/* gdb_glib_type_hierarchy Tests                                              */
/* ========================================================================== */

static void
test_glib_type_hierarchy_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "expression", "obj");

    result = gdb_tools_handle_gdb_glib_type_hierarchy (NULL, "gdb_glib_type_hierarchy",
                                                        arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_glib_type_hierarchy_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_glib_type_hierarchy_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "expression"));
}


/* ========================================================================== */
/* gdb_glib_signal_info Tests                                                 */
/* ========================================================================== */

static void
test_glib_signal_info_missing_session (void)
{
    g_autoptr(GdbSessionManager) manager = gdb_session_manager_new ();
    g_autoptr(JsonObject) arguments = json_object_new ();
    g_autoptr(McpToolResult) result = NULL;

    json_object_set_string_member (arguments, "sessionId", "nonexistent");
    json_object_set_string_member (arguments, "expression", "obj");

    result = gdb_tools_handle_gdb_glib_signal_info (NULL, "gdb_glib_signal_info",
                                                     arguments, manager);

    g_assert_nonnull (result);
    g_assert_true (mcp_tool_result_get_is_error (result));
}

static void
test_glib_signal_info_schema (void)
{
    g_autoptr(JsonNode) schema = NULL;
    JsonObject *obj;
    JsonObject *props;

    schema = gdb_tools_create_gdb_glib_signal_info_schema ();

    g_assert_nonnull (schema);

    obj = json_node_get_object (schema);
    props = json_object_get_object_member (obj, "properties");

    g_assert_true (json_object_has_member (props, "sessionId"));
    g_assert_true (json_object_has_member (props, "expression"));
}


/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    /* gdb_glib_print_gobject tests */
    g_test_add_func ("/gdb/tools/glib/print-gobject-missing-session", test_glib_print_gobject_missing_session);
    g_test_add ("/gdb/tools/glib/print-gobject-missing-expression",
                GlibToolsFixture, NULL,
                glib_fixture_setup,
                test_glib_print_gobject_missing_expression,
                glib_fixture_teardown);
    g_test_add_func ("/gdb/tools/glib/print-gobject-schema", test_glib_print_gobject_schema);

    /* gdb_glib_print_glist tests */
    g_test_add_func ("/gdb/tools/glib/print-glist-missing-session", test_glib_print_glist_missing_session);
    g_test_add_func ("/gdb/tools/glib/print-glist-schema", test_glib_print_glist_schema);

    /* gdb_glib_print_ghash tests */
    g_test_add_func ("/gdb/tools/glib/print-ghash-missing-session", test_glib_print_ghash_missing_session);
    g_test_add_func ("/gdb/tools/glib/print-ghash-schema", test_glib_print_ghash_schema);

    /* gdb_glib_type_hierarchy tests */
    g_test_add_func ("/gdb/tools/glib/type-hierarchy-missing-session", test_glib_type_hierarchy_missing_session);
    g_test_add_func ("/gdb/tools/glib/type-hierarchy-schema", test_glib_type_hierarchy_schema);

    /* gdb_glib_signal_info tests */
    g_test_add_func ("/gdb/tools/glib/signal-info-missing-session", test_glib_signal_info_missing_session);
    g_test_add_func ("/gdb/tools/glib/signal-info-schema", test_glib_signal_info_schema);

    return g_test_run ();
}
