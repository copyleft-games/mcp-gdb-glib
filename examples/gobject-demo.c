/*
 * gobject-demo.c - Example GLib/GObject application for debugging
 *
 * Copyright (C) 2025 Zach Podbielniak
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This example application demonstrates GLib/GObject features that can be
 * debugged using the GDB MCP server. It includes:
 * - A GObject subclass with properties and signals
 * - GList usage for linked lists
 * - GHashTable usage for key-value storage
 * - Multiple functions for stepping and backtrace demos
 * - Clear breakpoint targets
 *
 * Build: make examples
 * Debug: Use the GDB MCP server tools
 */

#include <glib.h>
#include <glib-object.h>
#include <stdio.h>

/* ========================================================================== */
/* DemoObject Type Declaration                                                 */
/* ========================================================================== */

#define DEMO_TYPE_OBJECT (demo_object_get_type ())
G_DECLARE_FINAL_TYPE (DemoObject, demo_object, DEMO, OBJECT, GObject)

struct _DemoObject
{
    GObject     parent_instance;

    gchar      *name;
    gint        counter;
    GList      *items;
    GHashTable *properties;
};

/* Signal IDs */
enum {
    SIGNAL_COUNTER_CHANGED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

/* Property IDs */
enum {
    PROP_0,
    PROP_NAME,
    PROP_COUNTER,
    N_PROPS
};

static GParamSpec *obj_properties[N_PROPS];

G_DEFINE_TYPE (DemoObject, demo_object, G_TYPE_OBJECT)

/* ========================================================================== */
/* Helper Functions (for backtrace demonstration)                              */
/* ========================================================================== */

/*
 * compute_value:
 * @base: Base value
 * @multiplier: Multiplier
 *
 * A helper function that creates nested call frames for backtrace demos.
 * Set a breakpoint here to see a deeper call stack.
 *
 * Returns: The computed value (base * multiplier + 10)
 */
static gint
compute_value (gint base,
               gint multiplier)
{
    gint result;

    /* Good breakpoint location - local variables visible */
    result = base * multiplier;
    result = result + 10;

    return result;
}

/*
 * process_item:
 * @item: The item string to process
 * @user_data: User data (counter pointer)
 *
 * Callback for g_list_foreach - processes each item.
 */
static void
process_item (gpointer item,
              gpointer user_data)
{
    const gchar *str = (const gchar *)item;
    gint *count = (gint *)user_data;

    g_print ("  Processing item: %s\n", str);
    (*count)++;
}

/* ========================================================================== */
/* GObject Implementation                                                      */
/* ========================================================================== */

static void
demo_object_dispose (GObject *object)
{
    DemoObject *self = DEMO_OBJECT (object);

    g_clear_pointer (&self->items, g_list_free);
    g_clear_pointer (&self->properties, g_hash_table_unref);

    G_OBJECT_CLASS (demo_object_parent_class)->dispose (object);
}

static void
demo_object_finalize (GObject *object)
{
    DemoObject *self = DEMO_OBJECT (object);

    g_clear_pointer (&self->name, g_free);

    G_OBJECT_CLASS (demo_object_parent_class)->finalize (object);
}

static void
demo_object_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    DemoObject *self = DEMO_OBJECT (object);

    switch (prop_id)
    {
        case PROP_NAME:
            g_value_set_string (value, self->name);
            break;
        case PROP_COUNTER:
            g_value_set_int (value, self->counter);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
demo_object_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    DemoObject *self = DEMO_OBJECT (object);

    switch (prop_id)
    {
        case PROP_NAME:
            g_free (self->name);
            self->name = g_value_dup_string (value);
            break;
        case PROP_COUNTER:
            self->counter = g_value_get_int (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
demo_object_class_init (DemoObjectClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = demo_object_dispose;
    object_class->finalize = demo_object_finalize;
    object_class->get_property = demo_object_get_property;
    object_class->set_property = demo_object_set_property;

    /**
     * DemoObject:name:
     *
     * The name of this demo object.
     */
    obj_properties[PROP_NAME] =
        g_param_spec_string ("name",
                             "Name",
                             "The object name",
                             "unnamed",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

    /**
     * DemoObject:counter:
     *
     * A counter value that can be incremented.
     */
    obj_properties[PROP_COUNTER] =
        g_param_spec_int ("counter",
                          "Counter",
                          "A counter value",
                          0, G_MAXINT, 0,
                          G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, obj_properties);

    /**
     * DemoObject::counter-changed:
     * @object: The #DemoObject
     * @old_value: The previous counter value
     * @new_value: The new counter value
     *
     * Emitted when the counter value changes.
     */
    signals[SIGNAL_COUNTER_CHANGED] =
        g_signal_new ("counter-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL,
                      G_TYPE_NONE, 2,
                      G_TYPE_INT, G_TYPE_INT);
}

static void
demo_object_init (DemoObject *self)
{
    self->name = g_strdup ("unnamed");
    self->counter = 0;
    self->items = NULL;
    self->properties = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, g_free);
}

/* ========================================================================== */
/* Public API                                                                  */
/* ========================================================================== */

/**
 * demo_object_new:
 * @name: The name for the new object
 *
 * Creates a new #DemoObject with the given name.
 * This is a good target for a breakpoint to inspect object creation.
 *
 * Returns: (transfer full): A new #DemoObject
 */
DemoObject *
demo_object_new (const gchar *name)
{
    DemoObject *obj;

    obj = g_object_new (DEMO_TYPE_OBJECT,
                        "name", name,
                        NULL);

    g_print ("Created DemoObject: %s (ptr=%p)\n", name, (void *)obj);

    return obj;
}

/**
 * demo_object_increment:
 * @self: A #DemoObject
 *
 * Increments the counter and emits the counter-changed signal.
 * This is an excellent breakpoint target to:
 * - Inspect 'self' with gdb_glib_print_gobject
 * - View the items list with gdb_glib_print_glist
 * - Check properties hash with gdb_glib_print_ghash
 * - Step through and watch counter change
 */
void
demo_object_increment (DemoObject *self)
{
    gint old_value;
    gint new_value;
    gint computed;

    g_return_if_fail (DEMO_IS_OBJECT (self));

    old_value = self->counter;

    /* Call nested function for backtrace demo */
    computed = compute_value (old_value, 2);
    new_value = computed - 10 + 1;  /* Simplifies to old_value * 2 + 1 */

    self->counter = new_value;

    g_print ("Counter: %d -> %d\n", old_value, new_value);

    g_signal_emit (self, signals[SIGNAL_COUNTER_CHANGED], 0,
                   old_value, new_value);
}

/**
 * demo_object_add_item:
 * @self: A #DemoObject
 * @item: The item string to add
 *
 * Adds an item to the internal GList.
 * Use gdb_glib_print_glist to inspect the list contents.
 */
void
demo_object_add_item (DemoObject  *self,
                      const gchar *item)
{
    g_return_if_fail (DEMO_IS_OBJECT (self));
    g_return_if_fail (item != NULL);

    self->items = g_list_append (self->items, g_strdup (item));
    g_print ("Added item: %s (list length: %u)\n",
             item, g_list_length (self->items));
}

/**
 * demo_object_set_property_value:
 * @self: A #DemoObject
 * @key: Property key
 * @value: Property value
 *
 * Sets a property in the internal GHashTable.
 * Use gdb_glib_print_ghash to inspect the hash table.
 */
void
demo_object_set_property_value (DemoObject  *self,
                                const gchar *key,
                                const gchar *value)
{
    g_return_if_fail (DEMO_IS_OBJECT (self));
    g_return_if_fail (key != NULL);

    g_hash_table_insert (self->properties,
                         g_strdup (key),
                         g_strdup (value));
    g_print ("Set property: %s = %s\n", key, value);
}

/**
 * demo_object_process:
 * @self: A #DemoObject
 *
 * Processes all items in the list.
 * This function uses g_list_foreach and is good for stepping practice.
 */
void
demo_object_process (DemoObject *self)
{
    gint processed_count = 0;

    g_return_if_fail (DEMO_IS_OBJECT (self));

    g_print ("Processing %u items...\n", g_list_length (self->items));

    /* Iterate through list - good for stepping */
    g_list_foreach (self->items, process_item, &processed_count);

    g_print ("Processed %d items\n", processed_count);
}

/**
 * demo_object_get_summary:
 * @self: A #DemoObject
 *
 * Returns a summary string. Good for gdb_print testing.
 *
 * Returns: (transfer full): A summary string
 */
gchar *
demo_object_get_summary (DemoObject *self)
{
    g_return_val_if_fail (DEMO_IS_OBJECT (self), NULL);

    return g_strdup_printf ("DemoObject[name=%s, counter=%d, items=%u, props=%u]",
                            self->name,
                            self->counter,
                            g_list_length (self->items),
                            g_hash_table_size (self->properties));
}

/* ========================================================================== */
/* Main - Entry Point                                                          */
/* ========================================================================== */

/*
 * main:
 *
 * Entry point for the demo application.
 * Set a breakpoint on main to start debugging.
 *
 * The workflow demonstrates:
 * 1. Object creation
 * 2. List manipulation
 * 3. Hash table usage
 * 4. Counter increments with signals
 * 5. Processing and cleanup
 */
int
main (int   argc,
      char *argv[])
{
    DemoObject *demo;
    gchar *summary;
    gint i;

    g_print ("=== GObject Demo Application ===\n");
    g_print ("This program is designed to be debugged with GDB MCP server.\n\n");

    /* Step 1: Create a DemoObject */
    g_print ("Step 1: Creating DemoObject...\n");
    demo = demo_object_new ("MyDemo");

    /* Step 2: Add items to the GList */
    g_print ("\nStep 2: Adding items to list...\n");
    demo_object_add_item (demo, "apple");
    demo_object_add_item (demo, "banana");
    demo_object_add_item (demo, "cherry");
    demo_object_add_item (demo, "date");

    /* Step 3: Set properties in the GHashTable */
    g_print ("\nStep 3: Setting properties...\n");
    demo_object_set_property_value (demo, "color", "blue");
    demo_object_set_property_value (demo, "size", "large");
    demo_object_set_property_value (demo, "priority", "high");

    /* Step 4: Increment counter several times */
    g_print ("\nStep 4: Incrementing counter...\n");
    for (i = 0; i < 3; i++)
    {
        demo_object_increment (demo);
    }

    /* Step 5: Process items */
    g_print ("\nStep 5: Processing items...\n");
    demo_object_process (demo);

    /* Step 6: Get summary */
    g_print ("\nStep 6: Getting summary...\n");
    summary = demo_object_get_summary (demo);
    g_print ("Summary: %s\n", summary);
    g_free (summary);

    /* Step 7: Cleanup */
    g_print ("\nStep 7: Cleaning up...\n");
    g_object_unref (demo);

    g_print ("\n=== Demo Complete ===\n");

    return 0;
}
