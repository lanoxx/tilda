/*
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#define _DEFAULT_SOURCE

/*
 * This message is shown in a modal dialog when tilda starts and there is a problem parsing the configuration file.
 * Such problems can occur for example if the config file contains a key-value pair that is unknown to tilda. This
 * can be the case if the user manually modified the configuration file or if a newer version of tilda was run and
 * which saved some settings that are not known by older versions of tilda.
 */
#define TILDA_CONFIG_PARSE_ERROR \
    "<b>A problem occurred while parsing the config file.</b>\n\n" \
    "This can happen if the tilda config contains a setting that is unknown to tilda. " \
    "Tilda will now start with the default configuration."

/*
 * This message is shown in a modal dialog when tilda starts and there is a any other problem with the configuration
 * file except a parse error.
 */
#define TILDA_CONFIG_OTHER_ERROR \
    "<b>An unexpected error occurred while parsing the config file.</b>\n\n" \
    "The default configuration will be used instead. This error can occur if the configuration file is corrupted" \
    "or otherwise unreadable. Tilda will now start with a default configuration."

#include "tilda.h"

#include "config.h"
#include "configsys.h"
#include "debug.h"
#include "key_grabber.h" /* for pull */
#include "tilda-cli-options.h"
#include "tilda-dbus-actions.h"
#include "tilda-keybinding.h"
#include "tilda-lock-files.h"
#include "tilda_window.h"
#include "tomboykeybinder.h"
#include "wizard.h"

#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vte/vte.h>

static void setup_signal_handlers (void);
static void show_startup_dialog (int config_init_result);

/**
 * Set values in the config from command-line parameters
 *
 * @param cli_options pointer to a struct containing command-line options
 */
static void setup_config_from_cli_options(tilda_cli_options *cli_options)
{
    if (cli_options->background_color != NULL
            && cli_options->background_color != config_getstr ("background_color")) {
        config_setstr ("background_color", cli_options->background_color);

        GdkRGBA col;
        if (gdk_rgba_parse (&col, cli_options->background_color))
        {
            config_setint("back_red", GUINT16_FROM_FLOAT (col.red));
            config_setint("back_green", GUINT16_FROM_FLOAT (col.green));
            config_setint("back_blue", GUINT16_FROM_FLOAT (col.blue));
        }

        g_free(cli_options->background_color);
    }
    if (cli_options->command != NULL
            && cli_options->command != config_getstr ("command"))
    {
        config_setbool ("run_command", TRUE);
        config_setstr ("command", cli_options->command);
        g_free(cli_options->command);
    }
    if (cli_options->font != NULL
            && cli_options->font != config_getstr ("font")) {
        config_setstr ("font", cli_options->font);
        g_free(cli_options->font);
    }

    if (cli_options->back_alpha != 0
            && cli_options->back_alpha != config_getint ("back_alpha"))
    {
        config_setbool ("enable_transparency", ~cli_options->back_alpha & 0xffff);
        config_setint ("back_alpha", cli_options->back_alpha);
    }

    if (cli_options->working_dir != NULL
            && cli_options->working_dir != config_getstr ("working_dir")) {
        config_setstr ("working_dir", cli_options->working_dir);
        g_free(cli_options->working_dir);
    }

    if (cli_options->lines != 0
            && cli_options->lines != config_getint ("lines"))
        config_setint ("lines", cli_options->lines);
    if (cli_options->x_pos != 0
            && cli_options->x_pos != config_getint ("x_pos"))
        config_setint ("x_pos", cli_options->x_pos);
    if (cli_options->y_pos != 0
            && cli_options->y_pos != config_getint ("y_pos"))
        config_setint ("y_pos", cli_options->y_pos);

    if (cli_options->hidden != FALSE
            && cli_options->hidden != config_getbool ("hidden"))
        config_setbool ("hidden", cli_options->hidden);
    if (cli_options->scrollbar != FALSE
            && cli_options->scrollbar != config_getbool ("scrollbar"))
        config_setbool ("scrollbar", cli_options->scrollbar);
}

/**
 * Get a pointer to the config file name for this instance.
 *
 * NOTE: you MUST call free() on the returned value!!!
 *
 * @param tw the tilda_window structure corresponding to this instance
 * @return a pointer to a string representation of the config file's name
 */
static gchar *get_config_file_name (gint instance)
{
    DEBUG_FUNCTION ("get_config_file_name");
    DEBUG_ASSERT (instance >= 0);

    gchar *config_dir = g_build_filename (g_get_user_config_dir (), "tilda", NULL);

    /* Make the ~/.config/tilda directory */
    if (!g_file_test (config_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
        g_print (_("Creating directory:'%s'\n"), config_dir);
        g_mkdir_with_parents (config_dir,  S_IRUSR | S_IWUSR | S_IXUSR);
    }

    gchar *config_file_prefix = g_build_filename (config_dir, "config_", NULL);
    gchar *config_file;

    config_file = g_strdup_printf ("%s%d", config_file_prefix, instance);
    g_free (config_file_prefix);

    /* resolve possible symlink pointing to a different location. */
    gchar * resolved_config_file = realpath (config_file, NULL);

    if (g_strcmp0 (config_file, resolved_config_file) != 0) {
        g_debug ("Config file at '%s' points to '%s'", config_file, resolved_config_file);
    }

    if (resolved_config_file != NULL) {
        g_free (config_file);
        /* resolved_config_file must be freed with free(3) so we duplicate the
         * string to ensure that what ever we return from this function can be
         * freed with g_free(). */
        config_file = g_strdup (resolved_config_file);
        free (resolved_config_file);
    }

    return config_file;
}

static void termination_handler (G_GNUC_UNUSED gint signum) {
    DEBUG_FUNCTION ("termination_handler");
    gtk_main_quit ();
}

static void load_custom_css_file () {
    GtkCssProvider *provider;
    GFile *file;
    GError *error;
    gchar *filename;

    filename = g_build_filename (g_get_user_config_dir (),
                                 "tilda", "style.css", NULL);

    if (!g_file_test (filename, G_FILE_TEST_EXISTS))
        return;

    g_print (_("Found style.css in the user config directory, "
               "applying user css style.\n"));

    provider = gtk_css_provider_new ();

    gtk_style_context_add_provider_for_screen (gdk_screen_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);


    file = g_file_new_for_path (filename);
    error = NULL;
    gtk_css_provider_load_from_file (provider, file, &error);

    if (error) {
        g_print ("Error: %s", error->message);
        g_error_free (error);
    }

    g_object_unref (file);
    g_free (filename);
}

int main (int argc, char *argv[])
{
#ifdef DEBUG
    /**
     * This enables the tilda log domain while we are in debug mode. This
     * is for convenience such that we do not need to start tilda with
     * 'G_MESSAGES_DEBUG=tilda ./tilda' each time we start tilda.
     */

    g_setenv ("G_MESSAGES_DEBUG", "tilda", FALSE);
#endif

    DEBUG_FUNCTION_MESSAGE ("main", "Using libvte version: %i.%i.%i",
                            VTE_MAJOR_VERSION, VTE_MINOR_VERSION, VTE_MICRO_VERSION);

    /* Set supported backend to X11 */
    gdk_set_allowed_backends ("x11");

    tilda_window tw;
    /* NULL set the tw pointers so we can get a clean exit on initialization failure */
    memset(&tw, 0, sizeof(tilda_window));

    struct lock_info lock;
    gboolean need_wizard = FALSE;
    gchar *config_file;

    tilda_lock_files_obtain_instance_lock (&lock);

#if ENABLE_NLS
    /* Gettext Initialization */
    char* locale = setlocale (LC_ALL, "");
    if(!locale) {
        g_warning ("Could not setup locale.");
    }
    bindtextdomain (PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain (PACKAGE);
#endif

    config_file = NULL;

    /* Parse the command line */
    tilda_cli_options *cli_options = tilda_cli_options_new ();
    need_wizard = tilda_cli_options_parse_options (cli_options, argc, argv, &config_file);

    if (config_file) {	  // if there was a config file specified via cli
        if (!g_file_test (config_file, G_FILE_TEST_EXISTS)) {
            g_printerr (_("Specified config file '%s' does not exist. Reverting to default path.\n"),
                    config_file);
            config_file = get_config_file_name (lock.instance);
        }
    } else {    // if not, we look for the defaut config file
        config_file = get_config_file_name (lock.instance);
    }

    /* Initialize GTK. Any code that interacts with GTK (e.g. creating a widget)
     * should come after this call. Gtk initialization should happen before we
     * initialize the config file. */
    gtk_init (&argc, &argv);

    /* Start up the configuration system and load from file */
    gint config_init_result = config_init (config_file);

    /* Set up possible overridden config options */
    setup_config_from_cli_options(cli_options);

    if (config_init_result > 0) {
        show_startup_dialog (config_init_result);
    }

    load_custom_css_file ();

    /* create new tilda_window */
    gboolean success = tilda_window_init (config_file, lock.instance, &tw);

    if(!success) {
        fprintf(stderr, "tilda.c: initialization failed\n");
        goto initialization_failed;
    }

    /* Initialize and set up the keybinding to toggle tilda's visibility. */
    tomboy_keybinder_init ();

    setup_signal_handlers ();

    /* If the config file doesn't exist open up the wizard */
    if (access (tw.config_file, R_OK) == -1)
    {
        /* We probably need a default key, too ... */
        gchar *default_key = g_strdup_printf ("F%d", tw.instance+1);
        config_setstr ("key", default_key);
        g_free (default_key);

        need_wizard = TRUE;
    }

    /* Show the wizard if we need to.
     *
     * Note that the key will be bound upon exiting the wizard */
    if (need_wizard) {
        g_print ("Starting the wizard to configure tilda options.\n");
        wizard (&tw);
    } else {
        gint ret = tilda_keygrabber_bind (config_getstr ("key"), &tw);

        if (!ret)
        {
            /* The key was unbindable, so we need to show the wizard */
            const char *message = _("The keybinding you chose for \"Pull Down Terminal\" is invalid. Please choose another.");

            tilda_keybinding_show_invalid_keybinding_dialog (NULL,
                                                             message);
            wizard (&tw);
        }
    }

    guint bus_identifier = 0;

    if (cli_options->enable_dbus) {

        gchar *bus_name = tilda_dbus_actions_get_bus_name (&tw);

        g_print ("Activating D-Bus interface on bus name: %s\n",
                 bus_name);

        g_free (bus_name);

        bus_identifier = tilda_dbus_actions_init (&tw);
    }

    g_free(cli_options);

    pull (&tw, config_getbool ("hidden") ? PULL_UP : PULL_DOWN, FALSE);

    g_print ("Tilda has started. Press %s to pull down the window.\n",
        config_getstr ("key"));
    /* Whew! We're finally all set up and ready to run GTK ... */
    gtk_main();

    if (bus_identifier != 0) {
        tilda_dbus_actions_finish (bus_identifier);
    }

initialization_failed:
    tilda_window_free(&tw);

    /* Ok, we're at the end of our run. Time to clean up ... */
    config_free (config_file);

    tilda_lock_files_free (&lock);

    close(lock.file_descriptor);

    g_free (config_file);
    return 0;
}

static void
setup_signal_handlers () {

    struct sigaction sa;
    /* Hook up signal handlers */
    sa.sa_handler = termination_handler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction (SIGINT,  &sa, NULL);
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGABRT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    sigaction (SIGKILL, &sa, NULL);
}

static void
show_startup_dialog (int config_init_result)
{
    /* This section shows a modal dialog to notify the user that something has gone wrong when loading the config.
     * Earlier version only used to print a message to stderr, but since tilda is usually not started from a
     * console this message would have been lost and a message dialog is much more user friendly.
     */
    GtkWidget *dialog = NULL;

    if(config_init_result == 1) {
        dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                    TILDA_CONFIG_PARSE_ERROR);
    } else if (config_init_result != 0) {
        dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                    TILDA_CONFIG_OTHER_ERROR);
    }

    if(dialog) {
        g_message("Running Dialog");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}
