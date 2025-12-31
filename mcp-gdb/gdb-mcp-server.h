/*
 * gdb-mcp-server.h - GDB MCP Server class
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef GDB_MCP_SERVER_H
#define GDB_MCP_SERVER_H

#include <glib-object.h>
#include <mcp.h>
#include "gdb-session-manager.h"

G_BEGIN_DECLS

#define GDB_TYPE_MCP_SERVER (gdb_mcp_server_get_type ())
G_DECLARE_FINAL_TYPE (GdbMcpServer, gdb_mcp_server, GDB, MCP_SERVER, GObject)

/**
 * gdb_mcp_server_new:
 * @name: the server name
 * @version: the server version
 *
 * Creates a new GDB MCP server instance.
 *
 * Returns: (transfer full): a new #GdbMcpServer
 */
GdbMcpServer *gdb_mcp_server_new (const gchar *name,
                                  const gchar *version);

/**
 * gdb_mcp_server_get_session_manager:
 * @self: a #GdbMcpServer
 *
 * Gets the session manager for this server.
 *
 * Returns: (transfer none): the #GdbSessionManager
 */
GdbSessionManager *gdb_mcp_server_get_session_manager (GdbMcpServer *self);

/**
 * gdb_mcp_server_set_default_gdb_path:
 * @self: a #GdbMcpServer
 * @gdb_path: (nullable): path to the GDB binary, or %NULL for default
 *
 * Sets the default GDB binary path for new sessions.
 */
void gdb_mcp_server_set_default_gdb_path (GdbMcpServer *self,
                                          const gchar  *gdb_path);

/**
 * gdb_mcp_server_get_default_gdb_path:
 * @self: a #GdbMcpServer
 *
 * Gets the default GDB binary path.
 *
 * Returns: (transfer none) (nullable): the GDB path, or %NULL for default
 */
const gchar *gdb_mcp_server_get_default_gdb_path (GdbMcpServer *self);

/**
 * gdb_mcp_server_run:
 * @self: a #GdbMcpServer
 *
 * Runs the server main loop. This function blocks until the server
 * is stopped via gdb_mcp_server_stop() or the client disconnects.
 */
void gdb_mcp_server_run (GdbMcpServer *self);

/**
 * gdb_mcp_server_stop:
 * @self: a #GdbMcpServer
 *
 * Stops the server main loop.
 */
void gdb_mcp_server_stop (GdbMcpServer *self);

G_END_DECLS

#endif /* GDB_MCP_SERVER_H */
