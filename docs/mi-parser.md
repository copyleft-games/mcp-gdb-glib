# GDB Machine Interface (MI) Parser

## Overview

The GDB MCP Server uses GDB's Machine Interface (MI) for structured communication. The MI protocol provides machine-parseable output instead of the human-readable console output.

## MI Output Format

### Record Types

MI output consists of different record types:

| Prefix | Type | Description |
|--------|------|-------------|
| `^` | Result | Synchronous command results |
| `*` | Exec Async | Execution state changes |
| `+` | Status Async | Status updates |
| `=` | Notify Async | Async notifications |
| `~` | Console Stream | Console output |
| `@` | Target Stream | Target output |
| `&` | Log Stream | Log output |

### Result Classes

Result records (`^`) have a class:

- `^done` - Command completed successfully
- `^running` - Program started running
- `^error` - Command failed
- `^exit` - GDB is exiting
- `^connected` - Connected to target

### Examples

```
# Successful breakpoint set
^done,bkpt={number="1",type="breakpoint",disp="keep",enabled="y",addr="0x00400520",func="main",file="test.c",line="10"}

# Program stopped
*stopped,reason="breakpoint-hit",disp="keep",bkptno="1",frame={addr="0x00400520",func="main",args=[],file="test.c",line="10"}

# Console output
~"Breakpoint 1, main () at test.c:10\n"

# Error
^error,msg="No symbol table is loaded."
```

## GdbMiParser

The `GdbMiParser` class parses MI output into structured data.

### Key Functions

```c
/* Parse a single line of MI output */
GdbMiRecord *gdb_mi_parser_parse_line (GdbMiParser *self,
                                        const gchar *line);

/* Unescape MI string (handles \n, \t, \", \\) */
gchar *gdb_mi_parser_unescape_string (const gchar *str);
```

### GdbMiRecord

Parsed records are stored in a boxed type:

```c
struct _GdbMiRecord
{
    gint ref_count;
    GdbMiRecordType type;
    GdbMiResultClass result_class;  /* For result records */
    JsonNode *data;                  /* Structured data */
    gchar *raw_output;              /* Original line */
};
```

### Record Type Enum

```c
typedef enum
{
    GDB_MI_RECORD_RESULT,
    GDB_MI_RECORD_EXEC_ASYNC,
    GDB_MI_RECORD_STATUS_ASYNC,
    GDB_MI_RECORD_NOTIFY_ASYNC,
    GDB_MI_RECORD_CONSOLE,
    GDB_MI_RECORD_TARGET,
    GDB_MI_RECORD_LOG
} GdbMiRecordType;
```

### Result Class Enum

```c
typedef enum
{
    GDB_MI_RESULT_DONE,
    GDB_MI_RESULT_RUNNING,
    GDB_MI_RESULT_CONNECTED,
    GDB_MI_RESULT_ERROR,
    GDB_MI_RESULT_EXIT
} GdbMiResultClass;
```

## MI Data Structures

### Tuples

Tuples are key-value pairs enclosed in braces:

```
{name="value",other="data"}
```

Parsed as JsonObject.

### Lists

Lists are enclosed in brackets:

```
[{item="1"},{item="2"}]
```

Parsed as JsonArray.

### Values

Values can be:
- Strings: `"text"`
- Tuples: `{...}`
- Lists: `[...]`

## Usage in Tool Handlers

Tool handlers use `gdb_tools_execute_command_sync()` which:

1. Sends command to GDB
2. Waits for result record
3. Returns console output as string

For advanced parsing:

```c
g_autoptr(GdbMiParser) parser = gdb_mi_parser_new ();

/* Parse a line */
g_autoptr(GdbMiRecord) record = gdb_mi_parser_parse_line (parser, line);

/* Check record type */
if (gdb_mi_record_get_record_type (record) == GDB_MI_RECORD_RESULT)
{
    /* Check result class */
    if (gdb_mi_record_get_result_class (record) == GDB_MI_RESULT_DONE)
    {
        /* Get structured data */
        JsonNode *data = gdb_mi_record_get_data (record);
        /* Process data... */
    }
}
```

## MI Mode

GDB is started with MI mode enabled:

```bash
gdb --interpreter=mi2
```

The `mi2` interpreter provides the most compatible MI version.

## References

- [GDB MI Documentation](https://sourceware.org/gdb/current/onlinedocs/gdb/GDB_002fMI.html)
- [GDB MI Output Syntax](https://sourceware.org/gdb/current/onlinedocs/gdb/GDB_002fMI-Output-Syntax.html)
