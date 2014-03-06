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
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#define _POSIX_SOURCE /* feature test macro for signal functions */
#define _XOPEN_SOURCE /* feature test macro for popen */

#include <tilda-config.h>

#include <debug.h>
#include <tilda.h>
#include <callback_func.h>
#include <configsys.h>
#include <tilda_window.h>
#include <key_grabber.h> /* for pull */
#include <wizard.h>
#include <xerror.h>
#include <tomboykeybinder.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <stdio.h>
#include <signal.h>
#include <pwd.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>

#include <vte/vte.h>
#include <glib/gstdio.h>


static gchar *create_lock_file (struct lock_info lock)
{
    DEBUG_FUNCTION ("create_lock_file");
    DEBUG_ASSERT (lock.instance >= 0);
    DEBUG_ASSERT (lock.pid >= 1);

    gint ret;
    gchar *lock_file_full;
    gchar *lock_dir = g_build_filename (g_get_user_cache_dir (), "tilda", "locks", NULL);
    gchar *lock_file = g_strdup_printf ("lock_%d_%d", lock.pid, lock.instance);

    /* Make the ~/.cache/tilda/locks directory */
    ret = g_mkdir_with_parents (lock_dir,  S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret == -1)
        goto mkdir_fail;

    /* Create the full path to the lock file */
    lock_file_full = g_build_filename (lock_dir, lock_file, NULL);

    /* Create the lock file */
    ret = g_creat (lock_file_full, S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret == -1)
        goto creat_fail;

    g_free (lock_file);
    g_free (lock_dir);

    return lock_file_full;

    /* Free memory and return NULL */
creat_fail:
    g_free (lock_file_full);
mkdir_fail:
    g_free (lock_file);
    g_free (lock_dir);

    return NULL;
}

/**
 * Check if a filename corresponds to a valid lockfile. Note that this
 * routine does NOT check whether it is a stale lock, however. This
 * will return the lock file's corresponding pid, if it is a valid lock.
 *
 * @param filename the filename to check
 * @return a new struct lock_info
 *
 * Success: struct lock_info will be filled in and non-NULL
 * Failure: return NULL
 */
static struct lock_info *islockfile (const gchar *filename)
{
    DEBUG_FUNCTION ("islockfile");
    DEBUG_ASSERT (filename != NULL);

    struct lock_info *lock;

    lock = g_malloc (sizeof (struct lock_info));

    if (lock == NULL)
        return NULL;

#ifdef USE_GLIB_2_14
    GRegex *regex;
    GMatchInfo *match_info;
    gboolean matched;
    gchar *temp;

    regex = g_regex_new ("^lock_(\d+)_(\d+)$", 0, 0, NULL);
    matched = g_regex_match (regex, filename, 0, &match_info);

    if (!matched)
    {
        g_free (lock);
        lock = NULL;
        goto nomatch;
    }

    /* Get the lock pid */
    temp = g_match_info_fetch (match_info, 0);
    lock->pid = atoi (temp);
    g_free (temp);

    /* Get the configuration number */
    temp = g_match_info_fetch (match_info, 1);
    lock->instance = atoi (temp);
    g_free (temp);

nomatch:
    if (match_info)
        g_match_info_free (match_info);

    g_regex_unref (regex);

    return matched;

#else /* Glib version is <2.14 */

    gboolean matches = g_str_has_prefix (filename, "lock_");
    gboolean islock = FALSE;
    gchar *pid_s, *instance_s;

    if (matches) /* we are prefixed with "lock_" and are probably a lock */
    {
        pid_s = strstr (filename, "_");

        if (pid_s) /* we have a valid pid */
        {
            /* Advance the pointer past the underscore */
            pid_s++;

            lock->pid = atoi (pid_s);
            instance_s = strstr (pid_s, "_");

            if (lock->pid > 0 && instance_s)
            {
                /* Advance the pointer past the underscore */
                instance_s++;

                /* Extract the instance number and store it */
                lock->instance = atoi (instance_s);

                /* we parsed everything, so yes, we were a lock */
                islock = TRUE;
            }
        }
    }

    if (!islock)
    {
        g_free (lock);
        lock = NULL;
    }

    return lock;
#endif
}

/**
 * Remove stale lock files from the ~/.tilda/locks/ directory.
 *
 * Success: returns 0
 * Failure: returns non-zero
 */
static gint remove_stale_lock_files ()
{
    DEBUG_FUNCTION ("remove_stale_lock_files");

    GSList *pids = NULL;
    FILE *ps_output;
    const gchar ps_command[] = "ps -C tilda -o pid=";
    gchar buf[16]; /* Really shouldn't need more than 6 */

    if ((ps_output = popen (ps_command, "r")) == NULL)
    {
        g_printerr (_("Unable to run command: `%s'\n"), ps_command);
        return -1;
    }

    /* The popen() succeeded, get all of the pids */
    while (fgets (buf, sizeof(buf), ps_output) != NULL)
        pids = g_slist_append (pids, GINT_TO_POINTER (atoi (buf)));

    /* We've read all of the pids, exit */
    pclose (ps_output);

    struct lock_info *lock;
    gchar *lock_dir = g_build_filename (g_get_user_cache_dir (), "tilda", "locks", NULL);
    gchar *remove_file;
    gchar *filename;
    GDir *dir;

    /* Open the lock directory for reading */
    dir = g_dir_open (lock_dir, 0, NULL);

    if (dir == NULL)
    {
        g_printerr (_("Unable to open lock directory: %s\n"), lock_dir);
        g_free (lock_dir);
        return -2;
    }

    /* For each possible lock file, check if it is a lock, and see if
     * it matches one of the running tildas */
    while ((filename = (gchar*) g_dir_read_name (dir)) != NULL)
    {
        lock = islockfile (filename);

        if (lock && (g_slist_find (pids, GINT_TO_POINTER (lock->pid)) == NULL))
        {
            /* We have found a stale element */
            remove_file = g_build_filename (lock_dir, filename, NULL);
            g_remove (remove_file);
            g_free (remove_file);
        }

        if (lock)
            g_free (lock);
    }

    g_dir_close (dir);
    g_free (lock_dir);

    return 0;
}

/**
 * Parse all of the Command-Line Options given to tilda.
 * This can modify argv and argc, and will set values in the config.
 *
 * @param argc argc from main
 * @param argv argv from main
 * @return TRUE if we should show the configuration wizard, FALSE otherwise
 */
static gboolean parse_cli (int argc, char *argv[])
{
    DEBUG_FUNCTION ("parse_cli");
    DEBUG_ASSERT (argc != 0);
    DEBUG_ASSERT (argv != NULL);

    /* Set default values */
    gchar *background_color = config_getstr ("background_color");
    gchar *command = config_getstr ("command");
    gchar *font = config_getstr ("font");
    gchar *image = config_getstr ("image");
    gchar *working_dir = config_getstr ("working_dir");

    gint lines = config_getint ("lines");
    gint transparency = config_getint ("transparency");
    gint x_pos = config_getint ("x_pos");
    gint y_pos = config_getint ("y_pos");

    gboolean antialias = config_getbool ("antialias");
    gboolean scrollbar = config_getbool ("scrollbar");
    gboolean show_config = FALSE;
    gboolean version = FALSE;
    gboolean hidden = config_getbool ("hidden");

    /* All of the various command-line options */
    GOptionEntry cl_opts[] = {
        { "antialias",          'a', 0, G_OPTION_ARG_NONE,      &antialias,         N_("Use Antialiased Fonts"), NULL },
        { "background-color",   'b', 0, G_OPTION_ARG_STRING,    &background_color,  N_("Set the background color"), NULL },
        { "command",            'c', 0, G_OPTION_ARG_STRING,    &command,           N_("Run a command at startup"), NULL },
        { "hidden",             'h', 0, G_OPTION_ARG_NONE,      &hidden,            N_("Start Tilda hidden"), NULL },
        { "font",               'f', 0, G_OPTION_ARG_STRING,    &font,              N_("Set the font to the following string"), NULL },
        { "lines",              'l', 0, G_OPTION_ARG_INT,       &lines,             N_("Scrollback Lines"), NULL },
        { "scrollbar",          's', 0, G_OPTION_ARG_NONE,      &scrollbar,         N_("Use Scrollbar"), NULL },
        { "transparency",       't', 0, G_OPTION_ARG_INT,       &transparency,      N_("Opaqueness: 0-100%"), NULL },
        { "version",            'v', 0, G_OPTION_ARG_NONE,      &version,           N_("Print the version, then exit"), NULL },
        { "working-dir",        'w', 0, G_OPTION_ARG_STRING,    &working_dir,       N_("Set Initial Working Directory"), NULL },
        { "x-pos",              'x', 0, G_OPTION_ARG_INT,       &x_pos,             N_("X Position"), NULL },
        { "y-pos",              'y', 0, G_OPTION_ARG_INT,       &y_pos,             N_("Y Position"), NULL },
        { "image",              'B', 0, G_OPTION_ARG_STRING,    &image,             N_("Set Background Image"), NULL },
        { "config",             'C', 0, G_OPTION_ARG_NONE,      &show_config,       N_("Show Configuration Wizard"), NULL },
        { NULL }
    };


    /* Set up the command-line parser */
    GError *error = NULL;
    GOptionContext *context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, cl_opts, NULL);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
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
    if (version)
    {
        g_print ("%s\n\n", TILDA_VERSION);

        g_print ("Copyright (c) 2012-2013 Sebastian Geiger (lanoxx@gmx.net)\n");
        g_print ("Copyright (c) 2005-2009 Tristan Sloughter (sloutri@iit.edu)\n");
        g_print ("Copyright (c) 2005-2009 Ira W. Snyder (tilda@irasnyder.com)\n\n");

        g_print ("General Information: https://github.com/lanoxx/tilda\n");
        g_print ("Bug Reports: https://github.com/lanoxx/tilda/issues?state=open\n\n");

        g_print ("This program comes with ABSOLUTELY NO WARRANTY.\n");
        g_print ("This is free software, and you are welcome to redistribute it\n");
        g_print ("under certain conditions. See the file COPYING for details.\n");

        exit (EXIT_SUCCESS);
    }

    /* Now set the options in the config, if they changed */
    if (background_color != config_getstr ("background_color"))
        config_setstr ("background_color", background_color);
    if (command != config_getstr ("command"))
    {
        config_setbool ("run_command", TRUE);
        config_setstr ("command", command);
    }
    if (font != config_getstr ("font"))
        config_setstr ("font", font);
    if (image != config_getstr ("image"))
        config_setstr ("image", image);
    if (working_dir != config_getstr ("working_dir"))
        config_setstr ("working_dir", working_dir);

    if (lines != config_getint ("lines"))
        config_setint ("lines", lines);
    if (transparency != config_getint ("transparency"))
    {
        config_setbool ("enable_transparency", transparency);
        config_setint ("transparency", transparency);
    }
    if (x_pos != config_getint ("x_pos"))
        config_setint ("x_pos", x_pos);
    if (y_pos != config_getint ("y_pos"))
        config_setint ("y_pos", y_pos);

    if (antialias != config_getbool ("antialias"))
        config_setbool ("antialias", antialias);
    if (hidden != config_getbool ("hidden"))
        config_setbool ("hidden", hidden);
    if (scrollbar != config_getbool ("scrollbar"))
        config_setbool ("scrollbar", scrollbar);

    /* TRUE if we should show the config wizard, FALSE otherwize */
    return show_config;
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

    return config_file;
}

/**
 * get_instance_number ()
 *
 * Gets the next available tilda instance number. This will always pick the
 * lowest non-running tilda available.
 *
 * Success: return next available instance number (>=0)
 * Failure: return 0
 */
static gint get_instance_number ()
{
    DEBUG_FUNCTION ("get_instance_number");

    gint i;
    gchar *name;

    GDir *dir;
    gint lowest_lock_instance = INT_MAX;
    struct lock_info *lock;
    gchar *lock_dir = g_build_filename (g_get_user_cache_dir (), "tilda", "locks", NULL);

    /* Open the lock directory */
    dir = g_dir_open (lock_dir, 0, NULL);

    /* Check for failure to open */
    if (dir == NULL)
    {
        g_printerr (_("Unable to open lock directory: %s\n"), lock_dir);
        g_free (lock_dir);
        return 0;
    }

    /* Look through every file in the lock directory, and see if it is a lock file.
     * If it is a lock file, check if it's the lowest lock instance (and store it if it is). */
    while ((name = (gchar*)g_dir_read_name (dir)) != NULL)
    {
        lock = islockfile (name);

        if (lock != NULL)
        {
            lowest_lock_instance = min(lock->instance, lowest_lock_instance);
            g_free (lock);
        }
    }

    g_dir_close (dir);
    g_free (lock_dir);

    return lowest_lock_instance;
}

static void termination_handler (G_GNUC_UNUSED gint signum) {
    DEBUG_FUNCTION ("termination_handler");
    gtk_main_quit ();
}

/*
 * This is to do the migration of config files from ~/.tilda to the
 * XDG_*_HOME folders
 */
static void migrate_config_files(char *old_config_path) {
    gchar* old_lock_dir = g_build_filename(old_config_path, "locks", NULL);
    gchar* new_lock_dir = g_build_filename(g_get_user_cache_dir (), "tilda", "locks", NULL);
    gchar* new_config_dir = g_build_filename(g_get_user_config_dir (), "tilda", NULL);

    if(!g_file_test(new_lock_dir, G_FILE_TEST_IS_DIR)) {
           g_mkdir_with_parents(new_lock_dir, 0700);
    }

    //we basically need to move the files from old_config_path to config and cache
    g_rename(old_lock_dir, new_lock_dir);
    //we must move the config files after we have moved the locks directory, otherwise it gets moved aswell
    g_rename(old_config_path, new_config_dir);
    g_free(old_lock_dir);
    g_free(new_lock_dir);
    g_free(new_config_dir);
}

static void load_custom_css_file () {
    GtkCssProvider *provider;
    char* cssfilename = g_build_filename(
        g_get_user_config_dir (), "tilda", "style.css", NULL);
    if (g_file_test (cssfilename, G_FILE_TEST_EXISTS)) {
        g_print (_("Found style.css in the user config directory, "
            "applying user css style.\n"));
        provider = gtk_css_provider_new ();
        gtk_style_context_add_provider_for_screen (
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
        GFile *cssfile = g_file_new_for_path (cssfilename);
        gtk_css_provider_load_from_file (GTK_CSS_PROVIDER (provider),
            cssfile, NULL);
        g_object_unref (cssfile);
    }
}

int main (int argc, char *argv[])
{
    DEBUG_FUNCTION ("main");

    tilda_window *tw = NULL;

    struct sigaction sa;
    struct lock_info lock;
    gboolean need_wizard = FALSE;
    gchar *config_file, *lock_file, *old_config_path;

    /*
     * Migration code to move old files to new XDG folders
     */
    old_config_path = g_build_filename(g_get_home_dir (), ".tilda", NULL);
    if (g_file_test(old_config_path, G_FILE_TEST_IS_DIR)) {
        g_print (_("Migrating old config path to XDG folders\n"));
        migrate_config_files(old_config_path);
        g_free(old_config_path);
    }

    /* Remove stale lock files */
    remove_stale_lock_files ();

    lock.pid = getpid ();
    lock.instance = get_instance_number ();
    config_file = get_config_file_name (lock.instance);
    lock_file = create_lock_file (lock);

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


#ifdef DEBUG
    /* Have to do this early. */
    if (getenv ("VTE_PROFILE_MEMORY"))
        if (atol (getenv ("VTE_PROFILE_MEMORY")) != 0)
            g_mem_set_vtable (glib_mem_profiler_table);
#endif
    /* Start up the configuration system */
    config_init (config_file);

    /* Parse the command line */
    need_wizard = parse_cli (argc, argv);

    /* We're about to startup X, so set the error handler. */
    XSetErrorHandler (xerror_handler);

    /* Initialize GTK and libglade */
    gtk_init (&argc, &argv);

    load_custom_css_file ();

    /* create new tilda_window */
    tw = tilda_window_init (config_file, lock.instance);

    /* Check the allocations above */
    if (tw == NULL)
        goto tw_alloc_failed;

    /* Adding widget title for CSS selection */
    gtk_widget_set_name (GTK_WIDGET(tw->window), "Main");

    /* Initialize and set up the keybinding to toggle tilda's visibility. */
    tomboy_keybinder_init ();

    /* Hook up signal handlers */
    sa.sa_handler = termination_handler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction (SIGINT,  &sa, NULL);
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGABRT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    sigaction (SIGKILL, &sa, NULL);

    /* If the config file doesn't exist open up the wizard */
    if (access (tw->config_file, R_OK) == -1)
    {
        /* We probably need a default key, too ... */
        gchar *default_key = g_strdup_printf ("F%d", tw->instance+1);
        config_setstr ("key", default_key);
        g_free (default_key);

        need_wizard = TRUE;
    }

    /* Show the wizard if we need to.
     *
     * Note that the key will be bound upon exiting the wizard */
    if (need_wizard) {
        g_print ("Starting the wizard to configure tilda options.");
        wizard (tw);
    } else {
        gint ret = tilda_keygrabber_bind (config_getstr ("key"), tw);

        if (!ret)
        {
            /* The key was unbindable, so we need to show the wizard */
            show_invalid_keybinding_dialog (NULL, _("The keybinding you chose for \"Pull Down Terminal\" is invalid. Please choose another."));
            wizard (tw);
        }
    }

    pull (tw, config_getbool ("hidden") ? PULL_UP : PULL_DOWN);

    g_print ("Tilda has started. Press %s to pull down the window.\n",
        config_getstr ("key"));
    /* Whew! We're finally all set up and ready to run GTK ... */
    gtk_main();

    /* Ok, we're at the end of our run. Time to clean up ... */
    tilda_window_free (tw);

tw_alloc_failed:
    config_free (config_file);
    g_remove (lock_file);
    g_free (lock_file);
    g_free (config_file);

    return 0;
}

