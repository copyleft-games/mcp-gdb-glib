/*
 * test-error.c - Unit tests for GDB error handling
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include <gio/gio.h>
#include "mcp-gdb/gdb-error.h"

/* ========================================================================== */
/* Error Quark Tests                                                          */
/* ========================================================================== */

static void
test_error_quark (void)
{
    GQuark quark1;
    GQuark quark2;

    quark1 = gdb_error_quark ();
    quark2 = gdb_error_quark ();

    /* Quark should be non-zero */
    g_assert_cmpuint (quark1, !=, 0);

    /* Quark should be consistent */
    g_assert_cmpuint (quark1, ==, quark2);

    /* GDB_ERROR macro should work */
    g_assert_cmpuint (GDB_ERROR, ==, quark1);
}

static void
test_error_quark_string (void)
{
    GQuark quark;
    const gchar *str;

    quark = gdb_error_quark ();
    str = g_quark_to_string (quark);

    g_assert_cmpstr (str, ==, "gdb-error-quark");
}


/* ========================================================================== */
/* Error Code Tests                                                           */
/* ========================================================================== */

static void
test_error_code_to_string (void)
{
    /* Test all error codes have non-NULL descriptions */
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_SESSION_NOT_FOUND));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_SESSION_NOT_READY));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_SESSION_LIMIT));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_SPAWN_FAILED));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_TIMEOUT));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_COMMAND_FAILED));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_PARSE_ERROR));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_INVALID_ARGUMENT));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_FILE_NOT_FOUND));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_ATTACH_FAILED));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_ALREADY_RUNNING));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_NOT_RUNNING));
    g_assert_nonnull (gdb_error_code_to_string (GDB_ERROR_INTERNAL));
}

static void
test_error_code_to_string_content (void)
{
    /* Test specific error messages */
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_SESSION_NOT_FOUND), ==, "Session not found");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_SESSION_NOT_READY), ==, "Session not ready for commands");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_SESSION_LIMIT), ==, "Maximum session count reached");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_SPAWN_FAILED), ==, "Failed to spawn GDB process");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_TIMEOUT), ==, "Command timed out");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_COMMAND_FAILED), ==, "GDB command failed");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_PARSE_ERROR), ==, "Failed to parse MI output");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_INVALID_ARGUMENT), ==, "Invalid argument");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_FILE_NOT_FOUND), ==, "File not found");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_ATTACH_FAILED), ==, "Failed to attach to process");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_ALREADY_RUNNING), ==, "Session already has a running program");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_NOT_RUNNING), ==, "No program is running");
    g_assert_cmpstr (gdb_error_code_to_string (GDB_ERROR_INTERNAL), ==, "Internal error");
}

static void
test_error_code_unknown (void)
{
    /* Unknown error code should return "Unknown error" */
    g_assert_cmpstr (gdb_error_code_to_string ((GdbErrorCode)999), ==, "Unknown error");
    g_assert_cmpstr (gdb_error_code_to_string ((GdbErrorCode)-1), ==, "Unknown error");
}


/* ========================================================================== */
/* GError Integration Tests                                                   */
/* ========================================================================== */

static void
test_error_creation (void)
{
    g_autoptr(GError) error = NULL;

    error = g_error_new (GDB_ERROR, GDB_ERROR_SESSION_NOT_FOUND, "Session %s not found", "test-123");

    g_assert_nonnull (error);
    g_assert_error (error, GDB_ERROR, GDB_ERROR_SESSION_NOT_FOUND);
    g_assert_cmpstr (error->message, ==, "Session test-123 not found");
}

static void
test_error_matches (void)
{
    g_autoptr(GError) error = NULL;

    error = g_error_new_literal (GDB_ERROR, GDB_ERROR_TIMEOUT, "Command timed out");

    g_assert_true (g_error_matches (error, GDB_ERROR, GDB_ERROR_TIMEOUT));
    g_assert_false (g_error_matches (error, GDB_ERROR, GDB_ERROR_SESSION_NOT_FOUND));
    g_assert_false (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT));
}

static void
test_error_propagation (void)
{
    g_autoptr(GError) error = NULL;
    GError *local_error = NULL;

    /* Simulate error propagation from inner function */
    local_error = g_error_new (GDB_ERROR, GDB_ERROR_COMMAND_FAILED, "Failed");
    g_propagate_error (&error, local_error);

    /* Verify error was propagated */
    g_assert_nonnull (error);
    g_assert_error (error, GDB_ERROR, GDB_ERROR_COMMAND_FAILED);

    /* Test g_set_error pattern */
    g_clear_error (&error);
    g_set_error (&error, GDB_ERROR, GDB_ERROR_PARSE_ERROR, "Parse error at line %d", 42);

    g_assert_nonnull (error);
    g_assert_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR);
    g_assert_cmpstr (error->message, ==, "Parse error at line 42");
}

static void
test_error_copy (void)
{
    g_autoptr(GError) original = NULL;
    g_autoptr(GError) copy = NULL;

    original = g_error_new (GDB_ERROR, GDB_ERROR_SPAWN_FAILED, "Cannot spawn GDB");
    copy = g_error_copy (original);

    g_assert_nonnull (copy);
    g_assert_true (copy != original);
    g_assert_cmpuint (copy->domain, ==, original->domain);
    g_assert_cmpint (copy->code, ==, original->code);
    g_assert_cmpstr (copy->message, ==, original->message);
}

static void
test_error_all_codes_unique (void)
{
    /* Verify all error codes are unique (no duplicates in enum) */
    GdbErrorCode codes[] = {
        GDB_ERROR_SESSION_NOT_FOUND,
        GDB_ERROR_SESSION_NOT_READY,
        GDB_ERROR_SESSION_LIMIT,
        GDB_ERROR_SPAWN_FAILED,
        GDB_ERROR_TIMEOUT,
        GDB_ERROR_COMMAND_FAILED,
        GDB_ERROR_PARSE_ERROR,
        GDB_ERROR_INVALID_ARGUMENT,
        GDB_ERROR_FILE_NOT_FOUND,
        GDB_ERROR_ATTACH_FAILED,
        GDB_ERROR_ALREADY_RUNNING,
        GDB_ERROR_NOT_RUNNING,
        GDB_ERROR_INTERNAL
    };
    gsize i, j;

    for (i = 0; i < G_N_ELEMENTS (codes); i++)
    {
        for (j = i + 1; j < G_N_ELEMENTS (codes); j++)
        {
            g_assert_cmpint (codes[i], !=, codes[j]);
        }
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

    /* Quark tests */
    g_test_add_func ("/gdb/error/quark", test_error_quark);
    g_test_add_func ("/gdb/error/quark-string", test_error_quark_string);

    /* Error code tests */
    g_test_add_func ("/gdb/error/code-to-string", test_error_code_to_string);
    g_test_add_func ("/gdb/error/code-to-string-content", test_error_code_to_string_content);
    g_test_add_func ("/gdb/error/code-unknown", test_error_code_unknown);

    /* GError integration tests */
    g_test_add_func ("/gdb/error/creation", test_error_creation);
    g_test_add_func ("/gdb/error/matches", test_error_matches);
    g_test_add_func ("/gdb/error/propagation", test_error_propagation);
    g_test_add_func ("/gdb/error/copy", test_error_copy);
    g_test_add_func ("/gdb/error/all-codes-unique", test_error_all_codes_unique);

    return g_test_run ();
}
