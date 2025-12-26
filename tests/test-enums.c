/*
 * test-enums.c - Unit tests for GDB enumeration types
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include "mcp-gdb/gdb-enums.h"

/* ========================================================================== */
/* GdbSessionState Tests                                                      */
/* ========================================================================== */

static void
test_session_state_to_string (void)
{
    g_assert_cmpstr (gdb_session_state_to_string (GDB_SESSION_STATE_DISCONNECTED), ==, "disconnected");
    g_assert_cmpstr (gdb_session_state_to_string (GDB_SESSION_STATE_STARTING), ==, "starting");
    g_assert_cmpstr (gdb_session_state_to_string (GDB_SESSION_STATE_READY), ==, "ready");
    g_assert_cmpstr (gdb_session_state_to_string (GDB_SESSION_STATE_RUNNING), ==, "running");
    g_assert_cmpstr (gdb_session_state_to_string (GDB_SESSION_STATE_STOPPED), ==, "stopped");
    g_assert_cmpstr (gdb_session_state_to_string (GDB_SESSION_STATE_TERMINATED), ==, "terminated");
    g_assert_cmpstr (gdb_session_state_to_string (GDB_SESSION_STATE_ERROR), ==, "error");

    /* Unknown value should return "unknown" */
    g_assert_cmpstr (gdb_session_state_to_string ((GdbSessionState)999), ==, "unknown");
}

static void
test_session_state_from_string (void)
{
    g_assert_cmpint (gdb_session_state_from_string ("disconnected"), ==, GDB_SESSION_STATE_DISCONNECTED);
    g_assert_cmpint (gdb_session_state_from_string ("starting"), ==, GDB_SESSION_STATE_STARTING);
    g_assert_cmpint (gdb_session_state_from_string ("ready"), ==, GDB_SESSION_STATE_READY);
    g_assert_cmpint (gdb_session_state_from_string ("running"), ==, GDB_SESSION_STATE_RUNNING);
    g_assert_cmpint (gdb_session_state_from_string ("stopped"), ==, GDB_SESSION_STATE_STOPPED);
    g_assert_cmpint (gdb_session_state_from_string ("terminated"), ==, GDB_SESSION_STATE_TERMINATED);
    g_assert_cmpint (gdb_session_state_from_string ("error"), ==, GDB_SESSION_STATE_ERROR);

    /* Unknown strings should return DISCONNECTED */
    g_assert_cmpint (gdb_session_state_from_string ("unknown"), ==, GDB_SESSION_STATE_DISCONNECTED);
    g_assert_cmpint (gdb_session_state_from_string ("invalid"), ==, GDB_SESSION_STATE_DISCONNECTED);
    g_assert_cmpint (gdb_session_state_from_string (NULL), ==, GDB_SESSION_STATE_DISCONNECTED);
}

static void
test_session_state_get_type (void)
{
    GType type = gdb_session_state_get_type ();

    g_assert_true (G_TYPE_IS_ENUM (type));
    g_assert_cmpstr (g_type_name (type), ==, "GdbSessionState");

    /* Verify GType macro works */
    g_assert_cmpuint (GDB_TYPE_SESSION_STATE, ==, type);
}


/* ========================================================================== */
/* GdbStopReason Tests                                                        */
/* ========================================================================== */

static void
test_stop_reason_to_string (void)
{
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_BREAKPOINT), ==, "breakpoint-hit");
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_WATCHPOINT), ==, "watchpoint-trigger");
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_SIGNAL), ==, "signal-received");
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_STEP), ==, "end-stepping-range");
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_FINISH), ==, "function-finished");
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_EXITED), ==, "exited");
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_EXITED_NORMALLY), ==, "exited-normally");
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_EXITED_SIGNALLED), ==, "exited-signalled");
    g_assert_cmpstr (gdb_stop_reason_to_string (GDB_STOP_REASON_UNKNOWN), ==, "unknown");

    /* Unknown value should return "unknown" */
    g_assert_cmpstr (gdb_stop_reason_to_string ((GdbStopReason)999), ==, "unknown");
}

static void
test_stop_reason_from_string (void)
{
    g_assert_cmpint (gdb_stop_reason_from_string ("breakpoint-hit"), ==, GDB_STOP_REASON_BREAKPOINT);
    g_assert_cmpint (gdb_stop_reason_from_string ("watchpoint-trigger"), ==, GDB_STOP_REASON_WATCHPOINT);
    g_assert_cmpint (gdb_stop_reason_from_string ("read-watchpoint-trigger"), ==, GDB_STOP_REASON_WATCHPOINT);
    g_assert_cmpint (gdb_stop_reason_from_string ("access-watchpoint-trigger"), ==, GDB_STOP_REASON_WATCHPOINT);
    g_assert_cmpint (gdb_stop_reason_from_string ("signal-received"), ==, GDB_STOP_REASON_SIGNAL);
    g_assert_cmpint (gdb_stop_reason_from_string ("end-stepping-range"), ==, GDB_STOP_REASON_STEP);
    g_assert_cmpint (gdb_stop_reason_from_string ("function-finished"), ==, GDB_STOP_REASON_FINISH);
    g_assert_cmpint (gdb_stop_reason_from_string ("exited"), ==, GDB_STOP_REASON_EXITED);
    g_assert_cmpint (gdb_stop_reason_from_string ("exited-normally"), ==, GDB_STOP_REASON_EXITED_NORMALLY);
    g_assert_cmpint (gdb_stop_reason_from_string ("exited-signalled"), ==, GDB_STOP_REASON_EXITED_SIGNALLED);

    /* Unknown strings should return UNKNOWN */
    g_assert_cmpint (gdb_stop_reason_from_string ("invalid"), ==, GDB_STOP_REASON_UNKNOWN);
    g_assert_cmpint (gdb_stop_reason_from_string (NULL), ==, GDB_STOP_REASON_UNKNOWN);
}

static void
test_stop_reason_get_type (void)
{
    GType type = gdb_stop_reason_get_type ();

    g_assert_true (G_TYPE_IS_ENUM (type));
    g_assert_cmpstr (g_type_name (type), ==, "GdbStopReason");

    /* Verify GType macro works */
    g_assert_cmpuint (GDB_TYPE_STOP_REASON, ==, type);
}


/* ========================================================================== */
/* GdbMiRecordType Tests                                                      */
/* ========================================================================== */

static void
test_mi_record_type_to_string (void)
{
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_RESULT), ==, "result");
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_EXEC_ASYNC), ==, "exec-async");
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_STATUS_ASYNC), ==, "status-async");
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_NOTIFY_ASYNC), ==, "notify-async");
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_CONSOLE), ==, "console");
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_TARGET), ==, "target");
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_LOG), ==, "log");
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_PROMPT), ==, "prompt");
    g_assert_cmpstr (gdb_mi_record_type_to_string (GDB_MI_RECORD_UNKNOWN), ==, "unknown");

    /* Unknown value should return "unknown" */
    g_assert_cmpstr (gdb_mi_record_type_to_string ((GdbMiRecordType)999), ==, "unknown");
}

static void
test_mi_record_type_from_char (void)
{
    g_assert_cmpint (gdb_mi_record_type_from_char ('^'), ==, GDB_MI_RECORD_RESULT);
    g_assert_cmpint (gdb_mi_record_type_from_char ('*'), ==, GDB_MI_RECORD_EXEC_ASYNC);
    g_assert_cmpint (gdb_mi_record_type_from_char ('+'), ==, GDB_MI_RECORD_STATUS_ASYNC);
    g_assert_cmpint (gdb_mi_record_type_from_char ('='), ==, GDB_MI_RECORD_NOTIFY_ASYNC);
    g_assert_cmpint (gdb_mi_record_type_from_char ('~'), ==, GDB_MI_RECORD_CONSOLE);
    g_assert_cmpint (gdb_mi_record_type_from_char ('@'), ==, GDB_MI_RECORD_TARGET);
    g_assert_cmpint (gdb_mi_record_type_from_char ('&'), ==, GDB_MI_RECORD_LOG);

    /* Unknown characters should return UNKNOWN */
    g_assert_cmpint (gdb_mi_record_type_from_char ('!'), ==, GDB_MI_RECORD_UNKNOWN);
    g_assert_cmpint (gdb_mi_record_type_from_char ('#'), ==, GDB_MI_RECORD_UNKNOWN);
    g_assert_cmpint (gdb_mi_record_type_from_char ('\0'), ==, GDB_MI_RECORD_UNKNOWN);
}

static void
test_mi_record_type_get_type (void)
{
    GType type = gdb_mi_record_type_get_type ();

    g_assert_true (G_TYPE_IS_ENUM (type));
    g_assert_cmpstr (g_type_name (type), ==, "GdbMiRecordType");

    /* Verify GType macro works */
    g_assert_cmpuint (GDB_TYPE_MI_RECORD_TYPE, ==, type);
}


/* ========================================================================== */
/* GdbMiResultClass Tests                                                     */
/* ========================================================================== */

static void
test_mi_result_class_to_string (void)
{
    g_assert_cmpstr (gdb_mi_result_class_to_string (GDB_MI_RESULT_DONE), ==, "done");
    g_assert_cmpstr (gdb_mi_result_class_to_string (GDB_MI_RESULT_RUNNING), ==, "running");
    g_assert_cmpstr (gdb_mi_result_class_to_string (GDB_MI_RESULT_CONNECTED), ==, "connected");
    g_assert_cmpstr (gdb_mi_result_class_to_string (GDB_MI_RESULT_ERROR), ==, "error");
    g_assert_cmpstr (gdb_mi_result_class_to_string (GDB_MI_RESULT_EXIT), ==, "exit");

    /* Unknown value should return "error" (as per implementation) */
    g_assert_cmpstr (gdb_mi_result_class_to_string ((GdbMiResultClass)999), ==, "error");
}

static void
test_mi_result_class_from_string (void)
{
    g_assert_cmpint (gdb_mi_result_class_from_string ("done"), ==, GDB_MI_RESULT_DONE);
    g_assert_cmpint (gdb_mi_result_class_from_string ("running"), ==, GDB_MI_RESULT_RUNNING);
    g_assert_cmpint (gdb_mi_result_class_from_string ("connected"), ==, GDB_MI_RESULT_CONNECTED);
    g_assert_cmpint (gdb_mi_result_class_from_string ("error"), ==, GDB_MI_RESULT_ERROR);
    g_assert_cmpint (gdb_mi_result_class_from_string ("exit"), ==, GDB_MI_RESULT_EXIT);

    /* Unknown strings should return ERROR */
    g_assert_cmpint (gdb_mi_result_class_from_string ("invalid"), ==, GDB_MI_RESULT_ERROR);
    g_assert_cmpint (gdb_mi_result_class_from_string (NULL), ==, GDB_MI_RESULT_ERROR);
}

static void
test_mi_result_class_get_type (void)
{
    GType type = gdb_mi_result_class_get_type ();

    g_assert_true (G_TYPE_IS_ENUM (type));
    g_assert_cmpstr (g_type_name (type), ==, "GdbMiResultClass");

    /* Verify GType macro works */
    g_assert_cmpuint (GDB_TYPE_MI_RESULT_CLASS, ==, type);
}


/* ========================================================================== */
/* Roundtrip Tests                                                            */
/* ========================================================================== */

static void
test_session_state_roundtrip (void)
{
    GdbSessionState states[] = {
        GDB_SESSION_STATE_DISCONNECTED,
        GDB_SESSION_STATE_STARTING,
        GDB_SESSION_STATE_READY,
        GDB_SESSION_STATE_RUNNING,
        GDB_SESSION_STATE_STOPPED,
        GDB_SESSION_STATE_TERMINATED,
        GDB_SESSION_STATE_ERROR
    };
    gsize i;

    for (i = 0; i < G_N_ELEMENTS (states); i++)
    {
        const gchar *str = gdb_session_state_to_string (states[i]);
        GdbSessionState result = gdb_session_state_from_string (str);
        g_assert_cmpint (result, ==, states[i]);
    }
}

static void
test_stop_reason_roundtrip (void)
{
    GdbStopReason reasons[] = {
        GDB_STOP_REASON_BREAKPOINT,
        GDB_STOP_REASON_WATCHPOINT,
        GDB_STOP_REASON_SIGNAL,
        GDB_STOP_REASON_STEP,
        GDB_STOP_REASON_FINISH,
        GDB_STOP_REASON_EXITED,
        GDB_STOP_REASON_EXITED_NORMALLY,
        GDB_STOP_REASON_EXITED_SIGNALLED,
        GDB_STOP_REASON_UNKNOWN
    };
    gsize i;

    for (i = 0; i < G_N_ELEMENTS (reasons); i++)
    {
        const gchar *str = gdb_stop_reason_to_string (reasons[i]);
        GdbStopReason result = gdb_stop_reason_from_string (str);
        g_assert_cmpint (result, ==, reasons[i]);
    }
}

static void
test_mi_result_class_roundtrip (void)
{
    GdbMiResultClass classes[] = {
        GDB_MI_RESULT_DONE,
        GDB_MI_RESULT_RUNNING,
        GDB_MI_RESULT_CONNECTED,
        GDB_MI_RESULT_ERROR,
        GDB_MI_RESULT_EXIT
    };
    gsize i;

    for (i = 0; i < G_N_ELEMENTS (classes); i++)
    {
        const gchar *str = gdb_mi_result_class_to_string (classes[i]);
        GdbMiResultClass result = gdb_mi_result_class_from_string (str);
        g_assert_cmpint (result, ==, classes[i]);
    }
}


/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    /* GdbSessionState tests */
    g_test_add_func ("/gdb/enums/session-state/to-string", test_session_state_to_string);
    g_test_add_func ("/gdb/enums/session-state/from-string", test_session_state_from_string);
    g_test_add_func ("/gdb/enums/session-state/get-type", test_session_state_get_type);
    g_test_add_func ("/gdb/enums/session-state/roundtrip", test_session_state_roundtrip);

    /* GdbStopReason tests */
    g_test_add_func ("/gdb/enums/stop-reason/to-string", test_stop_reason_to_string);
    g_test_add_func ("/gdb/enums/stop-reason/from-string", test_stop_reason_from_string);
    g_test_add_func ("/gdb/enums/stop-reason/get-type", test_stop_reason_get_type);
    g_test_add_func ("/gdb/enums/stop-reason/roundtrip", test_stop_reason_roundtrip);

    /* GdbMiRecordType tests */
    g_test_add_func ("/gdb/enums/mi-record-type/to-string", test_mi_record_type_to_string);
    g_test_add_func ("/gdb/enums/mi-record-type/from-char", test_mi_record_type_from_char);
    g_test_add_func ("/gdb/enums/mi-record-type/get-type", test_mi_record_type_get_type);

    /* GdbMiResultClass tests */
    g_test_add_func ("/gdb/enums/mi-result-class/to-string", test_mi_result_class_to_string);
    g_test_add_func ("/gdb/enums/mi-result-class/from-string", test_mi_result_class_from_string);
    g_test_add_func ("/gdb/enums/mi-result-class/get-type", test_mi_result_class_get_type);
    g_test_add_func ("/gdb/enums/mi-result-class/roundtrip", test_mi_result_class_roundtrip);

    return g_test_run ();
}
