/*
 * gdb-enums.c - Enumeration types implementation for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "mcp-gdb/gdb-enums.h"
#include <string.h>

/* ========================================================================== */
/* GdbSessionState                                                            */
/* ========================================================================== */

static const GEnumValue session_state_values[] = {
    { GDB_SESSION_STATE_DISCONNECTED, "GDB_SESSION_STATE_DISCONNECTED", "disconnected" },
    { GDB_SESSION_STATE_STARTING,     "GDB_SESSION_STATE_STARTING",     "starting" },
    { GDB_SESSION_STATE_READY,        "GDB_SESSION_STATE_READY",        "ready" },
    { GDB_SESSION_STATE_RUNNING,      "GDB_SESSION_STATE_RUNNING",      "running" },
    { GDB_SESSION_STATE_STOPPED,      "GDB_SESSION_STATE_STOPPED",      "stopped" },
    { GDB_SESSION_STATE_TERMINATED,   "GDB_SESSION_STATE_TERMINATED",   "terminated" },
    { GDB_SESSION_STATE_ERROR,        "GDB_SESSION_STATE_ERROR",        "error" },
    { 0, NULL, NULL }
};

GType
gdb_session_state_get_type (void)
{
    static gsize g_define_type_id__volatile = 0;

    if (g_once_init_enter (&g_define_type_id__volatile))
    {
        GType g_define_type_id =
            g_enum_register_static ("GdbSessionState", session_state_values);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

    return g_define_type_id__volatile;
}

const gchar *
gdb_session_state_to_string (GdbSessionState state)
{
    switch (state)
    {
        case GDB_SESSION_STATE_DISCONNECTED:
            return "disconnected";
        case GDB_SESSION_STATE_STARTING:
            return "starting";
        case GDB_SESSION_STATE_READY:
            return "ready";
        case GDB_SESSION_STATE_RUNNING:
            return "running";
        case GDB_SESSION_STATE_STOPPED:
            return "stopped";
        case GDB_SESSION_STATE_TERMINATED:
            return "terminated";
        case GDB_SESSION_STATE_ERROR:
            return "error";
        default:
            return "unknown";
    }
}

GdbSessionState
gdb_session_state_from_string (const gchar *str)
{
    if (str == NULL)
    {
        return GDB_SESSION_STATE_DISCONNECTED;
    }

    if (g_strcmp0 (str, "disconnected") == 0)
        return GDB_SESSION_STATE_DISCONNECTED;
    if (g_strcmp0 (str, "starting") == 0)
        return GDB_SESSION_STATE_STARTING;
    if (g_strcmp0 (str, "ready") == 0)
        return GDB_SESSION_STATE_READY;
    if (g_strcmp0 (str, "running") == 0)
        return GDB_SESSION_STATE_RUNNING;
    if (g_strcmp0 (str, "stopped") == 0)
        return GDB_SESSION_STATE_STOPPED;
    if (g_strcmp0 (str, "terminated") == 0)
        return GDB_SESSION_STATE_TERMINATED;
    if (g_strcmp0 (str, "error") == 0)
        return GDB_SESSION_STATE_ERROR;

    return GDB_SESSION_STATE_DISCONNECTED;
}


/* ========================================================================== */
/* GdbStopReason                                                              */
/* ========================================================================== */

static const GEnumValue stop_reason_values[] = {
    { GDB_STOP_REASON_BREAKPOINT,        "GDB_STOP_REASON_BREAKPOINT",        "breakpoint-hit" },
    { GDB_STOP_REASON_WATCHPOINT,        "GDB_STOP_REASON_WATCHPOINT",        "watchpoint-trigger" },
    { GDB_STOP_REASON_SIGNAL,            "GDB_STOP_REASON_SIGNAL",            "signal-received" },
    { GDB_STOP_REASON_STEP,              "GDB_STOP_REASON_STEP",              "end-stepping-range" },
    { GDB_STOP_REASON_FINISH,            "GDB_STOP_REASON_FINISH",            "function-finished" },
    { GDB_STOP_REASON_EXITED,            "GDB_STOP_REASON_EXITED",            "exited" },
    { GDB_STOP_REASON_EXITED_NORMALLY,   "GDB_STOP_REASON_EXITED_NORMALLY",   "exited-normally" },
    { GDB_STOP_REASON_EXITED_SIGNALLED,  "GDB_STOP_REASON_EXITED_SIGNALLED",  "exited-signalled" },
    { GDB_STOP_REASON_UNKNOWN,           "GDB_STOP_REASON_UNKNOWN",           "unknown" },
    { 0, NULL, NULL }
};

GType
gdb_stop_reason_get_type (void)
{
    static gsize g_define_type_id__volatile = 0;

    if (g_once_init_enter (&g_define_type_id__volatile))
    {
        GType g_define_type_id =
            g_enum_register_static ("GdbStopReason", stop_reason_values);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

    return g_define_type_id__volatile;
}

const gchar *
gdb_stop_reason_to_string (GdbStopReason reason)
{
    switch (reason)
    {
        case GDB_STOP_REASON_BREAKPOINT:
            return "breakpoint-hit";
        case GDB_STOP_REASON_WATCHPOINT:
            return "watchpoint-trigger";
        case GDB_STOP_REASON_SIGNAL:
            return "signal-received";
        case GDB_STOP_REASON_STEP:
            return "end-stepping-range";
        case GDB_STOP_REASON_FINISH:
            return "function-finished";
        case GDB_STOP_REASON_EXITED:
            return "exited";
        case GDB_STOP_REASON_EXITED_NORMALLY:
            return "exited-normally";
        case GDB_STOP_REASON_EXITED_SIGNALLED:
            return "exited-signalled";
        case GDB_STOP_REASON_UNKNOWN:
        default:
            return "unknown";
    }
}

GdbStopReason
gdb_stop_reason_from_string (const gchar *str)
{
    if (str == NULL)
    {
        return GDB_STOP_REASON_UNKNOWN;
    }

    /* GDB/MI uses these exact strings */
    if (g_strcmp0 (str, "breakpoint-hit") == 0)
        return GDB_STOP_REASON_BREAKPOINT;
    if (g_strcmp0 (str, "watchpoint-trigger") == 0 ||
        g_strcmp0 (str, "read-watchpoint-trigger") == 0 ||
        g_strcmp0 (str, "access-watchpoint-trigger") == 0)
        return GDB_STOP_REASON_WATCHPOINT;
    if (g_strcmp0 (str, "signal-received") == 0)
        return GDB_STOP_REASON_SIGNAL;
    if (g_strcmp0 (str, "end-stepping-range") == 0)
        return GDB_STOP_REASON_STEP;
    if (g_strcmp0 (str, "function-finished") == 0)
        return GDB_STOP_REASON_FINISH;
    if (g_strcmp0 (str, "exited") == 0)
        return GDB_STOP_REASON_EXITED;
    if (g_strcmp0 (str, "exited-normally") == 0)
        return GDB_STOP_REASON_EXITED_NORMALLY;
    if (g_strcmp0 (str, "exited-signalled") == 0)
        return GDB_STOP_REASON_EXITED_SIGNALLED;

    return GDB_STOP_REASON_UNKNOWN;
}


/* ========================================================================== */
/* GdbMiRecordType                                                            */
/* ========================================================================== */

static const GEnumValue mi_record_type_values[] = {
    { GDB_MI_RECORD_RESULT,       "GDB_MI_RECORD_RESULT",       "result" },
    { GDB_MI_RECORD_EXEC_ASYNC,   "GDB_MI_RECORD_EXEC_ASYNC",   "exec-async" },
    { GDB_MI_RECORD_STATUS_ASYNC, "GDB_MI_RECORD_STATUS_ASYNC", "status-async" },
    { GDB_MI_RECORD_NOTIFY_ASYNC, "GDB_MI_RECORD_NOTIFY_ASYNC", "notify-async" },
    { GDB_MI_RECORD_CONSOLE,      "GDB_MI_RECORD_CONSOLE",      "console" },
    { GDB_MI_RECORD_TARGET,       "GDB_MI_RECORD_TARGET",       "target" },
    { GDB_MI_RECORD_LOG,          "GDB_MI_RECORD_LOG",          "log" },
    { GDB_MI_RECORD_PROMPT,       "GDB_MI_RECORD_PROMPT",       "prompt" },
    { GDB_MI_RECORD_UNKNOWN,      "GDB_MI_RECORD_UNKNOWN",      "unknown" },
    { 0, NULL, NULL }
};

GType
gdb_mi_record_type_get_type (void)
{
    static gsize g_define_type_id__volatile = 0;

    if (g_once_init_enter (&g_define_type_id__volatile))
    {
        GType g_define_type_id =
            g_enum_register_static ("GdbMiRecordType", mi_record_type_values);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

    return g_define_type_id__volatile;
}

const gchar *
gdb_mi_record_type_to_string (GdbMiRecordType type)
{
    switch (type)
    {
        case GDB_MI_RECORD_RESULT:
            return "result";
        case GDB_MI_RECORD_EXEC_ASYNC:
            return "exec-async";
        case GDB_MI_RECORD_STATUS_ASYNC:
            return "status-async";
        case GDB_MI_RECORD_NOTIFY_ASYNC:
            return "notify-async";
        case GDB_MI_RECORD_CONSOLE:
            return "console";
        case GDB_MI_RECORD_TARGET:
            return "target";
        case GDB_MI_RECORD_LOG:
            return "log";
        case GDB_MI_RECORD_PROMPT:
            return "prompt";
        case GDB_MI_RECORD_UNKNOWN:
        default:
            return "unknown";
    }
}

GdbMiRecordType
gdb_mi_record_type_from_char (gchar c)
{
    switch (c)
    {
        case '^':
            return GDB_MI_RECORD_RESULT;
        case '*':
            return GDB_MI_RECORD_EXEC_ASYNC;
        case '+':
            return GDB_MI_RECORD_STATUS_ASYNC;
        case '=':
            return GDB_MI_RECORD_NOTIFY_ASYNC;
        case '~':
            return GDB_MI_RECORD_CONSOLE;
        case '@':
            return GDB_MI_RECORD_TARGET;
        case '&':
            return GDB_MI_RECORD_LOG;
        default:
            return GDB_MI_RECORD_UNKNOWN;
    }
}


/* ========================================================================== */
/* GdbMiResultClass                                                           */
/* ========================================================================== */

static const GEnumValue mi_result_class_values[] = {
    { GDB_MI_RESULT_DONE,      "GDB_MI_RESULT_DONE",      "done" },
    { GDB_MI_RESULT_RUNNING,   "GDB_MI_RESULT_RUNNING",   "running" },
    { GDB_MI_RESULT_CONNECTED, "GDB_MI_RESULT_CONNECTED", "connected" },
    { GDB_MI_RESULT_ERROR,     "GDB_MI_RESULT_ERROR",     "error" },
    { GDB_MI_RESULT_EXIT,      "GDB_MI_RESULT_EXIT",      "exit" },
    { 0, NULL, NULL }
};

GType
gdb_mi_result_class_get_type (void)
{
    static gsize g_define_type_id__volatile = 0;

    if (g_once_init_enter (&g_define_type_id__volatile))
    {
        GType g_define_type_id =
            g_enum_register_static ("GdbMiResultClass", mi_result_class_values);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

    return g_define_type_id__volatile;
}

const gchar *
gdb_mi_result_class_to_string (GdbMiResultClass result_class)
{
    switch (result_class)
    {
        case GDB_MI_RESULT_DONE:
            return "done";
        case GDB_MI_RESULT_RUNNING:
            return "running";
        case GDB_MI_RESULT_CONNECTED:
            return "connected";
        case GDB_MI_RESULT_ERROR:
            return "error";
        case GDB_MI_RESULT_EXIT:
            return "exit";
        default:
            return "error";
    }
}

GdbMiResultClass
gdb_mi_result_class_from_string (const gchar *str)
{
    if (str == NULL)
    {
        return GDB_MI_RESULT_ERROR;
    }

    if (g_strcmp0 (str, "done") == 0)
        return GDB_MI_RESULT_DONE;
    if (g_strcmp0 (str, "running") == 0)
        return GDB_MI_RESULT_RUNNING;
    if (g_strcmp0 (str, "connected") == 0)
        return GDB_MI_RESULT_CONNECTED;
    if (g_strcmp0 (str, "error") == 0)
        return GDB_MI_RESULT_ERROR;
    if (g_strcmp0 (str, "exit") == 0)
        return GDB_MI_RESULT_EXIT;

    return GDB_MI_RESULT_ERROR;
}
