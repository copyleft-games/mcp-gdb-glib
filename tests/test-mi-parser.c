/*
 * test-mi-parser.c - Unit tests for GDB/MI parser
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include "mcp-gdb/gdb-mi-parser.h"
#include "mcp-gdb/gdb-error.h"

/* ========================================================================== */
/* GdbMiParser Basic Tests                                                    */
/* ========================================================================== */

static void
test_mi_parser_new (void)
{
    g_autoptr(GdbMiParser) parser = NULL;

    parser = gdb_mi_parser_new ();
    g_assert_nonnull (parser);
    g_assert_true (GDB_IS_MI_PARSER (parser));
}

static void
test_mi_parser_is_prompt (void)
{
    /* Standard prompt */
    g_assert_true (gdb_mi_parser_is_prompt ("(gdb)"));
    g_assert_true (gdb_mi_parser_is_prompt ("(gdb) "));
    g_assert_true (gdb_mi_parser_is_prompt ("  (gdb)"));
    g_assert_true (gdb_mi_parser_is_prompt ("\t(gdb)"));

    /* Not prompts */
    g_assert_false (gdb_mi_parser_is_prompt ("gdb"));
    g_assert_false (gdb_mi_parser_is_prompt ("(gdb"));
    g_assert_false (gdb_mi_parser_is_prompt ("gdb)"));
    g_assert_false (gdb_mi_parser_is_prompt ("^done"));
    g_assert_false (gdb_mi_parser_is_prompt (""));
    g_assert_false (gdb_mi_parser_is_prompt (NULL));
}

static void
test_mi_parser_is_result_complete (void)
{
    /* Complete indicators */
    g_assert_true (gdb_mi_parser_is_result_complete ("(gdb)"));
    g_assert_true (gdb_mi_parser_is_result_complete ("^done"));
    g_assert_true (gdb_mi_parser_is_result_complete ("^running"));
    g_assert_true (gdb_mi_parser_is_result_complete ("^connected"));
    g_assert_true (gdb_mi_parser_is_result_complete ("^error"));
    g_assert_true (gdb_mi_parser_is_result_complete ("^exit"));
    g_assert_true (gdb_mi_parser_is_result_complete ("123^done"));

    /* Not complete indicators */
    g_assert_false (gdb_mi_parser_is_result_complete ("*stopped"));
    g_assert_false (gdb_mi_parser_is_result_complete ("~\"text\""));
    g_assert_false (gdb_mi_parser_is_result_complete ("=thread-created"));
    g_assert_false (gdb_mi_parser_is_result_complete (""));
    g_assert_false (gdb_mi_parser_is_result_complete (NULL));
}

static void
test_mi_parser_unescape_string (void)
{
    g_autofree gchar *result = NULL;

    /* Simple string */
    result = gdb_mi_parser_unescape_string ("\"hello\"");
    g_assert_cmpstr (result, ==, "hello");
    g_free (result);

    /* Newline escape */
    result = gdb_mi_parser_unescape_string ("\"hello\\nworld\"");
    g_assert_cmpstr (result, ==, "hello\nworld");
    g_free (result);

    /* Tab escape */
    result = gdb_mi_parser_unescape_string ("\"hello\\tworld\"");
    g_assert_cmpstr (result, ==, "hello\tworld");
    g_free (result);

    /* Backslash escape */
    result = gdb_mi_parser_unescape_string ("\"path\\\\to\\\\file\"");
    g_assert_cmpstr (result, ==, "path\\to\\file");
    g_free (result);

    /* Quote escape */
    result = gdb_mi_parser_unescape_string ("\"say \\\"hello\\\"\"");
    g_assert_cmpstr (result, ==, "say \"hello\"");
    g_free (result);

    /* Without surrounding quotes */
    result = gdb_mi_parser_unescape_string ("no quotes");
    g_assert_cmpstr (result, ==, "no quotes");
    g_free (result);

    /* Empty string */
    result = gdb_mi_parser_unescape_string ("\"\"");
    g_assert_cmpstr (result, ==, "");
    g_free (result);

    /* NULL input */
    result = gdb_mi_parser_unescape_string (NULL);
    g_assert_cmpstr (result, ==, "");
}


/* ========================================================================== */
/* GdbMiRecord Tests                                                          */
/* ========================================================================== */

static void
test_mi_record_ref_unref (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    GdbMiRecord *record1 = NULL;
    GdbMiRecord *record2 = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record1 = gdb_mi_parser_parse_line (parser, "^done", &error);
    g_assert_no_error (error);
    g_assert_nonnull (record1);

    /* Ref should return the same pointer */
    record2 = gdb_mi_record_ref (record1);
    g_assert_true (record1 == record2);

    /* Unref both - should not crash */
    gdb_mi_record_unref (record2);
    gdb_mi_record_unref (record1);

    /* Unref NULL should be safe */
    gdb_mi_record_unref (NULL);
}

static void
test_mi_record_get_type (void)
{
    GType type = gdb_mi_record_get_type ();

    g_assert_cmpuint (type, !=, G_TYPE_INVALID);
    g_assert_cmpuint (GDB_TYPE_MI_RECORD, ==, type);
    g_assert_true (G_TYPE_IS_BOXED (type));
}


/* ========================================================================== */
/* Parse Result Records                                                       */
/* ========================================================================== */

static void
test_parse_result_done (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "^done", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_RESULT);
    g_assert_cmpint (gdb_mi_record_get_result_class (record), ==, GDB_MI_RESULT_DONE);
    g_assert_cmpstr (gdb_mi_record_get_class (record), ==, "done");
    g_assert_cmpint (gdb_mi_record_get_token (record), ==, -1);
    g_assert_false (gdb_mi_record_is_error (record));
}

static void
test_parse_result_done_with_data (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "^done,value=\"42\"", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_result_class (record), ==, GDB_MI_RESULT_DONE);

    results = gdb_mi_record_get_results (record);
    g_assert_nonnull (results);
    g_assert_true (json_object_has_member (results, "value"));
    g_assert_cmpstr (json_object_get_string_member (results, "value"), ==, "42");
}

static void
test_parse_result_running (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "^running", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_result_class (record), ==, GDB_MI_RESULT_RUNNING);
}

static void
test_parse_result_connected (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "^connected", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_result_class (record), ==, GDB_MI_RESULT_CONNECTED);
}

static void
test_parse_result_error (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "^error,msg=\"Command failed\"", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_result_class (record), ==, GDB_MI_RESULT_ERROR);
    g_assert_true (gdb_mi_record_is_error (record));
    g_assert_cmpstr (gdb_mi_record_get_error_message (record), ==, "Command failed");
}

static void
test_parse_result_exit (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "^exit", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_result_class (record), ==, GDB_MI_RESULT_EXIT);
}

static void
test_parse_result_with_token (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "123^done", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_token (record), ==, 123);
    g_assert_cmpint (gdb_mi_record_get_result_class (record), ==, GDB_MI_RESULT_DONE);
}


/* ========================================================================== */
/* Parse Async Records                                                        */
/* ========================================================================== */

static void
test_parse_exec_stopped (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\"",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_EXEC_ASYNC);
    g_assert_cmpstr (gdb_mi_record_get_class (record), ==, "stopped");

    results = gdb_mi_record_get_results (record);
    g_assert_nonnull (results);
    g_assert_cmpstr (json_object_get_string_member (results, "reason"), ==, "breakpoint-hit");
    g_assert_cmpstr (json_object_get_string_member (results, "bkptno"), ==, "1");
}

static void
test_parse_exec_running (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "*running,thread-id=\"all\"",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_EXEC_ASYNC);
    g_assert_cmpstr (gdb_mi_record_get_class (record), ==, "running");

    results = gdb_mi_record_get_results (record);
    g_assert_cmpstr (json_object_get_string_member (results, "thread-id"), ==, "all");
}

static void
test_parse_notify_async (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "=thread-created,id=\"1\",group-id=\"i1\"",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_NOTIFY_ASYNC);
    g_assert_cmpstr (gdb_mi_record_get_class (record), ==, "thread-created");
}

static void
test_parse_status_async (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "+download,section=\".text\",section-size=\"1024\"",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_STATUS_ASYNC);
    g_assert_cmpstr (gdb_mi_record_get_class (record), ==, "download");
}


/* ========================================================================== */
/* Parse Stream Records                                                       */
/* ========================================================================== */

static void
test_parse_console_output (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "~\"Hello world\\n\"", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_CONSOLE);
    g_assert_cmpstr (gdb_mi_record_get_stream_content (record), ==, "Hello world\n");
}

static void
test_parse_target_output (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "@\"target output\"", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_TARGET);
    g_assert_cmpstr (gdb_mi_record_get_stream_content (record), ==, "target output");
}

static void
test_parse_log_output (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "&\"log message\"", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_LOG);
    g_assert_cmpstr (gdb_mi_record_get_stream_content (record), ==, "log message");
}


/* ========================================================================== */
/* Parse Complex Structures                                                   */
/* ========================================================================== */

static void
test_parse_nested_tuple (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;
    JsonObject *frame;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "^done,frame={addr=\"0x1234\",func=\"main\",file=\"test.c\",line=\"10\"}",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);

    results = gdb_mi_record_get_results (record);
    g_assert_nonnull (results);
    g_assert_true (json_object_has_member (results, "frame"));

    frame = json_object_get_object_member (results, "frame");
    g_assert_nonnull (frame);
    g_assert_cmpstr (json_object_get_string_member (frame, "addr"), ==, "0x1234");
    g_assert_cmpstr (json_object_get_string_member (frame, "func"), ==, "main");
    g_assert_cmpstr (json_object_get_string_member (frame, "file"), ==, "test.c");
    g_assert_cmpstr (json_object_get_string_member (frame, "line"), ==, "10");
}

static void
test_parse_list_values (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;
    JsonArray *groups;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "^done,groups=[\"i1\",\"i2\",\"i3\"]",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);

    results = gdb_mi_record_get_results (record);
    g_assert_nonnull (results);
    g_assert_true (json_object_has_member (results, "groups"));

    groups = json_object_get_array_member (results, "groups");
    g_assert_nonnull (groups);
    g_assert_cmpint (json_array_get_length (groups), ==, 3);
    g_assert_cmpstr (json_array_get_string_element (groups, 0), ==, "i1");
    g_assert_cmpstr (json_array_get_string_element (groups, 1), ==, "i2");
    g_assert_cmpstr (json_array_get_string_element (groups, 2), ==, "i3");
}

static void
test_parse_list_tuples (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;
    JsonArray *stack;
    JsonObject *frame0;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "^done,stack=[{level=\"0\",func=\"main\"},{level=\"1\",func=\"start\"}]",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);

    results = gdb_mi_record_get_results (record);
    g_assert_nonnull (results);

    stack = json_object_get_array_member (results, "stack");
    g_assert_nonnull (stack);
    g_assert_cmpint (json_array_get_length (stack), ==, 2);

    frame0 = json_array_get_object_element (stack, 0);
    g_assert_nonnull (frame0);
    g_assert_cmpstr (json_object_get_string_member (frame0, "level"), ==, "0");
    g_assert_cmpstr (json_object_get_string_member (frame0, "func"), ==, "main");
}

static void
test_parse_backtrace (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;
    JsonArray *stack;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "^done,stack=[frame={level=\"0\",addr=\"0x1149\",func=\"main\",file=\"test.c\",line=\"5\"}]",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);

    results = gdb_mi_record_get_results (record);
    g_assert_nonnull (results);
    g_assert_true (json_object_has_member (results, "stack"));

    stack = json_object_get_array_member (results, "stack");
    g_assert_nonnull (stack);
    g_assert_cmpint (json_array_get_length (stack), >=, 1);
}


/* ========================================================================== */
/* Parse Prompt and Edge Cases                                                */
/* ========================================================================== */

static void
test_parse_prompt (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "(gdb)", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);
    g_assert_cmpint (gdb_mi_record_get_type_enum (record), ==, GDB_MI_RECORD_PROMPT);
}

static void
test_parse_empty_tuple (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;
    JsonObject *empty;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "^done,empty={}", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);

    results = gdb_mi_record_get_results (record);
    g_assert_true (json_object_has_member (results, "empty"));

    empty = json_object_get_object_member (results, "empty");
    g_assert_nonnull (empty);
    g_assert_cmpint (json_object_get_size (empty), ==, 0);
}

static void
test_parse_empty_list (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;
    JsonArray *empty;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser, "^done,items=[]", &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);

    results = gdb_mi_record_get_results (record);
    g_assert_true (json_object_has_member (results, "items"));

    empty = json_object_get_array_member (results, "items");
    g_assert_nonnull (empty);
    g_assert_cmpint (json_array_get_length (empty), ==, 0);
}

static void
test_parse_multiple_results (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;
    JsonObject *results;

    parser = gdb_mi_parser_new ();
    record = gdb_mi_parser_parse_line (parser,
        "^done,a=\"1\",b=\"2\",c=\"3\"",
        &error);

    g_assert_no_error (error);
    g_assert_nonnull (record);

    results = gdb_mi_record_get_results (record);
    g_assert_cmpint (json_object_get_size (results), ==, 3);
    g_assert_cmpstr (json_object_get_string_member (results, "a"), ==, "1");
    g_assert_cmpstr (json_object_get_string_member (results, "b"), ==, "2");
    g_assert_cmpstr (json_object_get_string_member (results, "c"), ==, "3");
}


/* ========================================================================== */
/* Record Accessor Tests                                                      */
/* ========================================================================== */

static void
test_record_is_error_false_for_non_result (void)
{
    g_autoptr(GdbMiParser) parser = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autoptr(GError) error = NULL;

    parser = gdb_mi_parser_new ();

    /* Console record should not be an error */
    record = gdb_mi_parser_parse_line (parser, "~\"text\"", &error);
    g_assert_false (gdb_mi_record_is_error (record));
    g_assert_null (gdb_mi_record_get_error_message (record));
}


/* ========================================================================== */
/* Main                                                                       */
/* ========================================================================== */

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    /* Parser basic tests */
    g_test_add_func ("/gdb/mi-parser/new", test_mi_parser_new);
    g_test_add_func ("/gdb/mi-parser/is-prompt", test_mi_parser_is_prompt);
    g_test_add_func ("/gdb/mi-parser/is-result-complete", test_mi_parser_is_result_complete);
    g_test_add_func ("/gdb/mi-parser/unescape-string", test_mi_parser_unescape_string);

    /* Record tests */
    g_test_add_func ("/gdb/mi-record/ref-unref", test_mi_record_ref_unref);
    g_test_add_func ("/gdb/mi-record/get-type", test_mi_record_get_type);

    /* Parse result records */
    g_test_add_func ("/gdb/mi-parser/parse/result-done", test_parse_result_done);
    g_test_add_func ("/gdb/mi-parser/parse/result-done-with-data", test_parse_result_done_with_data);
    g_test_add_func ("/gdb/mi-parser/parse/result-running", test_parse_result_running);
    g_test_add_func ("/gdb/mi-parser/parse/result-connected", test_parse_result_connected);
    g_test_add_func ("/gdb/mi-parser/parse/result-error", test_parse_result_error);
    g_test_add_func ("/gdb/mi-parser/parse/result-exit", test_parse_result_exit);
    g_test_add_func ("/gdb/mi-parser/parse/result-with-token", test_parse_result_with_token);

    /* Parse async records */
    g_test_add_func ("/gdb/mi-parser/parse/exec-stopped", test_parse_exec_stopped);
    g_test_add_func ("/gdb/mi-parser/parse/exec-running", test_parse_exec_running);
    g_test_add_func ("/gdb/mi-parser/parse/notify-async", test_parse_notify_async);
    g_test_add_func ("/gdb/mi-parser/parse/status-async", test_parse_status_async);

    /* Parse stream records */
    g_test_add_func ("/gdb/mi-parser/parse/console-output", test_parse_console_output);
    g_test_add_func ("/gdb/mi-parser/parse/target-output", test_parse_target_output);
    g_test_add_func ("/gdb/mi-parser/parse/log-output", test_parse_log_output);

    /* Parse complex structures */
    g_test_add_func ("/gdb/mi-parser/parse/nested-tuple", test_parse_nested_tuple);
    g_test_add_func ("/gdb/mi-parser/parse/list-values", test_parse_list_values);
    g_test_add_func ("/gdb/mi-parser/parse/list-tuples", test_parse_list_tuples);
    g_test_add_func ("/gdb/mi-parser/parse/backtrace", test_parse_backtrace);

    /* Prompt and edge cases */
    g_test_add_func ("/gdb/mi-parser/parse/prompt", test_parse_prompt);
    g_test_add_func ("/gdb/mi-parser/parse/empty-tuple", test_parse_empty_tuple);
    g_test_add_func ("/gdb/mi-parser/parse/empty-list", test_parse_empty_list);
    g_test_add_func ("/gdb/mi-parser/parse/multiple-results", test_parse_multiple_results);

    /* Accessor tests */
    g_test_add_func ("/gdb/mi-record/is-error-non-result", test_record_is_error_false_for_non_result);

    return g_test_run ();
}
