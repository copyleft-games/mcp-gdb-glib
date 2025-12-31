/*
 * gdb-session.c - GDB session management implementation for mcp-gdb
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "mcp-gdb/gdb-session.h"
#include "mcp-gdb/gdb-error.h"

#include <gio/gio.h>
#include <string.h>

#define DEFAULT_TIMEOUT_MS 10000
#define DEFAULT_GDB_PATH   "gdb"

/* ========================================================================== */
/* GdbSession Structure                                                       */
/* ========================================================================== */

struct _GdbSession
{
    GObject          parent_instance;

    /* Identity */
    gchar           *session_id;
    gchar           *gdb_path;
    gchar           *working_dir;

    /* Target info */
    gchar           *target_program;

    /* Process management */
    GSubprocess     *process;
    GOutputStream   *stdin_pipe;
    GDataInputStream *stdout_reader;

    /* State */
    GdbSessionState  state;
    guint            timeout_ms;

    /* MI Parser */
    GdbMiParser     *mi_parser;
};

/* ========================================================================== */
/* Property IDs                                                               */
/* ========================================================================== */

enum {
    PROP_0,
    PROP_SESSION_ID,
    PROP_GDB_PATH,
    PROP_WORKING_DIR,
    PROP_TARGET_PROGRAM,
    PROP_STATE,
    PROP_TIMEOUT_MS,
    N_PROPS
};

static GParamSpec *properties[N_PROPS];

/* ========================================================================== */
/* Signal IDs                                                                 */
/* ========================================================================== */

enum {
    SIGNAL_STATE_CHANGED,
    SIGNAL_STOPPED,
    SIGNAL_CONSOLE_OUTPUT,
    SIGNAL_READY,
    SIGNAL_TERMINATED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE (GdbSession, gdb_session, G_TYPE_OBJECT)

/* ========================================================================== */
/* Private Helper Functions                                                   */
/* ========================================================================== */

static void
set_state (GdbSession      *self,
           GdbSessionState  new_state)
{
    GdbSessionState old_state;

    if (self->state == new_state)
    {
        return;
    }

    old_state = self->state;
    self->state = new_state;

    g_signal_emit (self, signals[SIGNAL_STATE_CHANGED], 0, old_state, new_state);

    if (new_state == GDB_SESSION_STATE_READY)
    {
        g_signal_emit (self, signals[SIGNAL_READY], 0);
    }
}

/* ========================================================================== */
/* GObject Implementation                                                     */
/* ========================================================================== */

static void
gdb_session_dispose (GObject *object)
{
    GdbSession *self = GDB_SESSION (object);

    gdb_session_terminate (self);

    g_clear_object (&self->mi_parser);

    G_OBJECT_CLASS (gdb_session_parent_class)->dispose (object);
}

static void
gdb_session_finalize (GObject *object)
{
    GdbSession *self = GDB_SESSION (object);

    g_clear_pointer (&self->session_id, g_free);
    g_clear_pointer (&self->gdb_path, g_free);
    g_clear_pointer (&self->working_dir, g_free);
    g_clear_pointer (&self->target_program, g_free);

    G_OBJECT_CLASS (gdb_session_parent_class)->finalize (object);
}

static void
gdb_session_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    GdbSession *self = GDB_SESSION (object);

    switch (prop_id)
    {
        case PROP_SESSION_ID:
            g_value_set_string (value, self->session_id);
            break;
        case PROP_GDB_PATH:
            g_value_set_string (value, self->gdb_path);
            break;
        case PROP_WORKING_DIR:
            g_value_set_string (value, self->working_dir);
            break;
        case PROP_TARGET_PROGRAM:
            g_value_set_string (value, self->target_program);
            break;
        case PROP_STATE:
            g_value_set_enum (value, self->state);
            break;
        case PROP_TIMEOUT_MS:
            g_value_set_uint (value, self->timeout_ms);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdb_session_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    GdbSession *self = GDB_SESSION (object);

    switch (prop_id)
    {
        case PROP_SESSION_ID:
            g_free (self->session_id);
            self->session_id = g_value_dup_string (value);
            break;
        case PROP_GDB_PATH:
            g_free (self->gdb_path);
            self->gdb_path = g_value_dup_string (value);
            if (self->gdb_path == NULL)
            {
                self->gdb_path = g_strdup (DEFAULT_GDB_PATH);
            }
            break;
        case PROP_WORKING_DIR:
            g_free (self->working_dir);
            self->working_dir = g_value_dup_string (value);
            break;
        case PROP_TIMEOUT_MS:
            self->timeout_ms = g_value_get_uint (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdb_session_class_init (GdbSessionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = gdb_session_dispose;
    object_class->finalize = gdb_session_finalize;
    object_class->get_property = gdb_session_get_property;
    object_class->set_property = gdb_session_set_property;

    /**
     * GdbSession:session-id:
     *
     * Unique identifier for this session.
     */
    properties[PROP_SESSION_ID] =
        g_param_spec_string ("session-id",
                             "Session ID",
                             "Unique identifier for this session",
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    /**
     * GdbSession:gdb-path:
     *
     * Path to the GDB executable.
     */
    properties[PROP_GDB_PATH] =
        g_param_spec_string ("gdb-path",
                             "GDB Path",
                             "Path to the GDB executable",
                             DEFAULT_GDB_PATH,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

    /**
     * GdbSession:working-dir:
     *
     * Working directory for GDB.
     */
    properties[PROP_WORKING_DIR] =
        g_param_spec_string ("working-dir",
                             "Working Directory",
                             "Working directory for GDB",
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

    /**
     * GdbSession:target-program:
     *
     * Currently loaded program.
     */
    properties[PROP_TARGET_PROGRAM] =
        g_param_spec_string ("target-program",
                             "Target Program",
                             "Currently loaded program",
                             NULL,
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    /**
     * GdbSession:state:
     *
     * Current session state.
     */
    properties[PROP_STATE] =
        g_param_spec_enum ("state",
                           "State",
                           "Current session state",
                           GDB_TYPE_SESSION_STATE,
                           GDB_SESSION_STATE_DISCONNECTED,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    /**
     * GdbSession:timeout-ms:
     *
     * Command timeout in milliseconds.
     */
    properties[PROP_TIMEOUT_MS] =
        g_param_spec_uint ("timeout-ms",
                           "Timeout",
                           "Command timeout in milliseconds",
                           0, G_MAXUINT, DEFAULT_TIMEOUT_MS,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    /**
     * GdbSession::state-changed:
     * @session: the #GdbSession
     * @old_state: the previous state
     * @new_state: the new state
     *
     * Emitted when the session state changes.
     */
    signals[SIGNAL_STATE_CHANGED] =
        g_signal_new ("state-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 2,
                      GDB_TYPE_SESSION_STATE,
                      GDB_TYPE_SESSION_STATE);

    /**
     * GdbSession::stopped:
     * @session: the #GdbSession
     * @reason: the stop reason
     * @details: additional details as JsonObject
     *
     * Emitted when the target program stops.
     */
    signals[SIGNAL_STOPPED] =
        g_signal_new ("stopped",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 2,
                      GDB_TYPE_STOP_REASON,
                      JSON_TYPE_OBJECT);

    /**
     * GdbSession::console-output:
     * @session: the #GdbSession
     * @text: the console output text
     *
     * Emitted when GDB outputs console text.
     */
    signals[SIGNAL_CONSOLE_OUTPUT] =
        g_signal_new ("console-output",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 1,
                      G_TYPE_STRING);

    /**
     * GdbSession::ready:
     * @session: the #GdbSession
     *
     * Emitted when the session becomes ready for commands.
     */
    signals[SIGNAL_READY] =
        g_signal_new ("ready",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 0);

    /**
     * GdbSession::terminated:
     * @session: the #GdbSession
     * @exit_code: the exit code (-1 if unknown)
     *
     * Emitted when the GDB process terminates.
     */
    signals[SIGNAL_TERMINATED] =
        g_signal_new ("terminated",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 1,
                      G_TYPE_INT);
}

static void
gdb_session_init (GdbSession *self)
{
    self->state = GDB_SESSION_STATE_DISCONNECTED;
    self->timeout_ms = DEFAULT_TIMEOUT_MS;
    self->mi_parser = gdb_mi_parser_new ();
}

/* ========================================================================== */
/* Public API - Creation                                                      */
/* ========================================================================== */

GdbSession *
gdb_session_new (const gchar *session_id,
                 const gchar *gdb_path,
                 const gchar *working_dir)
{
    return (GdbSession *)g_object_new (GDB_TYPE_SESSION,
                                       "session-id", session_id,
                                       "gdb-path", gdb_path,
                                       "working-dir", working_dir,
                                       NULL);
}

/* ========================================================================== */
/* Public API - Property Accessors                                            */
/* ========================================================================== */

const gchar *
gdb_session_get_session_id (GdbSession *self)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), NULL);
    return self->session_id;
}

const gchar *
gdb_session_get_gdb_path (GdbSession *self)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), NULL);
    return self->gdb_path;
}

const gchar *
gdb_session_get_working_dir (GdbSession *self)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), NULL);
    return self->working_dir;
}

const gchar *
gdb_session_get_target_program (GdbSession *self)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), NULL);
    return self->target_program;
}

void
gdb_session_set_target_program (GdbSession  *self,
                                const gchar *program)
{
    g_return_if_fail (GDB_IS_SESSION (self));

    g_free (self->target_program);
    self->target_program = g_strdup (program);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TARGET_PROGRAM]);
}

GdbSessionState
gdb_session_get_state (GdbSession *self)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), GDB_SESSION_STATE_DISCONNECTED);
    return self->state;
}

gboolean
gdb_session_is_ready (GdbSession *self)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), FALSE);

    return self->state == GDB_SESSION_STATE_READY ||
           self->state == GDB_SESSION_STATE_STOPPED;
}

guint
gdb_session_get_timeout_ms (GdbSession *self)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), DEFAULT_TIMEOUT_MS);
    return self->timeout_ms;
}

void
gdb_session_set_timeout_ms (GdbSession *self,
                            guint       timeout_ms)
{
    g_return_if_fail (GDB_IS_SESSION (self));

    self->timeout_ms = timeout_ms;
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TIMEOUT_MS]);
}

GdbMiParser *
gdb_session_get_mi_parser (GdbSession *self)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), NULL);
    return self->mi_parser;
}

/* ========================================================================== */
/* Utility Functions                                                          */
/* ========================================================================== */

/*
 * add_timeout_to_context:
 * @interval_ms: Timeout interval in milliseconds
 * @callback: Callback function
 * @user_data: User data for callback
 *
 * Adds a timeout to the thread-default GMainContext. This is necessary
 * because g_timeout_add() always uses the global default context, which
 * doesn't work when running in a nested main loop with its own context
 * (e.g., when using gdb_tools_execute_command_sync).
 *
 * The caller should store the returned GSource pointer and call
 * g_source_destroy() followed by g_source_unref() when the timeout
 * is no longer needed. This is necessary because g_source_remove()
 * only works for sources attached to the global default context.
 *
 * Returns: (transfer full): The GSource (caller owns reference)
 */
static GSource *
add_timeout_to_context (guint        interval_ms,
                        GSourceFunc  callback,
                        gpointer     user_data)
{
    GSource *source;
    GMainContext *context;

    source = g_timeout_source_new (interval_ms);
    g_source_set_callback (source, callback, user_data, NULL);

    context = g_main_context_get_thread_default ();
    if (context == NULL)
    {
        context = g_main_context_default ();
    }

    g_source_attach (source, context);

    return source;
}

/*
 * get_post_command_delay_ms:
 *
 * Gets the delay (in milliseconds) to wait after writing a command
 * to GDB before starting to read the response. This gives GDB time
 * to process the command and buffer output.
 *
 * The delay can be configured via the GDB_MCP_POST_COMMAND_DELAY_MS
 * environment variable. If not set, DEFAULT_POST_COMMAND_DELAY_MS is used.
 *
 * Returns: The delay in milliseconds
 */
static guint
get_post_command_delay_ms (void)
{
    const gchar *env_val;
    guint delay;

    env_val = g_getenv ("GDB_MCP_POST_COMMAND_DELAY_MS");
    if (env_val != NULL)
    {
        delay = (guint) g_ascii_strtoull (env_val, NULL, 10);
        if (delay > 0)
        {
            return delay;
        }
    }

    return DEFAULT_POST_COMMAND_DELAY_MS;
}

/* ========================================================================== */
/* Start Implementation                                                       */
/* ========================================================================== */

typedef struct {
    GdbSession *session;
    gboolean    ready_received;
    GSource    *timeout_source;
} StartData;

static void
start_data_free (StartData *data)
{
    /* Destroy timeout source if still pending */
    if (data->timeout_source != NULL)
    {
        g_source_destroy (data->timeout_source);
        g_source_unref (data->timeout_source);
        data->timeout_source = NULL;
    }
    g_clear_object (&data->session);
    g_slice_free (StartData, data);
}

static void
on_startup_line_read (GObject      *source,
                      GAsyncResult *result,
                      gpointer      user_data);

static void
read_next_startup_line (GTask *task)
{
    StartData *data = (StartData *)g_task_get_task_data (task);

    g_data_input_stream_read_line_async (data->session->stdout_reader,
                                         G_PRIORITY_DEFAULT,
                                         g_task_get_cancellable (task),
                                         on_startup_line_read,
                                         task);
}

static void
on_startup_line_read (GObject      *source,
                      GAsyncResult *result,
                      gpointer      user_data)
{
    GTask *task = G_TASK (user_data);
    StartData *data = (StartData *)g_task_get_task_data (task);
    g_autoptr(GError) error = NULL;
    g_autofree gchar *line = NULL;
    gsize length;

    line = g_data_input_stream_read_line_finish (G_DATA_INPUT_STREAM (source),
                                                 result, &length, &error);

    if (error != NULL)
    {
        set_state (data->session, GDB_SESSION_STATE_ERROR);

        /* Cancel the timeout before completing */
        if (data->timeout_source != NULL)
        {
            g_source_destroy (data->timeout_source);
            g_source_unref (data->timeout_source);
            data->timeout_source = NULL;
            g_object_unref (task);  /* Release ref held by timeout callback */
        }

        g_task_return_error (task, g_steal_pointer (&error));
        g_object_unref (task);
        return;
    }

    if (line == NULL)
    {
        /* EOF - process exited */
        set_state (data->session, GDB_SESSION_STATE_ERROR);

        /* Cancel the timeout before completing */
        if (data->timeout_source != NULL)
        {
            g_source_destroy (data->timeout_source);
            g_source_unref (data->timeout_source);
            data->timeout_source = NULL;
            g_object_unref (task);  /* Release ref held by timeout callback */
        }

        g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_SPAWN_FAILED,
                                 "GDB process exited unexpectedly during startup");
        g_object_unref (task);
        return;
    }

    /* Check if GDB is ready - only complete when we see the (gdb) prompt
     * to ensure we've consumed all buffered output from startup.
     * GDB sends ^done first, then (gdb) prompt - we must wait for the prompt
     * to avoid leaving it buffered for the next command to read.
     */
    if (gdb_mi_parser_is_prompt (line))
    {
        data->ready_received = TRUE;
        set_state (data->session, GDB_SESSION_STATE_READY);

        /* Cancel the timeout before completing - must unref the task ref it holds */
        if (data->timeout_source != NULL)
        {
            g_source_destroy (data->timeout_source);
            g_source_unref (data->timeout_source);
            data->timeout_source = NULL;
            g_object_unref (task);  /* Release ref held by timeout callback */
        }

        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    /* Continue reading */
    read_next_startup_line (task);
}

static gboolean
on_start_timeout (gpointer user_data)
{
    GTask *task = G_TASK (user_data);
    StartData *data = (StartData *)g_task_get_task_data (task);

    /* Clear source pointer and unref - the source is being removed by
     * returning G_SOURCE_REMOVE, so start_data_free shouldn't touch it.
     */
    if (data != NULL && data->timeout_source != NULL)
    {
        g_source_unref (data->timeout_source);
        data->timeout_source = NULL;
    }

    g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_TIMEOUT,
                             "GDB startup timed out");
    g_object_unref (task);

    return G_SOURCE_REMOVE;
}

void
gdb_session_start_async (GdbSession          *self,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
    g_autoptr(GTask) task = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(GSubprocessLauncher) launcher = NULL;
    StartData *data;
    const gchar *argv[3];

    g_return_if_fail (GDB_IS_SESSION (self));

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_source_tag (task, gdb_session_start_async);

    /* Check state */
    if (self->state != GDB_SESSION_STATE_DISCONNECTED)
    {
        g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_ALREADY_RUNNING,
                                 "Session already started");
        return;
    }

    set_state (self, GDB_SESSION_STATE_STARTING);

    /* Create subprocess launcher */
    launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE |
                                          G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                          G_SUBPROCESS_FLAGS_STDERR_MERGE);

    if (self->working_dir != NULL)
    {
        g_subprocess_launcher_set_cwd (launcher, self->working_dir);
    }

    /* Build argv */
    argv[0] = self->gdb_path;
    argv[1] = "--interpreter=mi";
    argv[2] = NULL;

    /* Spawn subprocess */
    self->process = g_subprocess_launcher_spawnv (launcher, argv, &error);
    if (self->process == NULL)
    {
        set_state (self, GDB_SESSION_STATE_ERROR);
        g_task_return_error (task, g_steal_pointer (&error));
        return;
    }

    /* Get I/O streams */
    self->stdin_pipe = g_subprocess_get_stdin_pipe (self->process);
    self->stdout_reader = g_data_input_stream_new (
        g_subprocess_get_stdout_pipe (self->process));

    /* Set up task data */
    data = g_slice_new0 (StartData);
    data->session = g_object_ref (self);
    data->ready_received = FALSE;
    data->timeout_source = NULL;
    g_task_set_task_data (task, data, (GDestroyNotify) start_data_free);

    /* Set up timeout - store source so we can cancel it when done */
    data->timeout_source = add_timeout_to_context (self->timeout_ms,
                                                    on_start_timeout,
                                                    g_object_ref (task));

    /* Start reading output */
    read_next_startup_line (g_steal_pointer (&task));
}

gboolean
gdb_session_start_finish (GdbSession    *self,
                          GAsyncResult  *result,
                          GError       **error)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

/* ========================================================================== */
/* Execute Implementation                                                     */
/* ========================================================================== */

typedef struct {
    GdbSession *session;
    GString    *output;
    gboolean    complete;
    gboolean    saw_error;       /* Saw ^error result */
    gchar      *error_message;   /* Error message from ^error */
    gboolean    saw_running;     /* Saw ^running or *running - wait for *stopped */
    gboolean    saw_stopped;     /* Saw *stopped - can complete on next (gdb) */
    GSource    *timeout_source;
} ExecuteData;

static void
execute_data_free (ExecuteData *data)
{
    /* Destroy timeout source if still pending */
    if (data->timeout_source != NULL)
    {
        g_source_destroy (data->timeout_source);
        g_source_unref (data->timeout_source);
        data->timeout_source = NULL;
    }
    g_clear_object (&data->session);
    if (data->output != NULL)
    {
        g_string_free (data->output, TRUE);
    }
    g_free (data->error_message);
    g_slice_free (ExecuteData, data);
}

static void
on_execute_line_read (GObject      *source,
                      GAsyncResult *result,
                      gpointer      user_data);

static void
read_next_execute_line (GTask *task)
{
    ExecuteData *data = (ExecuteData *)g_task_get_task_data (task);

    g_data_input_stream_read_line_async (data->session->stdout_reader,
                                         G_PRIORITY_DEFAULT,
                                         g_task_get_cancellable (task),
                                         on_execute_line_read,
                                         task);
}

static void
on_execute_line_read (GObject      *source,
                      GAsyncResult *result,
                      gpointer      user_data)
{
    GTask *task = G_TASK (user_data);
    ExecuteData *data = (ExecuteData *)g_task_get_task_data (task);
    g_autoptr(GError) error = NULL;
    g_autofree gchar *line = NULL;
    gsize length;

    line = g_data_input_stream_read_line_finish (G_DATA_INPUT_STREAM (source),
                                                 result, &length, &error);

    if (error != NULL)
    {
        /* Cancel the timeout before completing */
        if (data->timeout_source != NULL)
        {
            g_source_destroy (data->timeout_source);
            g_source_unref (data->timeout_source);
            data->timeout_source = NULL;
            g_object_unref (task);  /* Release ref held by timeout callback */
        }

        g_task_return_error (task, g_steal_pointer (&error));
        g_object_unref (task);
        return;
    }

    if (line == NULL)
    {
        /* EOF */
        /* Cancel the timeout before completing */
        if (data->timeout_source != NULL)
        {
            g_source_destroy (data->timeout_source);
            g_source_unref (data->timeout_source);
            data->timeout_source = NULL;
            g_object_unref (task);  /* Release ref held by timeout callback */
        }

        g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_COMMAND_FAILED,
                                 "GDB process exited unexpectedly");
        g_object_unref (task);
        return;
    }

    /* Append to output */
    g_string_append (data->output, line);
    g_string_append_c (data->output, '\n');

    /* Emit console output for stream records */
    if (line[0] == '~')
    {
        g_autofree gchar *content = gdb_mi_parser_unescape_string (line + 1);
        g_signal_emit (data->session, signals[SIGNAL_CONSOLE_OUTPUT], 0, content);
    }

    /* Track error results - we'll report them when the drain timeout fires */
    if (g_str_has_prefix (line, "^error"))
    {
        g_autoptr(GdbMiRecord) record = NULL;
        const gchar *msg;

        record = gdb_mi_parser_parse_line (data->session->mi_parser, line, NULL);
        msg = record ? gdb_mi_record_get_error_message (record) : "Unknown error";

        data->saw_error = TRUE;
        g_free (data->error_message);
        data->error_message = g_strdup (msg ? msg : "GDB command failed");
    }

    /* Track execution state for async commands like run, continue, step, next, finish.
     * These commands return ^running immediately, then *stopped when execution completes.
     * We need to wait for *stopped before completing, otherwise we'll miss output.
     */
    if (g_str_has_prefix (line, "^running") || g_str_has_prefix (line, "*running"))
    {
        data->saw_running = TRUE;
    }

    if (g_str_has_prefix (line, "*stopped"))
    {
        data->saw_stopped = TRUE;
    }

    /* Complete when we see the prompt, but for execution commands (^running),
     * we must wait for *stopped first. This ensures we capture all async output
     * including the stop reason, frame info, etc.
     *
     * For ^exit, GDB is terminating so we complete on that too.
     */
    if (gdb_mi_parser_is_prompt (line) || g_str_has_prefix (line, "^exit"))
    {
        gchar *result_str;

        /* If we saw ^running but haven't seen *stopped yet, keep reading.
         * The program is still executing and we need to wait for it to stop.
         */
        if (data->saw_running && !data->saw_stopped)
        {
            read_next_execute_line (task);
            return;
        }

        data->complete = TRUE;

        /* Cancel the timeout before completing */
        if (data->timeout_source != NULL)
        {
            g_source_destroy (data->timeout_source);
            g_source_unref (data->timeout_source);
            data->timeout_source = NULL;
            g_object_unref (task);  /* Release ref held by timeout callback */
        }

        result_str = g_string_free (data->output, FALSE);
        data->output = NULL;

        /* Return error if we saw one, otherwise success */
        if (data->saw_error)
        {
            g_free (result_str);
            g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_COMMAND_FAILED,
                                     "%s", data->error_message);
            g_object_unref (task);
            return;
        }

        g_task_return_pointer (task, result_str, g_free);
        g_object_unref (task);
        return;
    }

    /* Continue reading */
    read_next_execute_line (task);
}

/*
 * on_read_delay_complete:
 * @user_data: the GTask for the execute operation
 *
 * Callback called after the post-command delay has elapsed.
 * This gives GDB time to process the command and buffer output
 * before we start reading the response.
 *
 * Returns: G_SOURCE_REMOVE to remove the timeout source
 */
static gboolean
on_read_delay_complete (gpointer user_data)
{
    GTask *task = G_TASK (user_data);

    /* Start reading response after delay */
    read_next_execute_line (task);

    return G_SOURCE_REMOVE;
}

/*
 * on_command_written:
 * @source: the output stream
 * @result: the async result
 * @user_data: the GTask for the execute operation
 *
 * Callback called after the command has been written to GDB's stdin.
 * Instead of immediately reading the response, we add a configurable
 * delay to give GDB time to process the command and buffer output.
 * This fixes issues where output from one command would appear in
 * the next command's response.
 */
static void
on_command_written (GObject      *source,
                    GAsyncResult *result,
                    gpointer      user_data)
{
    GTask *task = G_TASK (user_data);
    ExecuteData *data = (ExecuteData *)g_task_get_task_data (task);
    g_autoptr(GError) error = NULL;
    GSource *delay_source;

    if (!g_output_stream_write_all_finish (G_OUTPUT_STREAM (source), result, NULL, &error))
    {
        /* Cancel the timeout before completing */
        if (data != NULL && data->timeout_source != NULL)
        {
            g_source_destroy (data->timeout_source);
            g_source_unref (data->timeout_source);
            data->timeout_source = NULL;
            g_object_unref (task);  /* Release ref held by timeout callback */
        }

        g_task_return_error (task, g_steal_pointer (&error));
        g_object_unref (task);
        return;
    }

    /* Add delay before reading to let GDB process and buffer output.
     * We don't need to track this source since it's short-lived and
     * just triggers the read operation.
     */
    delay_source = add_timeout_to_context (get_post_command_delay_ms (),
                                            on_read_delay_complete,
                                            task);
    g_source_unref (delay_source);  /* Let the source own itself */
}

static gboolean
on_execute_timeout (gpointer user_data)
{
    GTask *task = G_TASK (user_data);
    ExecuteData *data = (ExecuteData *)g_task_get_task_data (task);

    /* Clear source pointer and unref - the source is being removed by
     * returning G_SOURCE_REMOVE, so execute_data_free shouldn't touch it.
     */
    if (data != NULL && data->timeout_source != NULL)
    {
        g_source_unref (data->timeout_source);
        data->timeout_source = NULL;
    }

    g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_TIMEOUT,
                             "GDB command timed out");
    g_object_unref (task);

    return G_SOURCE_REMOVE;
}

void
gdb_session_execute_async (GdbSession          *self,
                           const gchar         *command,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
    g_autoptr(GTask) task = NULL;
    ExecuteData *data;
    g_autofree gchar *cmd_with_nl = NULL;

    g_return_if_fail (GDB_IS_SESSION (self));
    g_return_if_fail (command != NULL);

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_source_tag (task, gdb_session_execute_async);

    /* Check state */
    if (!gdb_session_is_ready (self))
    {
        g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_SESSION_NOT_READY,
                                 "Session not ready for commands");
        return;
    }

    /* Set up task data */
    data = g_slice_new0 (ExecuteData);
    data->session = g_object_ref (self);
    data->output = g_string_new (NULL);
    data->complete = FALSE;
    data->timeout_source = NULL;
    g_task_set_task_data (task, data, (GDestroyNotify) execute_data_free);

    /* Set up timeout - store source so we can cancel it when done */
    data->timeout_source = add_timeout_to_context (self->timeout_ms,
                                                    on_execute_timeout,
                                                    g_object_ref (task));

    /* Send command */
    cmd_with_nl = g_strdup_printf ("%s\n", command);
    g_output_stream_write_all_async (self->stdin_pipe,
                                     cmd_with_nl,
                                     strlen (cmd_with_nl),
                                     G_PRIORITY_DEFAULT,
                                     cancellable,
                                     on_command_written,
                                     g_steal_pointer (&task));
}

gchar *
gdb_session_execute_finish (GdbSession    *self,
                            GAsyncResult  *result,
                            GError       **error)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), NULL);
    g_return_val_if_fail (g_task_is_valid (result, self), NULL);

    return (gchar *)g_task_propagate_pointer (G_TASK (result), error);
}

/* ========================================================================== */
/* Execute MI Implementation                                                  */
/* ========================================================================== */

typedef struct {
    GdbSession *session;
    GList      *records;
    GString    *current_line;
} ExecuteMiData;

static void
execute_mi_data_free (ExecuteMiData *data)
{
    g_clear_object (&data->session);
    g_list_free_full (data->records, (GDestroyNotify) gdb_mi_record_unref);
    if (data->current_line != NULL)
    {
        g_string_free (data->current_line, TRUE);
    }
    g_slice_free (ExecuteMiData, data);
}

static void
on_execute_mi_line_read (GObject      *source,
                         GAsyncResult *result,
                         gpointer      user_data);

static void
read_next_mi_line (GTask *task)
{
    ExecuteMiData *data = (ExecuteMiData *)g_task_get_task_data (task);

    g_data_input_stream_read_line_async (data->session->stdout_reader,
                                         G_PRIORITY_DEFAULT,
                                         g_task_get_cancellable (task),
                                         on_execute_mi_line_read,
                                         task);
}

static void
on_execute_mi_line_read (GObject      *source,
                         GAsyncResult *result,
                         gpointer      user_data)
{
    GTask *task = G_TASK (user_data);
    ExecuteMiData *data = (ExecuteMiData *)g_task_get_task_data (task);
    g_autoptr(GError) error = NULL;
    g_autoptr(GdbMiRecord) record = NULL;
    g_autofree gchar *line = NULL;
    gsize length;

    line = g_data_input_stream_read_line_finish (G_DATA_INPUT_STREAM (source),
                                                 result, &length, &error);

    if (error != NULL)
    {
        g_task_return_error (task, g_steal_pointer (&error));
        g_object_unref (task);
        return;
    }

    if (line == NULL)
    {
        g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_COMMAND_FAILED,
                                 "GDB process exited unexpectedly");
        g_object_unref (task);
        return;
    }

    /* Parse the line */
    record = gdb_mi_parser_parse_line (data->session->mi_parser, line, &error);
    if (record != NULL)
    {
        data->records = g_list_append (data->records, gdb_mi_record_ref (record));
    }

    /* Check for completion */
    if (gdb_mi_parser_is_prompt (line) ||
        (record && gdb_mi_record_get_type_enum (record) == GDB_MI_RECORD_RESULT))
    {
        GList *result_list;

        result_list = data->records;
        data->records = NULL;

        g_task_return_pointer (task, result_list,
                               (GDestroyNotify) (void (*)(GList *)) g_list_free);
        g_object_unref (task);
        return;
    }

    /* Continue reading */
    read_next_mi_line (task);
}

static void
on_mi_command_written (GObject      *source,
                       GAsyncResult *result,
                       gpointer      user_data)
{
    GTask *task = G_TASK (user_data);
    g_autoptr(GError) error = NULL;

    if (!g_output_stream_write_all_finish (G_OUTPUT_STREAM (source), result, NULL, &error))
    {
        g_task_return_error (task, g_steal_pointer (&error));
        g_object_unref (task);
        return;
    }

    read_next_mi_line (task);
}

void
gdb_session_execute_mi_async (GdbSession          *self,
                              const gchar         *command,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
    g_autoptr(GTask) task = NULL;
    ExecuteMiData *data;
    g_autofree gchar *cmd_with_nl = NULL;

    g_return_if_fail (GDB_IS_SESSION (self));
    g_return_if_fail (command != NULL);

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_source_tag (task, gdb_session_execute_mi_async);

    if (!gdb_session_is_ready (self))
    {
        g_task_return_new_error (task, GDB_ERROR, GDB_ERROR_SESSION_NOT_READY,
                                 "Session not ready for commands");
        return;
    }

    data = g_slice_new0 (ExecuteMiData);
    data->session = g_object_ref (self);
    data->records = NULL;
    g_task_set_task_data (task, data, (GDestroyNotify) execute_mi_data_free);

    /* Note: execute_mi doesn't track the timeout source for cancellation.
     * This is a simplification - the MI parser is typically fast and timeouts
     * are rare. The source will be cleaned up when it fires or context dies.
     */
    {
        GSource *timeout_src = add_timeout_to_context (self->timeout_ms,
                                                        on_execute_timeout,
                                                        g_object_ref (task));
        g_source_unref (timeout_src);
    }

    cmd_with_nl = g_strdup_printf ("%s\n", command);
    g_output_stream_write_all_async (self->stdin_pipe,
                                     cmd_with_nl,
                                     strlen (cmd_with_nl),
                                     G_PRIORITY_DEFAULT,
                                     cancellable,
                                     on_mi_command_written,
                                     g_steal_pointer (&task));
}

GList *
gdb_session_execute_mi_finish (GdbSession    *self,
                               GAsyncResult  *result,
                               GError       **error)
{
    g_return_val_if_fail (GDB_IS_SESSION (self), NULL);
    g_return_val_if_fail (g_task_is_valid (result, self), NULL);

    return (GList *)g_task_propagate_pointer (G_TASK (result), error);
}

/* ========================================================================== */
/* Terminate Implementation                                                   */
/* ========================================================================== */

#define TERMINATE_TIMEOUT_MS 500

/*
 * cleanup_session_resources:
 * @self: the GdbSession
 *
 * Clean up session resources after termination. Called from timeout callback
 * after the process has exited or been force-killed.
 */
static void
cleanup_session_resources (GdbSession *self)
{
    gint exit_status = -1;

    if (self->process != NULL && g_subprocess_get_if_exited (self->process))
    {
        exit_status = g_subprocess_get_exit_status (self->process);
    }

    set_state (self, GDB_SESSION_STATE_TERMINATED);
    g_signal_emit (self, signals[SIGNAL_TERMINATED], 0, exit_status);

    g_clear_object (&self->stdout_reader);
    self->stdin_pipe = NULL; /* Owned by subprocess */
    g_clear_object (&self->process);
}

/*
 * on_terminate_timeout:
 * @user_data: the GdbSession (with extra ref held)
 *
 * Timeout callback to check if GDB process has exited and force kill if not.
 * This runs after sending the quit command, giving GDB time to exit gracefully.
 * We hold an extra reference to the session during the timeout to prevent
 * use-after-free if the session is removed from the manager before timeout fires.
 *
 * Returns: G_SOURCE_REMOVE to run only once
 */
static gboolean
on_terminate_timeout (gpointer user_data)
{
    GdbSession *self = GDB_SESSION (user_data);

    if (self->process == NULL)
    {
        /* Already cleaned up somehow */
        g_object_unref (self);
        return G_SOURCE_REMOVE;
    }

    /* Check if process exited gracefully */
    if (!g_subprocess_get_if_exited (self->process))
    {
        /* Still running - force kill it */
        g_subprocess_force_exit (self->process);
    }

    /* Clean up resources */
    cleanup_session_resources (self);

    /* Release the reference we held during the timeout */
    g_object_unref (self);

    return G_SOURCE_REMOVE;
}

void
gdb_session_terminate (GdbSession *self)
{
    g_return_if_fail (GDB_IS_SESSION (self));

    if (self->process == NULL)
    {
        return;
    }

    if (self->state != GDB_SESSION_STATE_TERMINATED &&
        self->state != GDB_SESSION_STATE_DISCONNECTED)
    {
        /* Try graceful shutdown first by sending quit command */
        if (self->stdin_pipe != NULL)
        {
            const gchar *quit_cmd = "quit\n";
            g_output_stream_write_all (self->stdin_pipe, quit_cmd,
                                       strlen (quit_cmd), NULL, NULL, NULL);
        }

        /* Schedule timeout to check/force kill - don't block the main loop.
         * This returns immediately and the cleanup happens asynchronously.
         * We take a reference to prevent use-after-free if the session is
         * removed from the manager before the timeout fires.
         */
        {
            GSource *terminate_src = add_timeout_to_context (TERMINATE_TIMEOUT_MS,
                                                              on_terminate_timeout,
                                                              g_object_ref (self));
            g_source_unref (terminate_src);
        }
    }
    else
    {
        /* Already terminated or disconnected - just clean up */
        cleanup_session_resources (self);
    }
}
