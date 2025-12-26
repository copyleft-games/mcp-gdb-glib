/*
 * gdb-mi-parser.c - GDB/MI output parser implementation for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Parses GDB Machine Interface (MI) output into structured data.
 * Reference: https://sourceware.org/gdb/current/onlinedocs/gdb.html/GDB_002fMI.html
 *
 * MI Output Grammar (simplified):
 *   output       -> out-of-band-record* result-record? "(gdb)"
 *   result-record -> [token] "^" result-class ("," result)* nl
 *   out-of-band-record -> async-record | stream-record
 *   async-record -> [token] async-class ("," result)* nl
 *   stream-record -> stream-type c-string nl
 *   result       -> variable "=" value
 *   value        -> const | tuple | list
 *   const        -> c-string
 *   tuple        -> "{}" | "{" result ("," result)* "}"
 *   list         -> "[]" | "[" value ("," value)* "]"
 */

#include "mcp-gdb/gdb-mi-parser.h"
#include "mcp-gdb/gdb-error.h"

#include <string.h>
#include <ctype.h>

/* ========================================================================== */
/* GdbMiRecord Boxed Type                                                     */
/* ========================================================================== */

struct _GdbMiRecord
{
    volatile gint  ref_count;
    GdbMiRecordType type;
    GdbMiResultClass result_class;
    gchar          *class_name;     /* e.g., "done", "stopped", "breakpoint-created" */
    JsonObject     *results;        /* Parsed results as JSON */
    gchar          *stream_content; /* For stream records */
    gint64          token;          /* Command token, -1 if none */
};

static GdbMiRecord *
gdb_mi_record_new (GdbMiRecordType type)
{
    GdbMiRecord *record;

    record = g_slice_new0 (GdbMiRecord);
    record->ref_count = 1;
    record->type = type;
    record->result_class = GDB_MI_RESULT_DONE;
    record->token = -1;

    return record;
}

GdbMiRecord *
gdb_mi_record_ref (GdbMiRecord *record)
{
    g_return_val_if_fail (record != NULL, NULL);

    g_atomic_int_inc (&record->ref_count);

    return record;
}

void
gdb_mi_record_unref (GdbMiRecord *record)
{
    if (record == NULL)
    {
        return;
    }

    if (g_atomic_int_dec_and_test (&record->ref_count))
    {
        g_clear_pointer (&record->class_name, g_free);
        g_clear_pointer (&record->stream_content, g_free);
        if (record->results != NULL)
        {
            json_object_unref (record->results);
        }
        g_slice_free (GdbMiRecord, record);
    }
}

G_DEFINE_BOXED_TYPE (GdbMiRecord, gdb_mi_record,
                     gdb_mi_record_ref, gdb_mi_record_unref)

GdbMiRecordType
gdb_mi_record_get_type_enum (GdbMiRecord *record)
{
    g_return_val_if_fail (record != NULL, GDB_MI_RECORD_UNKNOWN);
    return record->type;
}

const gchar *
gdb_mi_record_get_class (GdbMiRecord *record)
{
    g_return_val_if_fail (record != NULL, NULL);
    return record->class_name;
}

GdbMiResultClass
gdb_mi_record_get_result_class (GdbMiRecord *record)
{
    g_return_val_if_fail (record != NULL, GDB_MI_RESULT_ERROR);
    return record->result_class;
}

JsonObject *
gdb_mi_record_get_results (GdbMiRecord *record)
{
    g_return_val_if_fail (record != NULL, NULL);
    return record->results;
}

const gchar *
gdb_mi_record_get_stream_content (GdbMiRecord *record)
{
    g_return_val_if_fail (record != NULL, NULL);
    return record->stream_content;
}

gint64
gdb_mi_record_get_token (GdbMiRecord *record)
{
    g_return_val_if_fail (record != NULL, -1);
    return record->token;
}

gboolean
gdb_mi_record_is_error (GdbMiRecord *record)
{
    g_return_val_if_fail (record != NULL, FALSE);

    if (record->type != GDB_MI_RECORD_RESULT)
    {
        return FALSE;
    }

    return record->result_class == GDB_MI_RESULT_ERROR;
}

const gchar *
gdb_mi_record_get_error_message (GdbMiRecord *record)
{
    JsonObject *results;

    g_return_val_if_fail (record != NULL, NULL);

    if (!gdb_mi_record_is_error (record))
    {
        return NULL;
    }

    results = gdb_mi_record_get_results (record);
    if (results == NULL)
    {
        return NULL;
    }

    if (json_object_has_member (results, "msg"))
    {
        return json_object_get_string_member (results, "msg");
    }

    return NULL;
}


/* ========================================================================== */
/* GdbMiParser GObject                                                        */
/* ========================================================================== */

struct _GdbMiParser
{
    GObject parent_instance;
};

G_DEFINE_TYPE (GdbMiParser, gdb_mi_parser, G_TYPE_OBJECT)

static void
gdb_mi_parser_class_init (GdbMiParserClass *klass G_GNUC_UNUSED)
{
    /* No special cleanup needed */
}

static void
gdb_mi_parser_init (GdbMiParser *self G_GNUC_UNUSED)
{
    /* Nothing to initialize */
}

GdbMiParser *
gdb_mi_parser_new (void)
{
    return (GdbMiParser *)g_object_new (GDB_TYPE_MI_PARSER, NULL);
}


/* ========================================================================== */
/* Parsing Helper Functions                                                   */
/* ========================================================================== */

gchar *
gdb_mi_parser_unescape_string (const gchar *str)
{
    GString *result;
    const gchar *p;
    gsize len;

    if (str == NULL)
    {
        return g_strdup ("");
    }

    len = strlen (str);

    /* Skip surrounding quotes if present */
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"')
    {
        str++;
        len -= 2;
    }

    result = g_string_sized_new (len);

    for (p = str; p < str + len; p++)
    {
        if (*p == '\\' && p + 1 < str + len)
        {
            p++;
            switch (*p)
            {
                case 'n':
                    g_string_append_c (result, '\n');
                    break;
                case 't':
                    g_string_append_c (result, '\t');
                    break;
                case 'r':
                    g_string_append_c (result, '\r');
                    break;
                case '\\':
                    g_string_append_c (result, '\\');
                    break;
                case '"':
                    g_string_append_c (result, '"');
                    break;
                case '0':
                    /* Octal escape - simplified handling */
                    g_string_append_c (result, '\0');
                    break;
                default:
                    /* Unknown escape, keep as-is */
                    g_string_append_c (result, '\\');
                    g_string_append_c (result, *p);
                    break;
            }
        }
        else
        {
            g_string_append_c (result, *p);
        }
    }

    return g_string_free (result, FALSE);
}

gboolean
gdb_mi_parser_is_prompt (const gchar *line)
{
    if (line == NULL)
    {
        return FALSE;
    }

    /* Strip leading whitespace */
    while (*line && g_ascii_isspace (*line))
    {
        line++;
    }

    return g_strcmp0 (line, "(gdb)") == 0 ||
           g_str_has_prefix (line, "(gdb) ");
}

gboolean
gdb_mi_parser_is_result_complete (const gchar *line)
{
    if (line == NULL)
    {
        return FALSE;
    }

    /* Check for prompt */
    if (gdb_mi_parser_is_prompt (line))
    {
        return TRUE;
    }

    /* Check for result record prefixes */
    /* Skip optional token (digits) */
    while (*line && g_ascii_isdigit (*line))
    {
        line++;
    }

    if (*line == '^')
    {
        return TRUE;
    }

    return FALSE;
}


/* Forward declarations for recursive parsing */
static JsonNode *parse_value (const gchar **p, GError **error);
static JsonObject *parse_tuple (const gchar **p, GError **error);
static JsonArray *parse_list (const gchar **p, GError **error);

/*
 * skip_whitespace:
 *
 * Advances pointer past any whitespace.
 */
static void
skip_whitespace (const gchar **p)
{
    while (**p && g_ascii_isspace (**p))
    {
        (*p)++;
    }
}

/*
 * parse_c_string:
 *
 * Parses a C-style quoted string.
 * Returns the unescaped content.
 */
static gchar *
parse_c_string (const gchar **p,
                GError      **error)
{
    GString *str;

    if (**p != '"')
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Expected '\"' at start of string");
        return NULL;
    }

    (*p)++; /* Skip opening quote */
    str = g_string_new (NULL);

    while (**p && **p != '"')
    {
        if (**p == '\\' && *(*p + 1))
        {
            (*p)++;
            switch (**p)
            {
                case 'n':
                    g_string_append_c (str, '\n');
                    break;
                case 't':
                    g_string_append_c (str, '\t');
                    break;
                case 'r':
                    g_string_append_c (str, '\r');
                    break;
                case '\\':
                    g_string_append_c (str, '\\');
                    break;
                case '"':
                    g_string_append_c (str, '"');
                    break;
                default:
                    g_string_append_c (str, **p);
                    break;
            }
        }
        else
        {
            g_string_append_c (str, **p);
        }
        (*p)++;
    }

    if (**p == '"')
    {
        (*p)++; /* Skip closing quote */
    }

    return g_string_free (str, FALSE);
}

/*
 * parse_variable:
 *
 * Parses a variable name (alphanumeric + underscore + hyphen).
 */
static gchar *
parse_variable (const gchar **p)
{
    const gchar *start;

    start = *p;

    while (**p && (g_ascii_isalnum (**p) || **p == '_' || **p == '-'))
    {
        (*p)++;
    }

    if (*p == start)
    {
        return NULL;
    }

    return g_strndup (start, *p - start);
}

/*
 * parse_value:
 *
 * Parses a value: const (c-string), tuple, or list.
 */
static JsonNode *
parse_value (const gchar **p,
             GError      **error)
{
    JsonNode *node = NULL;

    skip_whitespace (p);

    if (**p == '"')
    {
        /* C-string constant */
        g_autofree gchar *str = parse_c_string (p, error);
        if (str == NULL)
        {
            return NULL;
        }
        node = json_node_new (JSON_NODE_VALUE);
        json_node_set_string (node, str);
    }
    else if (**p == '{')
    {
        /* Tuple */
        g_autoptr(JsonObject) obj = parse_tuple (p, error);
        if (obj == NULL)
        {
            return NULL;
        }
        node = json_node_new (JSON_NODE_OBJECT);
        json_node_set_object (node, obj);
    }
    else if (**p == '[')
    {
        /* List */
        g_autoptr(JsonArray) arr = parse_list (p, error);
        if (arr == NULL)
        {
            return NULL;
        }
        node = json_node_new (JSON_NODE_ARRAY);
        json_node_set_array (node, arr);
    }
    else
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Unexpected character '%c' when parsing value", **p);
        return NULL;
    }

    return node;
}

/*
 * parse_result:
 *
 * Parses a result: variable "=" value.
 * Adds the result to the given object.
 */
static gboolean
parse_result (const gchar **p,
              JsonObject   *obj,
              GError      **error)
{
    g_autofree gchar *name = NULL;
    g_autoptr(JsonNode) value = NULL;

    skip_whitespace (p);

    name = parse_variable (p);
    if (name == NULL)
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Expected variable name");
        return FALSE;
    }

    skip_whitespace (p);

    if (**p != '=')
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Expected '=' after variable name '%s'", name);
        return FALSE;
    }
    (*p)++; /* Skip '=' */

    skip_whitespace (p);

    value = parse_value (p, error);
    if (value == NULL)
    {
        return FALSE;
    }

    json_object_set_member (obj, name, g_steal_pointer (&value));

    return TRUE;
}

/*
 * parse_tuple:
 *
 * Parses a tuple: "{}" or "{" result ("," result)* "}".
 */
static JsonObject *
parse_tuple (const gchar **p,
             GError      **error)
{
    JsonObject *obj;

    if (**p != '{')
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Expected '{' for tuple");
        return NULL;
    }
    (*p)++; /* Skip '{' */

    obj = json_object_new ();

    skip_whitespace (p);

    if (**p == '}')
    {
        (*p)++; /* Empty tuple */
        return obj;
    }

    /* Parse first result */
    if (!parse_result (p, obj, error))
    {
        json_object_unref (obj);
        return NULL;
    }

    /* Parse remaining results */
    while (**p == ',')
    {
        (*p)++; /* Skip ',' */
        if (!parse_result (p, obj, error))
        {
            json_object_unref (obj);
            return NULL;
        }
    }

    skip_whitespace (p);

    if (**p != '}')
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Expected '}' to close tuple");
        json_object_unref (obj);
        return NULL;
    }
    (*p)++; /* Skip '}' */

    return obj;
}

/*
 * parse_list:
 *
 * Parses a list: "[]" or "[" value ("," value)* "]".
 * Also handles lists of results: "[" result ("," result)* "]".
 */
static JsonArray *
parse_list (const gchar **p,
            GError      **error)
{
    JsonArray *arr;

    if (**p != '[')
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Expected '[' for list");
        return NULL;
    }
    (*p)++; /* Skip '[' */

    arr = json_array_new ();

    skip_whitespace (p);

    if (**p == ']')
    {
        (*p)++; /* Empty list */
        return arr;
    }

    /* Check if this is a list of results (name=value) or values */
    const gchar *lookahead = *p;
    gboolean is_result_list = FALSE;

    /* Look for pattern: identifier '=' */
    while (*lookahead && (g_ascii_isalnum (*lookahead) || *lookahead == '_' || *lookahead == '-'))
    {
        lookahead++;
    }
    if (*lookahead == '=')
    {
        is_result_list = TRUE;
    }

    if (is_result_list)
    {
        /* List of results - convert to array of objects */
        g_autoptr(JsonObject) obj = json_object_new ();

        if (!parse_result (p, obj, error))
        {
            json_array_unref (arr);
            return NULL;
        }

        /* Wrap single result in object */
        JsonNode *node = json_node_new (JSON_NODE_OBJECT);
        json_node_set_object (node, g_steal_pointer (&obj));
        json_array_add_element (arr, node);

        while (**p == ',')
        {
            (*p)++; /* Skip ',' */
            skip_whitespace (p);

            /* Check again if next is result or value */
            lookahead = *p;
            is_result_list = FALSE;
            while (*lookahead && (g_ascii_isalnum (*lookahead) || *lookahead == '_' || *lookahead == '-'))
            {
                lookahead++;
            }
            if (*lookahead == '=')
            {
                is_result_list = TRUE;
            }

            if (is_result_list)
            {
                obj = json_object_new ();
                if (!parse_result (p, obj, error))
                {
                    json_array_unref (arr);
                    return NULL;
                }
                node = json_node_new (JSON_NODE_OBJECT);
                json_node_set_object (node, g_steal_pointer (&obj));
                json_array_add_element (arr, node);
            }
            else
            {
                JsonNode *val = parse_value (p, error);
                if (val == NULL)
                {
                    json_array_unref (arr);
                    return NULL;
                }
                json_array_add_element (arr, val);
            }
        }
    }
    else
    {
        /* List of values */
        JsonNode *val = parse_value (p, error);
        if (val == NULL)
        {
            json_array_unref (arr);
            return NULL;
        }
        json_array_add_element (arr, val);

        while (**p == ',')
        {
            (*p)++; /* Skip ',' */
            val = parse_value (p, error);
            if (val == NULL)
            {
                json_array_unref (arr);
                return NULL;
            }
            json_array_add_element (arr, val);
        }
    }

    skip_whitespace (p);

    if (**p != ']')
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Expected ']' to close list");
        json_array_unref (arr);
        return NULL;
    }
    (*p)++; /* Skip ']' */

    return arr;
}

/*
 * parse_results:
 *
 * Parses comma-separated results into a JsonObject.
 */
static JsonObject *
parse_results (const gchar **p,
               GError      **error)
{
    JsonObject *obj;

    obj = json_object_new ();

    skip_whitespace (p);

    /* Check if there are any results */
    if (**p == '\0' || **p == '\n')
    {
        return obj;
    }

    /* Expect comma before first result (after class) */
    if (**p == ',')
    {
        (*p)++; /* Skip ',' */
    }

    while (**p && **p != '\n' && **p != '\0')
    {
        if (!parse_result (p, obj, error))
        {
            json_object_unref (obj);
            return NULL;
        }

        skip_whitespace (p);

        if (**p == ',')
        {
            (*p)++; /* Skip ',' */
        }
        else
        {
            break;
        }
    }

    return obj;
}


/* ========================================================================== */
/* Main Parsing Function                                                      */
/* ========================================================================== */

GdbMiRecord *
gdb_mi_parser_parse_line (GdbMiParser  *self,
                          const gchar  *line,
                          GError      **error)
{
    GdbMiRecord *record;
    const gchar *p;
    gint64 token = -1;

    g_return_val_if_fail (GDB_IS_MI_PARSER (self), NULL);
    g_return_val_if_fail (line != NULL, NULL);

    p = line;

    /* Check for prompt */
    if (gdb_mi_parser_is_prompt (line))
    {
        record = gdb_mi_record_new (GDB_MI_RECORD_PROMPT);
        return record;
    }

    /* Parse optional token (sequence of digits) */
    if (g_ascii_isdigit (*p))
    {
        token = 0;
        while (g_ascii_isdigit (*p))
        {
            token = token * 10 + (*p - '0');
            p++;
        }
    }

    /* Determine record type from prefix character */
    GdbMiRecordType type = gdb_mi_record_type_from_char (*p);

    if (type == GDB_MI_RECORD_UNKNOWN)
    {
        g_set_error (error, GDB_ERROR, GDB_ERROR_PARSE_ERROR,
                     "Unknown MI record prefix: '%c'", *p);
        return NULL;
    }

    record = gdb_mi_record_new (type);
    record->token = token;
    p++; /* Skip prefix character */

    /* Handle stream records (console, target, log) */
    if (type == GDB_MI_RECORD_CONSOLE ||
        type == GDB_MI_RECORD_TARGET ||
        type == GDB_MI_RECORD_LOG)
    {
        if (*p == '"')
        {
            record->stream_content = parse_c_string (&p, error);
            if (record->stream_content == NULL)
            {
                gdb_mi_record_unref (record);
                return NULL;
            }
        }
        else
        {
            /* Just copy remaining content */
            record->stream_content = g_strdup (p);
        }
        return record;
    }

    /* Parse class name for result and async records */
    {
        const gchar *class_start = p;
        while (*p && (g_ascii_isalnum (*p) || *p == '-' || *p == '_'))
        {
            p++;
        }
        record->class_name = g_strndup (class_start, p - class_start);
    }

    /* For result records, determine the result class */
    if (type == GDB_MI_RECORD_RESULT)
    {
        record->result_class = gdb_mi_result_class_from_string (record->class_name);
    }

    /* Parse results if present */
    if (*p == ',' || *p == ' ')
    {
        record->results = parse_results (&p, error);
        if (record->results == NULL)
        {
            gdb_mi_record_unref (record);
            return NULL;
        }
    }
    else
    {
        record->results = json_object_new ();
    }

    return record;
}
