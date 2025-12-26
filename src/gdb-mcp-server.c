/*
 * gdb-mcp-server.c - GDB MCP Server class implementation
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This file implements the main GDB MCP server class that:
 * - Owns the McpServer instance
 * - Registers all GDB debugging tools
 * - Manages the GdbSessionManager
 * - Handles server lifecycle
 */

#include "mcp-gdb/gdb-mcp-server.h"
#include "tools/gdb-tools-internal.h"
#include <mcp/mcp.h>

/* ========================================================================== */
/* GdbMcpServer Structure                                                     */
/* ========================================================================== */

struct _GdbMcpServer
{
    GObject parent_instance;

    /* Properties */
    gchar *name;
    gchar *version;
    gchar *default_gdb_path;

    /* Owned objects */
    McpServer *mcp_server;
    GdbSessionManager *session_manager;
    GMainLoop *main_loop;
};

G_DEFINE_TYPE (GdbMcpServer, gdb_mcp_server, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_NAME,
    PROP_VERSION,
    PROP_DEFAULT_GDB_PATH,
    PROP_SESSION_MANAGER,
    N_PROPS
};

static GParamSpec *properties[N_PROPS];

/* ========================================================================== */
/* Server Instructions                                                        */
/* ========================================================================== */

static const gchar *SERVER_INSTRUCTIONS =
    "GDB MCP Server - Debug programs with GDB through MCP tools.\n"
    "\n"
    "## Session Management\n"
    "- gdb_start: Start a new GDB session (returns sessionId)\n"
    "- gdb_terminate: End a GDB session\n"
    "- gdb_list_sessions: List all active sessions\n"
    "\n"
    "## Program Loading\n"
    "- gdb_load: Load a program into GDB\n"
    "- gdb_attach: Attach to a running process by PID\n"
    "- gdb_load_core: Load a core dump for post-mortem analysis\n"
    "\n"
    "## Execution Control\n"
    "- gdb_continue: Resume program execution\n"
    "- gdb_step: Step into functions (stepi for instructions)\n"
    "- gdb_next: Step over function calls (nexti for instructions)\n"
    "- gdb_finish: Execute until current function returns\n"
    "\n"
    "## Breakpoints\n"
    "- gdb_set_breakpoint: Set a breakpoint with optional condition\n"
    "\n"
    "## Inspection\n"
    "- gdb_backtrace: Show call stack\n"
    "- gdb_print: Evaluate an expression\n"
    "- gdb_examine: Examine memory at address\n"
    "- gdb_info_registers: Show CPU registers\n"
    "- gdb_command: Execute arbitrary GDB command\n"
    "\n"
    "## GLib/GObject Debugging\n"
    "- gdb_glib_print_gobject: Pretty-print a GObject instance\n"
    "- gdb_glib_print_glist: Pretty-print GList/GSList contents\n"
    "- gdb_glib_print_ghash: Pretty-print GHashTable contents\n"
    "- gdb_glib_type_hierarchy: Show GType inheritance chain\n"
    "- gdb_glib_signal_info: List signals on a GObject\n"
    "\n"
    "## Typical Workflow\n"
    "1. gdb_start -> Get sessionId\n"
    "2. gdb_load with program path\n"
    "3. gdb_set_breakpoint at function or line\n"
    "4. gdb_continue to run until breakpoint\n"
    "5. gdb_backtrace, gdb_print to inspect state\n"
    "6. gdb_step/next to trace execution\n"
    "7. gdb_terminate when done\n";

/* ========================================================================== */
/* Tool Registration                                                          */
/* ========================================================================== */

/*
 * register_session_tools:
 * @self: the server
 *
 * Registers session management tools: gdb_start, gdb_terminate, gdb_list_sessions
 */
static void
register_session_tools (GdbMcpServer *self)
{
    /* gdb_start */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_start",
            "Start a new GDB debugging session");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_start_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_start,
                             self->session_manager, NULL);
    }

    /* gdb_terminate */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_terminate",
            "Terminate a GDB session");
        g_autoptr(JsonNode) schema = gdb_tools_create_session_id_only_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_terminate,
                             self->session_manager, NULL);
    }

    /* gdb_list_sessions */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_list_sessions",
            "List all active GDB sessions");
        /* No schema needed - no arguments */
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_list_sessions,
                             self->session_manager, NULL);
    }
}

/*
 * register_load_tools:
 * @self: the server
 *
 * Registers program loading tools: gdb_load, gdb_attach, gdb_load_core
 */
static void
register_load_tools (GdbMcpServer *self)
{
    /* gdb_load */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_load",
            "Load a program into GDB for debugging");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_load_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_load,
                             self->session_manager, NULL);
    }

    /* gdb_attach */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_attach",
            "Attach to a running process by PID");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_attach_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_attach,
                             self->session_manager, NULL);
    }

    /* gdb_load_core */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_load_core",
            "Load a core dump file for post-mortem debugging");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_load_core_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_load_core,
                             self->session_manager, NULL);
    }
}

/*
 * register_exec_tools:
 * @self: the server
 *
 * Registers execution control tools: gdb_continue, gdb_step, gdb_next, gdb_finish
 */
static void
register_exec_tools (GdbMcpServer *self)
{
    /* gdb_continue */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_continue",
            "Continue program execution until next breakpoint or exit");
        g_autoptr(JsonNode) schema = gdb_tools_create_session_id_only_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_continue,
                             self->session_manager, NULL);
    }

    /* gdb_step */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_step",
            "Step into functions (single step by source line or instruction)");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_step_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_step,
                             self->session_manager, NULL);
    }

    /* gdb_next */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_next",
            "Step over function calls (single step without entering functions)");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_next_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_next,
                             self->session_manager, NULL);
    }

    /* gdb_finish */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_finish",
            "Execute until the current function returns");
        g_autoptr(JsonNode) schema = gdb_tools_create_session_id_only_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_finish,
                             self->session_manager, NULL);
    }
}

/*
 * register_breakpoint_tools:
 * @self: the server
 *
 * Registers breakpoint tools: gdb_set_breakpoint
 */
static void
register_breakpoint_tools (GdbMcpServer *self)
{
    /* gdb_set_breakpoint */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_set_breakpoint",
            "Set a breakpoint at a location (function, file:line, or *address)");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_breakpoint_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_set_breakpoint,
                             self->session_manager, NULL);
    }
}

/*
 * register_inspect_tools:
 * @self: the server
 *
 * Registers inspection tools: gdb_backtrace, gdb_print, gdb_examine,
 *                             gdb_info_registers, gdb_command
 */
static void
register_inspect_tools (GdbMcpServer *self)
{
    /* gdb_backtrace */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_backtrace",
            "Show the current call stack / backtrace");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_backtrace_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_backtrace,
                             self->session_manager, NULL);
    }

    /* gdb_print */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_print",
            "Evaluate and print an expression");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_print_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_print,
                             self->session_manager, NULL);
    }

    /* gdb_examine */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_examine",
            "Examine memory at a given address");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_examine_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_examine,
                             self->session_manager, NULL);
    }

    /* gdb_info_registers */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_info_registers",
            "Show CPU register values");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_info_registers_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_info_registers,
                             self->session_manager, NULL);
    }

    /* gdb_command */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_command",
            "Execute an arbitrary GDB command (escape hatch for advanced use)");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_command_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_command,
                             self->session_manager, NULL);
    }
}

/*
 * register_glib_tools:
 * @self: the server
 *
 * Registers GLib/GObject debugging tools
 */
static void
register_glib_tools (GdbMcpServer *self)
{
    /* gdb_glib_print_gobject */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_glib_print_gobject",
            "Pretty-print a GObject instance (type, ref_count, properties)");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_glib_print_gobject_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_glib_print_gobject,
                             self->session_manager, NULL);
    }

    /* gdb_glib_print_glist */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_glib_print_glist",
            "Pretty-print GList or GSList contents");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_glib_print_glist_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_glib_print_glist,
                             self->session_manager, NULL);
    }

    /* gdb_glib_print_ghash */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_glib_print_ghash",
            "Pretty-print GHashTable key-value pairs");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_glib_print_ghash_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_glib_print_ghash,
                             self->session_manager, NULL);
    }

    /* gdb_glib_type_hierarchy */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_glib_type_hierarchy",
            "Show the GType inheritance hierarchy for a type or instance");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_glib_type_hierarchy_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_glib_type_hierarchy,
                             self->session_manager, NULL);
    }

    /* gdb_glib_signal_info */
    {
        g_autoptr(McpTool) tool = mcp_tool_new (
            "gdb_glib_signal_info",
            "List signals registered on a GObject type or instance");
        g_autoptr(JsonNode) schema = gdb_tools_create_gdb_glib_signal_info_schema ();
        mcp_tool_set_input_schema (tool, schema);
        mcp_server_add_tool (self->mcp_server, tool,
                             gdb_tools_handle_gdb_glib_signal_info,
                             self->session_manager, NULL);
    }
}

/*
 * register_all_tools:
 * @self: the server
 *
 * Registers all GDB debugging tools with the MCP server.
 */
static void
register_all_tools (GdbMcpServer *self)
{
    register_session_tools (self);
    register_load_tools (self);
    register_exec_tools (self);
    register_breakpoint_tools (self);
    register_inspect_tools (self);
    register_glib_tools (self);
}

/* ========================================================================== */
/* Signal Handlers                                                            */
/* ========================================================================== */

static void
on_client_disconnected (McpServer *mcp_server G_GNUC_UNUSED,
                        gpointer   user_data)
{
    GdbMcpServer *self = GDB_MCP_SERVER (user_data);

    g_message ("Client disconnected, shutting down");

    if (self->main_loop != NULL)
    {
        g_main_loop_quit (self->main_loop);
    }
}

/* ========================================================================== */
/* GObject Implementation                                                     */
/* ========================================================================== */

static void
gdb_mcp_server_dispose (GObject *object)
{
    GdbMcpServer *self = GDB_MCP_SERVER (object);

    g_clear_object (&self->mcp_server);
    g_clear_object (&self->session_manager);

    if (self->main_loop != NULL)
    {
        g_main_loop_unref (self->main_loop);
        self->main_loop = NULL;
    }

    G_OBJECT_CLASS (gdb_mcp_server_parent_class)->dispose (object);
}

static void
gdb_mcp_server_finalize (GObject *object)
{
    GdbMcpServer *self = GDB_MCP_SERVER (object);

    g_free (self->name);
    g_free (self->version);
    g_free (self->default_gdb_path);

    G_OBJECT_CLASS (gdb_mcp_server_parent_class)->finalize (object);
}

static void
gdb_mcp_server_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    GdbMcpServer *self = GDB_MCP_SERVER (object);

    switch (prop_id)
    {
        case PROP_NAME:
            g_value_set_string (value, self->name);
            break;

        case PROP_VERSION:
            g_value_set_string (value, self->version);
            break;

        case PROP_DEFAULT_GDB_PATH:
            g_value_set_string (value, self->default_gdb_path);
            break;

        case PROP_SESSION_MANAGER:
            g_value_set_object (value, self->session_manager);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gdb_mcp_server_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    GdbMcpServer *self = GDB_MCP_SERVER (object);

    switch (prop_id)
    {
        case PROP_NAME:
            g_free (self->name);
            self->name = g_value_dup_string (value);
            break;

        case PROP_VERSION:
            g_free (self->version);
            self->version = g_value_dup_string (value);
            break;

        case PROP_DEFAULT_GDB_PATH:
            g_free (self->default_gdb_path);
            self->default_gdb_path = g_value_dup_string (value);
            if (self->session_manager != NULL)
            {
                gdb_session_manager_set_default_gdb_path (
                    self->session_manager, self->default_gdb_path);
            }
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gdb_mcp_server_constructed (GObject *object)
{
    GdbMcpServer *self = GDB_MCP_SERVER (object);

    G_OBJECT_CLASS (gdb_mcp_server_parent_class)->constructed (object);

    /* Create the McpServer */
    self->mcp_server = mcp_server_new (self->name, self->version);
    mcp_server_set_instructions (self->mcp_server, SERVER_INSTRUCTIONS);

    /* Create session manager */
    self->session_manager = gdb_session_manager_new ();
    if (self->default_gdb_path != NULL)
    {
        gdb_session_manager_set_default_gdb_path (
            self->session_manager, self->default_gdb_path);
    }

    /* Connect signals */
    g_signal_connect (self->mcp_server, "client-disconnected",
                      G_CALLBACK (on_client_disconnected), self);

    /* Register all tools */
    register_all_tools (self);
}

static void
gdb_mcp_server_class_init (GdbMcpServerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = gdb_mcp_server_dispose;
    object_class->finalize = gdb_mcp_server_finalize;
    object_class->get_property = gdb_mcp_server_get_property;
    object_class->set_property = gdb_mcp_server_set_property;
    object_class->constructed = gdb_mcp_server_constructed;

    /**
     * GdbMcpServer:name:
     *
     * The server name.
     */
    properties[PROP_NAME] =
        g_param_spec_string ("name",
                             "Name",
                             "The server name",
                             "gdb-mcp-server",
                             G_PARAM_READWRITE |
                             G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS);

    /**
     * GdbMcpServer:version:
     *
     * The server version.
     */
    properties[PROP_VERSION] =
        g_param_spec_string ("version",
                             "Version",
                             "The server version",
                             "1.0.0",
                             G_PARAM_READWRITE |
                             G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS);

    /**
     * GdbMcpServer:default-gdb-path:
     *
     * The default path to the GDB binary.
     */
    properties[PROP_DEFAULT_GDB_PATH] =
        g_param_spec_string ("default-gdb-path",
                             "Default GDB Path",
                             "The default path to the GDB binary",
                             NULL,
                             G_PARAM_READWRITE |
                             G_PARAM_STATIC_STRINGS);

    /**
     * GdbMcpServer:session-manager:
     *
     * The session manager (read-only).
     */
    properties[PROP_SESSION_MANAGER] =
        g_param_spec_object ("session-manager",
                             "Session Manager",
                             "The GDB session manager",
                             GDB_TYPE_SESSION_MANAGER,
                             G_PARAM_READABLE |
                             G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gdb_mcp_server_init (GdbMcpServer *self)
{
    self->name = NULL;
    self->version = NULL;
    self->default_gdb_path = NULL;
    self->mcp_server = NULL;
    self->session_manager = NULL;
    self->main_loop = NULL;
}

/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

GdbMcpServer *
gdb_mcp_server_new (const gchar *name,
                    const gchar *version)
{
    return (GdbMcpServer *)g_object_new (GDB_TYPE_MCP_SERVER,
                                         "name", name,
                                         "version", version,
                                         NULL);
}

GdbSessionManager *
gdb_mcp_server_get_session_manager (GdbMcpServer *self)
{
    g_return_val_if_fail (GDB_IS_MCP_SERVER (self), NULL);

    return self->session_manager;
}

void
gdb_mcp_server_set_default_gdb_path (GdbMcpServer *self,
                                     const gchar  *gdb_path)
{
    g_return_if_fail (GDB_IS_MCP_SERVER (self));

    g_object_set (self, "default-gdb-path", gdb_path, NULL);
}

const gchar *
gdb_mcp_server_get_default_gdb_path (GdbMcpServer *self)
{
    g_return_val_if_fail (GDB_IS_MCP_SERVER (self), NULL);

    return self->default_gdb_path;
}

void
gdb_mcp_server_run (GdbMcpServer *self)
{
    g_autoptr(McpStdioTransport) transport = NULL;

    g_return_if_fail (GDB_IS_MCP_SERVER (self));

    /* Create main loop */
    self->main_loop = g_main_loop_new (NULL, FALSE);

    /* Set up stdio transport */
    transport = mcp_stdio_transport_new ();
    mcp_server_set_transport (self->mcp_server, MCP_TRANSPORT (transport));

    /* Start the server */
    g_message ("Starting GDB MCP Server (%s %s)...", self->name, self->version);
    mcp_server_start_async (self->mcp_server, NULL, NULL, NULL);

    /* Run the main loop */
    g_main_loop_run (self->main_loop);

    g_message ("GDB MCP Server stopped");
}

void
gdb_mcp_server_stop (GdbMcpServer *self)
{
    g_return_if_fail (GDB_IS_MCP_SERVER (self));

    if (self->main_loop != NULL && g_main_loop_is_running (self->main_loop))
    {
        g_main_loop_quit (self->main_loop);
    }
}
