#include "tilda-dbus-actions.h"

#include "key_grabber.h"
#include "tilda-dbus.h"

#define TILDA_DBUS_ACTIONS_BUS_NAME "com.github.lanoxx.tilda.Actions"
#define TILDA_DBUS_ACTIONS_OBJECT_PATH "/com/github/lanoxx/tilda/Actions"

static gchar *
tilda_dbus_actions_get_bus_name_for_instance (int instance_id);

static gchar *
tilda_dbus_actions_get_object_path (tilda_window *window);

static gchar *
tilda_dbus_actions_get_object_path_for_instance (int instance_id);

static gboolean
on_handle_toggle (TildaDbusActions *skeleton,
                  GDBusMethodInvocation *invocation,
                  gpointer user_data)
{
    tilda_window *window;

    window = user_data;

    pull (window, PULL_TOGGLE, FALSE);

    tilda_dbus_actions_complete_toggle (skeleton, invocation);

    return GDK_EVENT_STOP;
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar *name,
                  gpointer window)
{
    TildaDbusActions *actions;
    tilda_window *tw;
    GError *error;
    gchar *path;

    tw = window;

    g_debug ("TildaDbusActions: Name acquired: %s", name);

    error = NULL;

    actions = tilda_dbus_actions_skeleton_new ();

    g_signal_connect (actions, "handle-toggle",G_CALLBACK (on_handle_toggle), window);

    path = tilda_dbus_actions_get_object_path (tw);

    g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (actions),
                                      connection,
                                      path,
                                      &error);

    if (error)
    {
        g_error ("Error while exporting object path of DBus interface %s: %s",
                 TILDA_DBUS_ACTIONS_OBJECT_PATH, error->message);
    }

    g_free (path);
}

guint
tilda_dbus_actions_init (tilda_window *window)
{
    guint bus_identifier;
    gchar *name;

    name = tilda_dbus_actions_get_bus_name (window);

    bus_identifier = g_bus_own_name (G_BUS_TYPE_SESSION,
                                     name,
                                     G_BUS_NAME_OWNER_FLAGS_NONE,
                                     NULL,
                                     on_name_acquired,
                                     NULL, window, NULL);

    g_free (name);

    return bus_identifier;
}

void tilda_dbus_actions_toggle (gint instance_id)
{
    GDBusConnection *conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

    GError * error = NULL;

    if (!conn)
    {
        return;
    }

    gchar * name = tilda_dbus_actions_get_bus_name_for_instance (instance_id);
    gchar * path = tilda_dbus_actions_get_object_path_for_instance (instance_id);

    GVariant * result;

    result = g_dbus_connection_call_sync (conn, name, path,
                                          "com.github.lanoxx.tilda.Actions", "Toggle",
                                          NULL, NULL, G_DBUS_CALL_FLAGS_NONE,
                                          -1, NULL, &error);

    if (error != NULL)
    {
        g_printerr ("Failed to toggle visibility for instance %d: %s\n",
                    instance_id, error->message);
        g_error_free (error);
    } else {
        g_variant_unref (result);
    }

    g_free (name);
    g_free (path);

    g_object_unref (conn);
}

gchar *
tilda_dbus_actions_get_bus_name (tilda_window *window)
{
    return tilda_dbus_actions_get_bus_name_for_instance (window->instance);
}

void
tilda_dbus_actions_finish (guint bus_identifier)
{
    g_bus_unown_name (bus_identifier);
}

static gchar *
tilda_dbus_actions_get_bus_name_for_instance (int instance_id) {
    return g_strdup_printf ("%s%d", TILDA_DBUS_ACTIONS_BUS_NAME, instance_id);
}

static gchar *
tilda_dbus_actions_get_object_path (tilda_window *window)
{
    return tilda_dbus_actions_get_object_path_for_instance (window->instance);
}

static gchar *
tilda_dbus_actions_get_object_path_for_instance (int instance_id)
{
    return g_strdup_printf ("%s%d", TILDA_DBUS_ACTIONS_OBJECT_PATH, instance_id);
}
