#include "tilda-cli-options.h"

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include "debug.h"

static GQuark tilda_error_quark (void);

G_DEFINE_QUARK(tilda-config-error-quark, tilda_error)

typedef enum {
    TILDA_CONFIG_ERROR_BAD_INPUT, // Error for bad config argument
} TildaConfigError;

static gboolean toggle_option_cb (const gchar *option_name,
                                  const gchar *value,
                                  gpointer user_data,
                                  GError **error);

gboolean tilda_cli_options_parse_options (tilda_cli_options *cli_options,
                                          gint argc,
                                          gchar *argv[],
                                          gchar **config_file)
{
    DEBUG_FUNCTION ("parse_cli");
    DEBUG_ASSERT (argc != 0);
    DEBUG_ASSERT (argv != NULL);
    // *config_file must be non-null only if a configuration file path has been parsed
    DEBUG_ASSERT (*config_file == NULL);

    /* All tilda command-line options */
    GOptionEntry cl_opts[] = {
            { "background-color",   'b', 0, G_OPTION_ARG_STRING,    &(cli_options->background_color),  N_("Set the background color"), NULL },
            { "command",            'c', 0, G_OPTION_ARG_STRING,    &(cli_options->command),           N_("Run a command at startup"), NULL },
            { "hidden",             'h', 0, G_OPTION_ARG_NONE,      &(cli_options->hidden),            N_("Start Tilda hidden"), NULL },
            { "font",               'f', 0, G_OPTION_ARG_STRING,    &(cli_options->font),              N_("Set the font to the following string"), NULL },
            { "config-file",        'g', 0, G_OPTION_ARG_STRING,    config_file,                       N_("Configuration file"), NULL },
            { "lines",              'l', 0, G_OPTION_ARG_INT,       &(cli_options->lines),             N_("Scrollback Lines"), NULL },
            { "scrollbar",          's', 0, G_OPTION_ARG_NONE,      &(cli_options->scrollbar),         N_("Use Scrollbar"), NULL },
            { "version",            'v', 0, G_OPTION_ARG_NONE,      &(cli_options->version),           N_("Print the version, then exit"), NULL },
            { "working-dir",        'w', 0, G_OPTION_ARG_STRING,    &(cli_options->working_dir),       N_("Set Initial Working Directory"), NULL },
            { "x-pos",              'x', 0, G_OPTION_ARG_INT,       &(cli_options->x_pos),             N_("X Position"), NULL },
            { "y-pos",              'y', 0, G_OPTION_ARG_INT,       &(cli_options->y_pos),             N_("Y Position"), NULL },
            { "background-alpha",   't', 0, G_OPTION_ARG_INT,       &(cli_options->back_alpha),        N_("Opaqueness: 0-100%"), NULL },
            { "config",             'C', 0, G_OPTION_ARG_NONE,      &(cli_options->show_config),       N_("Show Configuration Wizard"), NULL },
            G_OPTION_ENTRY_NULL
    };

    GOptionEntry dbus_opts[] = {
            { "dbus", 0, 0, G_OPTION_ARG_NONE,
              &(cli_options->enable_dbus), N_("Enable D-Bus interface for this instance"), NULL },
            { "toggle-window", 'T', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
              toggle_option_cb,  N_("Toggle N-th instance Window visibility and exit"), NULL
            },
            G_OPTION_ENTRY_NULL
    };

    /* Set up the command-line parser */
    GError *error = NULL;
    GOptionContext *context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, cl_opts, NULL);

    /* Register a separate group for DBus related options. */
    GOptionGroup *group = g_option_group_new ("dbus", "D-Bus Options:", "Show Tilda D-Bus Options", cli_options, NULL);
    g_option_group_add_entries (group, dbus_opts);
    g_option_context_add_group (context, group);

    /* Register default GTK and GDK related option group. */
    g_option_context_add_group (context, gtk_get_option_group (FALSE));

    g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free (context);

    /* Check for unknown options, and give a nice message if there are some */
    if (error)
    {
        g_printerr (_("Error parsing command-line options. Try \"tilda --help\"\nto see all possible options.\n\nError message: %s\n"),
                    error->message);

        exit (EXIT_FAILURE);
    }

    /* If we need to show the version, show it then exit normally */
    if (cli_options->version)
    {
        g_print ("%s\n\n", PACKAGE_STRING);

        g_print ("Copyright (c) 2012-2020 Sebastian Geiger (lanoxx@gmx.net)\n");
        g_print ("Copyright (c) 2005-2009 Tristan Sloughter (sloutri@iit.edu)\n");
        g_print ("Copyright (c) 2005-2009 Ira W. Snyder (tilda@irasnyder.com)\n\n");

        g_print ("General Information: https://github.com/lanoxx/tilda\n");
        g_print ("Bug Reports: https://github.com/lanoxx/tilda/issues?state=open\n\n");

        g_print ("This program comes with ABSOLUTELY NO WARRANTY.\n");
        g_print ("This is free software, and you are welcome to redistribute it\n");
        g_print ("under certain conditions. See the file COPYING for details.\n");

        exit (EXIT_SUCCESS);
    }

    /* TRUE if we should show the config wizard, FALSE otherwise */
    return cli_options->show_config;
}

tilda_cli_options *tilda_cli_options_new ()
{
    tilda_cli_options *options = g_malloc0(sizeof(tilda_cli_options));
    if (!options)
    {
        g_printerr (_("Error allocating memory for a new tilda_cli_options structure.\n"));
        exit (EXIT_FAILURE);
    }

    // instance id of the windows will be in range from 0 to N,
    // defaults to -1 (unset) if option is not used.
    options->toggle_window = -1;

    return options;
}

/**
 * toggle_option_cb: A GOptionArgFunc which parses the toggle-window option
 * argument. Using this toggle function allows us to assign a default value,
 * if the user just specifies the option without a value. For example, the
 * user may use '-T' instead of '-T 0', in this case we set the default
 * instance_id to 0. Most users will want to use the default unless they
 * are running multiple tilda instances and thus this makes the setup
 * of toggle shortcuts easier.
 */
static gboolean toggle_option_cb (G_GNUC_UNUSED const gchar *option_name,
                                  const gchar *value,
                                  gpointer user_data,
                                  GError **error)
{
    if (!user_data) {
        g_error("Missing user_data pointer in toggle_option_cb function.");
    }

    tilda_cli_options *options = user_data;

    if (!value || !value[0]) {
        options->toggle_window = 0;
        return TRUE;
    }

    char * parseEnd = NULL;

    long instance_id = strtol(value, &parseEnd, 10);

    if (parseEnd != NULL && *parseEnd != '\0') {
        g_set_error(error, tilda_error_quark(), TILDA_CONFIG_ERROR_BAD_INPUT, "Could not parse the toggle argument. The argument must be a valid integer.");
        return FALSE;
    }

    // sanity check for (gint) cast below. In practice, there will
    // only ever be a few instances running, so this value will probably
    // never be higher than 10, but to make the gint cast save, we check
    // against INT_MAX
    if (instance_id > INT_MAX) {
        g_set_error (error, tilda_error_quark(), TILDA_CONFIG_ERROR_BAD_INPUT,
                     "The toggle window option cannot must not be greater than %d, but value was %ld.", INT_MAX, instance_id);
        return FALSE;
    }

    // if the user specified a negative value, then we default to 0.
    if (instance_id < 0) {
        instance_id = 0;
    }

    options->toggle_window = (gint) instance_id;

    return TRUE;
}
