/*
 * gdb-mi-parser.h - GDB/MI output parser for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This parser handles GDB Machine Interface (MI) output format.
 * See: https://sourceware.org/gdb/current/onlinedocs/gdb.html/GDB_002fMI.html
 */

#ifndef GDB_MI_PARSER_H
#define GDB_MI_PARSER_H

#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "gdb-enums.h"

G_BEGIN_DECLS

#define GDB_TYPE_MI_PARSER (gdb_mi_parser_get_type ())

G_DECLARE_FINAL_TYPE (GdbMiParser, gdb_mi_parser, GDB, MI_PARSER, GObject)

/**
 * GdbMiRecord:
 *
 * Represents a parsed GDB/MI output record.
 * This is a reference-counted boxed type.
 */
typedef struct _GdbMiRecord GdbMiRecord;

#define GDB_TYPE_MI_RECORD (gdb_mi_record_get_type ())

GType gdb_mi_record_get_type (void) G_GNUC_CONST;

/**
 * gdb_mi_record_ref:
 * @record: a #GdbMiRecord
 *
 * Increases the reference count of @record.
 *
 * Returns: (transfer full): @record
 */
GdbMiRecord *gdb_mi_record_ref (GdbMiRecord *record);

/**
 * gdb_mi_record_unref:
 * @record: a #GdbMiRecord
 *
 * Decreases the reference count of @record.
 * When the count reaches zero, the record is freed.
 */
void gdb_mi_record_unref (GdbMiRecord *record);

/**
 * gdb_mi_record_get_type_enum:
 * @record: a #GdbMiRecord
 *
 * Gets the record type.
 *
 * Returns: the #GdbMiRecordType
 */
GdbMiRecordType gdb_mi_record_get_type_enum (GdbMiRecord *record);

/**
 * gdb_mi_record_get_class:
 * @record: a #GdbMiRecord
 *
 * Gets the record class (e.g., "done", "stopped", "breakpoint-created").
 * Only valid for result and async records.
 *
 * Returns: (transfer none) (nullable): the class string, or %NULL
 */
const gchar *gdb_mi_record_get_class (GdbMiRecord *record);

/**
 * gdb_mi_record_get_result_class:
 * @record: a #GdbMiRecord
 *
 * Gets the result class for result records.
 *
 * Returns: the #GdbMiResultClass
 */
GdbMiResultClass gdb_mi_record_get_result_class (GdbMiRecord *record);

/**
 * gdb_mi_record_get_results:
 * @record: a #GdbMiRecord
 *
 * Gets the results data as a JsonObject.
 * Only valid for result and async records.
 *
 * Returns: (transfer none) (nullable): the results, or %NULL
 */
JsonObject *gdb_mi_record_get_results (GdbMiRecord *record);

/**
 * gdb_mi_record_get_stream_content:
 * @record: a #GdbMiRecord
 *
 * Gets the stream content.
 * Only valid for console, target, and log records.
 *
 * Returns: (transfer none) (nullable): the content string, or %NULL
 */
const gchar *gdb_mi_record_get_stream_content (GdbMiRecord *record);

/**
 * gdb_mi_record_get_token:
 * @record: a #GdbMiRecord
 *
 * Gets the optional token associated with the record.
 * Tokens are used to match responses to commands.
 *
 * Returns: the token, or -1 if none
 */
gint64 gdb_mi_record_get_token (GdbMiRecord *record);

/**
 * gdb_mi_record_is_error:
 * @record: a #GdbMiRecord
 *
 * Checks if this is an error record.
 *
 * Returns: %TRUE if this is an error record
 */
gboolean gdb_mi_record_is_error (GdbMiRecord *record);

/**
 * gdb_mi_record_get_error_message:
 * @record: a #GdbMiRecord
 *
 * Gets the error message from an error record.
 *
 * Returns: (transfer none) (nullable): the error message, or %NULL
 */
const gchar *gdb_mi_record_get_error_message (GdbMiRecord *record);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GdbMiRecord, gdb_mi_record_unref)


/* ========================================================================== */
/* GdbMiParser                                                                */
/* ========================================================================== */

/**
 * gdb_mi_parser_new:
 *
 * Creates a new GDB/MI parser.
 *
 * Returns: (transfer full): a new #GdbMiParser
 */
GdbMiParser *gdb_mi_parser_new (void);

/**
 * gdb_mi_parser_parse_line:
 * @self: a #GdbMiParser
 * @line: the line to parse
 * @error: (nullable): return location for a #GError
 *
 * Parses a single line of GDB/MI output.
 *
 * Returns: (transfer full) (nullable): a #GdbMiRecord, or %NULL on error
 */
GdbMiRecord *gdb_mi_parser_parse_line (GdbMiParser  *self,
                                       const gchar  *line,
                                       GError      **error);

/**
 * gdb_mi_parser_is_prompt:
 * @line: the line to check
 *
 * Checks if a line is the GDB prompt.
 *
 * Returns: %TRUE if this is the prompt
 */
gboolean gdb_mi_parser_is_prompt (const gchar *line);

/**
 * gdb_mi_parser_is_result_complete:
 * @line: the line to check
 *
 * Checks if a line indicates command completion.
 * This includes ^done, ^error, ^exit, and (gdb) prompt.
 *
 * Returns: %TRUE if this indicates completion
 */
gboolean gdb_mi_parser_is_result_complete (const gchar *line);

/**
 * gdb_mi_parser_unescape_string:
 * @str: the MI-escaped string (including quotes)
 *
 * Unescapes a GDB/MI C-style string.
 *
 * Returns: (transfer full): the unescaped string
 */
gchar *gdb_mi_parser_unescape_string (const gchar *str);

G_END_DECLS

#endif /* GDB_MI_PARSER_H */
