/*
 * main.c - GDB MCP Server entry point
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This is the main entry point for the GDB MCP server. It handles:
 * - Command-line argument parsing
 * - Signal handling (SIGINT, SIGTERM)
 * - Server lifecycle management
 */

#include "mcp-gdb/gdb-mcp-server.h"
#include <glib.h>
#include <glib-unix.h>
#include <locale.h>

#define SERVER_NAME    "gdb-mcp-server"
#define SERVER_VERSION "1.0.0"

/* Global server reference for signal handlers */
static GdbMcpServer *g_server = NULL;

/* ========================================================================== */
/* License and Version Information                                            */
/* ========================================================================== */

static const gchar *LICENSE_TEXT =
    "gdb-mcp-server - GDB debugger MCP server\n"
    "Copyright (C) 2025 Zach Podbielniak\n"
    "\n"
    "This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU Affero General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU Affero General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU Affero General Public License\n"
    "along with this program.  If not, see <https://www.gnu.org/licenses/>.\n";

/* ========================================================================== */
/* Signal Handling                                                            */
/* ========================================================================== */

/*
 * on_sigint:
 * @user_data: the server
 *
 * Handles SIGINT (Ctrl+C) by stopping the server gracefully.
 *
 * Returns: G_SOURCE_REMOVE to remove the signal source
 */
static gboolean
on_sigint (gpointer user_data)
{
    GdbMcpServer *server = GDB_MCP_SERVER (user_data);

    g_message ("Received SIGINT, shutting down...");
    gdb_mcp_server_stop (server);

    return G_SOURCE_REMOVE;
}

/*
 * on_sigterm:
 * @user_data: the server
 *
 * Handles SIGTERM by stopping the server gracefully.
 *
 * Returns: G_SOURCE_REMOVE to remove the signal source
 */
static gboolean
on_sigterm (gpointer user_data)
{
    GdbMcpServer *server = GDB_MCP_SERVER (user_data);

    g_message ("Received SIGTERM, shutting down...");
    gdb_mcp_server_stop (server);

    return G_SOURCE_REMOVE;
}

/* ========================================================================== */
/* Command-Line Parsing                                                       */
/* ========================================================================== */

static gboolean show_version = FALSE;
static gboolean show_license = FALSE;
static gchar *gdb_path = NULL;

static GOptionEntry option_entries[] =
{
    {
        "version", 'v', 0, G_OPTION_ARG_NONE, &show_version,
        "Show version information", NULL
    },
    {
        "license", 'l', 0, G_OPTION_ARG_NONE, &show_license,
        "Show license information (AGPLv3)", NULL
    },
    {
        "gdb-path", 'g', 0, G_OPTION_ARG_FILENAME, &gdb_path,
        "Path to the GDB binary (default: 'gdb' from PATH)", "PATH"
    },
    { NULL }
};

/* ========================================================================== */
/* Main Entry Point                                                           */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_autoptr(GOptionContext) context = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(GdbMcpServer) server = NULL;

    /* Set up locale */
    setlocale (LC_ALL, "");

    /* Parse command-line arguments */
    context = g_option_context_new ("- GDB debugger MCP server");
    g_option_context_add_main_entries (context, option_entries, NULL);
    g_option_context_set_summary (context,
        "A Model Context Protocol (MCP) server for GDB debugging.\n"
        "Provides tools for debugging programs, setting breakpoints,\n"
        "inspecting state, and GLib/GObject-specific debugging features.");
    g_option_context_set_description (context,
        "Examples:\n"
        "  gdb-mcp-server                    # Start with default GDB\n"
        "  gdb-mcp-server --gdb-path=/usr/bin/gdb-15\n"
        "  gdb-mcp-server -v                 # Show version\n"
        "  gdb-mcp-server -l                 # Show license\n"
        "\n"
        "For more information, see:\n"
        "  https://github.com/zachpodbielniak/mcp-gdb-glib\n");

    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_printerr ("Error: %s\n", error->message);
        g_printerr ("Use --help for usage information\n");
        return 1;
    }

    /* Handle --version */
    if (show_version)
    {
        g_print ("%s %s\n", SERVER_NAME, SERVER_VERSION);
        g_print ("Copyright (C) 2025 Zach Podbielniak\n");
        g_print ("License: AGPLv3+ <https://www.gnu.org/licenses/agpl-3.0.html>\n");
        return 0;
    }

    /* Handle --license */
    if (show_license)
    {
        g_print ("%s", LICENSE_TEXT);
        return 0;
    }

    /* Create the server */
    server = gdb_mcp_server_new (SERVER_NAME, SERVER_VERSION);
    g_server = server;

    /* Set custom GDB path if provided */
    if (gdb_path != NULL)
    {
        gdb_mcp_server_set_default_gdb_path (server, gdb_path);
        g_message ("Using GDB: %s", gdb_path);
    }

    /* Set up signal handlers */
    g_unix_signal_add (SIGINT, on_sigint, server);
    g_unix_signal_add (SIGTERM, on_sigterm, server);

    /* Run the server (blocks until stopped) */
    gdb_mcp_server_run (server);

    /* Cleanup */
    g_free (gdb_path);
    g_server = NULL;

    return 0;
}
