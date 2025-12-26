/*
 * gdb-enums.h - Enumeration types for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef GDB_ENUMS_H
#define GDB_ENUMS_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * GdbSessionState:
 * @GDB_SESSION_STATE_DISCONNECTED: Not connected to GDB process
 * @GDB_SESSION_STATE_STARTING: GDB process is starting
 * @GDB_SESSION_STATE_READY: GDB is ready to accept commands
 * @GDB_SESSION_STATE_RUNNING: Target program is running
 * @GDB_SESSION_STATE_STOPPED: Target program is stopped
 * @GDB_SESSION_STATE_TERMINATED: GDB process has terminated
 * @GDB_SESSION_STATE_ERROR: Session is in an error state
 *
 * State of a GDB debugging session.
 */
typedef enum {
    GDB_SESSION_STATE_DISCONNECTED,
    GDB_SESSION_STATE_STARTING,
    GDB_SESSION_STATE_READY,
    GDB_SESSION_STATE_RUNNING,
    GDB_SESSION_STATE_STOPPED,
    GDB_SESSION_STATE_TERMINATED,
    GDB_SESSION_STATE_ERROR
} GdbSessionState;

GType gdb_session_state_get_type (void) G_GNUC_CONST;
#define GDB_TYPE_SESSION_STATE (gdb_session_state_get_type ())

/**
 * gdb_session_state_to_string:
 * @state: a #GdbSessionState
 *
 * Converts a session state to its string representation.
 *
 * Returns: (transfer none): the string representation
 */
const gchar *gdb_session_state_to_string (GdbSessionState state);

/**
 * gdb_session_state_from_string:
 * @str: a string representation
 *
 * Converts a string to its session state value.
 *
 * Returns: the #GdbSessionState, or %GDB_SESSION_STATE_DISCONNECTED if unknown
 */
GdbSessionState gdb_session_state_from_string (const gchar *str);


/**
 * GdbStopReason:
 * @GDB_STOP_REASON_BREAKPOINT: Stopped at a breakpoint
 * @GDB_STOP_REASON_WATCHPOINT: Watchpoint triggered
 * @GDB_STOP_REASON_SIGNAL: Signal received
 * @GDB_STOP_REASON_STEP: Step operation completed
 * @GDB_STOP_REASON_FINISH: Function return completed
 * @GDB_STOP_REASON_EXITED: Program exited with status
 * @GDB_STOP_REASON_EXITED_NORMALLY: Program exited normally
 * @GDB_STOP_REASON_EXITED_SIGNALLED: Program exited due to signal
 * @GDB_STOP_REASON_UNKNOWN: Unknown stop reason
 *
 * Reason why the target program stopped.
 */
typedef enum {
    GDB_STOP_REASON_BREAKPOINT,
    GDB_STOP_REASON_WATCHPOINT,
    GDB_STOP_REASON_SIGNAL,
    GDB_STOP_REASON_STEP,
    GDB_STOP_REASON_FINISH,
    GDB_STOP_REASON_EXITED,
    GDB_STOP_REASON_EXITED_NORMALLY,
    GDB_STOP_REASON_EXITED_SIGNALLED,
    GDB_STOP_REASON_UNKNOWN
} GdbStopReason;

GType gdb_stop_reason_get_type (void) G_GNUC_CONST;
#define GDB_TYPE_STOP_REASON (gdb_stop_reason_get_type ())

/**
 * gdb_stop_reason_to_string:
 * @reason: a #GdbStopReason
 *
 * Converts a stop reason to its string representation.
 *
 * Returns: (transfer none): the string representation
 */
const gchar *gdb_stop_reason_to_string (GdbStopReason reason);

/**
 * gdb_stop_reason_from_string:
 * @str: a string representation (GDB/MI format)
 *
 * Converts a GDB/MI stop reason string to its enum value.
 *
 * Returns: the #GdbStopReason, or %GDB_STOP_REASON_UNKNOWN if unknown
 */
GdbStopReason gdb_stop_reason_from_string (const gchar *str);


/**
 * GdbMiRecordType:
 * @GDB_MI_RECORD_RESULT: Result record (^done, ^running, ^error, ^exit)
 * @GDB_MI_RECORD_EXEC_ASYNC: Exec async record (*stopped, *running)
 * @GDB_MI_RECORD_STATUS_ASYNC: Status async record (+download, etc.)
 * @GDB_MI_RECORD_NOTIFY_ASYNC: Notify async record (=thread-created, etc.)
 * @GDB_MI_RECORD_CONSOLE: Console stream record (~"output")
 * @GDB_MI_RECORD_TARGET: Target stream record (@"output")
 * @GDB_MI_RECORD_LOG: Log stream record (&"output")
 * @GDB_MI_RECORD_PROMPT: GDB prompt (gdb)
 * @GDB_MI_RECORD_UNKNOWN: Unknown record type
 *
 * Type of GDB/MI output record.
 */
typedef enum {
    GDB_MI_RECORD_RESULT,
    GDB_MI_RECORD_EXEC_ASYNC,
    GDB_MI_RECORD_STATUS_ASYNC,
    GDB_MI_RECORD_NOTIFY_ASYNC,
    GDB_MI_RECORD_CONSOLE,
    GDB_MI_RECORD_TARGET,
    GDB_MI_RECORD_LOG,
    GDB_MI_RECORD_PROMPT,
    GDB_MI_RECORD_UNKNOWN
} GdbMiRecordType;

GType gdb_mi_record_type_get_type (void) G_GNUC_CONST;
#define GDB_TYPE_MI_RECORD_TYPE (gdb_mi_record_type_get_type ())

/**
 * gdb_mi_record_type_to_string:
 * @type: a #GdbMiRecordType
 *
 * Converts a MI record type to its string representation.
 *
 * Returns: (transfer none): the string representation
 */
const gchar *gdb_mi_record_type_to_string (GdbMiRecordType type);

/**
 * gdb_mi_record_type_from_char:
 * @c: the prefix character from GDB/MI output
 *
 * Determines the record type from the GDB/MI prefix character.
 *
 * Returns: the #GdbMiRecordType
 */
GdbMiRecordType gdb_mi_record_type_from_char (gchar c);


/**
 * GdbMiResultClass:
 * @GDB_MI_RESULT_DONE: Command completed successfully (^done)
 * @GDB_MI_RESULT_RUNNING: Target is running (^running)
 * @GDB_MI_RESULT_CONNECTED: Connected to target (^connected)
 * @GDB_MI_RESULT_ERROR: Command error (^error)
 * @GDB_MI_RESULT_EXIT: GDB exit (^exit)
 *
 * Class of a GDB/MI result record.
 */
typedef enum {
    GDB_MI_RESULT_DONE,
    GDB_MI_RESULT_RUNNING,
    GDB_MI_RESULT_CONNECTED,
    GDB_MI_RESULT_ERROR,
    GDB_MI_RESULT_EXIT
} GdbMiResultClass;

GType gdb_mi_result_class_get_type (void) G_GNUC_CONST;
#define GDB_TYPE_MI_RESULT_CLASS (gdb_mi_result_class_get_type ())

/**
 * gdb_mi_result_class_to_string:
 * @result_class: a #GdbMiResultClass
 *
 * Converts a MI result class to its string representation.
 *
 * Returns: (transfer none): the string representation
 */
const gchar *gdb_mi_result_class_to_string (GdbMiResultClass result_class);

/**
 * gdb_mi_result_class_from_string:
 * @str: a string representation
 *
 * Converts a string to its MI result class value.
 *
 * Returns: the #GdbMiResultClass, or %GDB_MI_RESULT_ERROR if unknown
 */
GdbMiResultClass gdb_mi_result_class_from_string (const gchar *str);

G_END_DECLS

#endif /* GDB_ENUMS_H */
