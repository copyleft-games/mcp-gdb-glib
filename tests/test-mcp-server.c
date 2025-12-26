/*
 * test-mcp-server.c - Unit tests for GdbMcpServer
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include "mcp-gdb/gdb-mcp-server.h"
#include "mcp-gdb/gdb-session-manager.h"

/* ========================================================================== */
/* Construction Tests                                                         */
/* ========================================================================== */

static void
test_mcp_server_new (void)
{
    g_autoptr(GdbMcpServer) server = NULL;

    server = gdb_mcp_server_new ("test-server", "1.0.0");

    g_assert_nonnull (server);
    g_assert_true (GDB_IS_MCP_SERVER (server));
}

static void
test_mcp_server_new_custom_name (void)
{
    g_autoptr(GdbMcpServer) server = NULL;

    server = gdb_mcp_server_new ("my-custom-server", "2.5.1");

    g_assert_nonnull (server);
    g_assert_true (GDB_IS_MCP_SERVER (server));
}


/* ========================================================================== */
/* Properties Tests                                                           */
/* ========================================================================== */

static void
test_mcp_server_properties_name (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    g_autofree gchar *name = NULL;

    server = gdb_mcp_server_new ("test-name", "1.0.0");

    g_object_get (server, "name", &name, NULL);
    g_assert_cmpstr (name, ==, "test-name");
}

static void
test_mcp_server_properties_version (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    g_autofree gchar *version = NULL;

    server = gdb_mcp_server_new ("server", "3.2.1");

    g_object_get (server, "version", &version, NULL);
    g_assert_cmpstr (version, ==, "3.2.1");
}

static void
test_mcp_server_properties_session_manager (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    g_autoptr(GdbSessionManager) manager = NULL;

    server = gdb_mcp_server_new ("test", "1.0.0");

    g_object_get (server, "session-manager", &manager, NULL);
    g_assert_nonnull (manager);
    g_assert_true (GDB_IS_SESSION_MANAGER (manager));
}

static void
test_mcp_server_properties_default_gdb_path (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    g_autofree gchar *path = NULL;

    server = gdb_mcp_server_new ("test", "1.0.0");

    /* Should be NULL initially */
    g_object_get (server, "default-gdb-path", &path, NULL);
    g_assert_null (path);

    /* Set a path */
    g_object_set (server, "default-gdb-path", "/usr/local/bin/gdb", NULL);

    g_free (path);
    path = NULL;
    g_object_get (server, "default-gdb-path", &path, NULL);
    g_assert_cmpstr (path, ==, "/usr/local/bin/gdb");
}


/* ========================================================================== */
/* Session Manager Accessor Tests                                             */
/* ========================================================================== */

static void
test_mcp_server_get_session_manager (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    GdbSessionManager *manager;

    server = gdb_mcp_server_new ("test", "1.0.0");

    manager = gdb_mcp_server_get_session_manager (server);

    g_assert_nonnull (manager);
    g_assert_true (GDB_IS_SESSION_MANAGER (manager));
}

static void
test_mcp_server_session_manager_consistent (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    GdbSessionManager *manager1;
    GdbSessionManager *manager2;

    server = gdb_mcp_server_new ("test", "1.0.0");

    manager1 = gdb_mcp_server_get_session_manager (server);
    manager2 = gdb_mcp_server_get_session_manager (server);

    /* Should return the same instance */
    g_assert_true (manager1 == manager2);
}


/* ========================================================================== */
/* Default GDB Path Tests                                                     */
/* ========================================================================== */

static void
test_mcp_server_default_gdb_path_initial (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    const gchar *path;

    server = gdb_mcp_server_new ("test", "1.0.0");

    path = gdb_mcp_server_get_default_gdb_path (server);
    g_assert_null (path);
}

static void
test_mcp_server_set_default_gdb_path (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    const gchar *path;

    server = gdb_mcp_server_new ("test", "1.0.0");

    gdb_mcp_server_set_default_gdb_path (server, "/opt/gdb/bin/gdb");
    path = gdb_mcp_server_get_default_gdb_path (server);

    g_assert_cmpstr (path, ==, "/opt/gdb/bin/gdb");
}

static void
test_mcp_server_set_default_gdb_path_null (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    const gchar *path;

    server = gdb_mcp_server_new ("test", "1.0.0");

    /* Set to something */
    gdb_mcp_server_set_default_gdb_path (server, "/custom/gdb");
    g_assert_cmpstr (gdb_mcp_server_get_default_gdb_path (server), ==, "/custom/gdb");

    /* Set to NULL */
    gdb_mcp_server_set_default_gdb_path (server, NULL);
    path = gdb_mcp_server_get_default_gdb_path (server);

    g_assert_null (path);
}

static void
test_mcp_server_gdb_path_propagates_to_manager (void)
{
    g_autoptr(GdbMcpServer) server = NULL;
    GdbSessionManager *manager;
    const gchar *manager_path;

    server = gdb_mcp_server_new ("test", "1.0.0");
    manager = gdb_mcp_server_get_session_manager (server);

    /* Set path on server */
    gdb_mcp_server_set_default_gdb_path (server, "/my/gdb");

    /* Should propagate to session manager */
    manager_path = gdb_session_manager_get_default_gdb_path (manager);
    g_assert_cmpstr (manager_path, ==, "/my/gdb");
}


/* ========================================================================== */
/* Server Lifecycle Tests                                                     */
/* ========================================================================== */

static void
test_mcp_server_stop_not_running (void)
{
    g_autoptr(GdbMcpServer) server = NULL;

    server = gdb_mcp_server_new ("test", "1.0.0");

    /* Should not crash when stopping a server that isn't running */
    gdb_mcp_server_stop (server);
}


/* ========================================================================== */
/* Type Tests                                                                 */
/* ========================================================================== */

static void
test_mcp_server_type (void)
{
    GType type;

    type = GDB_TYPE_MCP_SERVER;

    g_assert_cmpuint (type, !=, G_TYPE_INVALID);
    g_assert_true (g_type_is_a (type, G_TYPE_OBJECT));
}

static void
test_mcp_server_type_name (void)
{
    GType type;
    const gchar *name;

    type = GDB_TYPE_MCP_SERVER;
    name = g_type_name (type);

    g_assert_cmpstr (name, ==, "GdbMcpServer");
}


/* ========================================================================== */
/* Reference Counting Tests                                                   */
/* ========================================================================== */

static void
test_mcp_server_ref_unref (void)
{
    GdbMcpServer *server;

    server = gdb_mcp_server_new ("test", "1.0.0");
    g_assert_nonnull (server);

    /* Add a reference */
    g_object_ref (server);

    /* Remove the extra reference */
    g_object_unref (server);

    /* Should still be valid */
    g_assert_true (GDB_IS_MCP_SERVER (server));

    /* Final unref */
    g_object_unref (server);
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
    g_test_add_func ("/gdb/mcp-server/new", test_mcp_server_new);
    g_test_add_func ("/gdb/mcp-server/new-custom-name", test_mcp_server_new_custom_name);

    /* Properties tests */
    g_test_add_func ("/gdb/mcp-server/properties/name", test_mcp_server_properties_name);
    g_test_add_func ("/gdb/mcp-server/properties/version", test_mcp_server_properties_version);
    g_test_add_func ("/gdb/mcp-server/properties/session-manager", test_mcp_server_properties_session_manager);
    g_test_add_func ("/gdb/mcp-server/properties/default-gdb-path", test_mcp_server_properties_default_gdb_path);

    /* Session manager accessor tests */
    g_test_add_func ("/gdb/mcp-server/get-session-manager", test_mcp_server_get_session_manager);
    g_test_add_func ("/gdb/mcp-server/session-manager-consistent", test_mcp_server_session_manager_consistent);

    /* Default GDB path tests */
    g_test_add_func ("/gdb/mcp-server/default-gdb-path/initial", test_mcp_server_default_gdb_path_initial);
    g_test_add_func ("/gdb/mcp-server/set-default-gdb-path", test_mcp_server_set_default_gdb_path);
    g_test_add_func ("/gdb/mcp-server/set-default-gdb-path-null", test_mcp_server_set_default_gdb_path_null);
    g_test_add_func ("/gdb/mcp-server/gdb-path-propagates", test_mcp_server_gdb_path_propagates_to_manager);

    /* Lifecycle tests */
    g_test_add_func ("/gdb/mcp-server/stop-not-running", test_mcp_server_stop_not_running);

    /* Type tests */
    g_test_add_func ("/gdb/mcp-server/type", test_mcp_server_type);
    g_test_add_func ("/gdb/mcp-server/type-name", test_mcp_server_type_name);

    /* Reference counting tests */
    g_test_add_func ("/gdb/mcp-server/ref-unref", test_mcp_server_ref_unref);

    return g_test_run ();
}
