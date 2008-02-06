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

#include <tilda-config.h>

#include <debug.h>
#include <tilda.h>
#include <callback_func.h>
#include <configsys.h>
#include <tilda_window.h>
#include <key_grabber.h> /* for pull */
#include <wizard.h>
#include <xerror.h>
#include <translation.h>

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
#include <stdio.h>
#include <signal.h>
#include <pwd.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>

#include <vte/vte.h>
#include <glib/gstdio.h>


static gchar *create_lock_file (gchar *home_directory, struct lock_info lock)
{
    DEBUG_FUNCTION ("create_lock_file");
    DEBUG_ASSERT (home_directory != NULL);
    DEBUG_ASSERT (lock.instance >= 0);
    DEBUG_ASSERT (lock.pid >= 1);

    gint ret;
    gchar *lock_file_full;
    gchar *lock_directory = g_build_filename (home_directory, ".tilda", "locks", NULL);
    gchar *lock_file = g_strdup_printf ("lock_%d_%d", lock.pid, lock.instance);

    /* Make the ~/.tilda/locks directory */
    ret = g_mkdir_with_parents (lock_directory,  S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret == -1)
        goto mkdir_fail;

    /* Create the full path to the lock file */
    lock_file_full = g_build_filename (lock_directory, lock_file, NULL);

    /* Create the lock file */
    ret = g_creat (lock_file_full, S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret == -1)
        goto creat_fail;

    g_free (lock_file);
    g_free (lock_directory);

    return lock_file_full;

    /* Free memory and return NULL */
creat_fail:
    g_free (lock_file_full);
mkdir_fail:
    g_free (lock_file);
    g_free (lock_directory);

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
 * @param home_directory the user's home directory
 *
 * Success: returns 0
 * Failure: returns non-zero
 */
static gint remove_stale_lock_files (gchar *home_directory)
{
    DEBUG_FUNCTION ("remove_stale_lock_files");
    DEBUG_ASSERT (home_directory != NULL);

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
    gchar *lock_dir = g_build_filename (home_directory, ".tilda", "locks", NULL);
    gchar *remove_file;
    gchar *filename;
    GDir *dir;
    gint pid;

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
    while (filename = (gchar*) g_dir_read_name (dir))
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
    DEBUG_ASSERT (argc != NULL);
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
        { "antialias",          'a', 0, G_OPTION_ARG_NONE,      &antialias,         _("Use Antialiased Fonts"), NULL },
        { "background-color",   'b', 0, G_OPTION_ARG_STRING,    &background_color,  _("Set the background color"), NULL },
        { "command",            'c', 0, G_OPTION_ARG_STRING,    &command,           _("Run a command at startup"), NULL },
        { "hidden",             'h', 0, G_OPTION_ARG_NONE,      &hidden,            _("Start Tilda hidden"), NULL },
        { "font",               'f', 0, G_OPTION_ARG_STRING,    &font,              _("Set the font to the following string"), NULL },
        { "lines",              'l', 0, G_OPTION_ARG_INT,       &lines,             _("Scrollback Lines"), NULL },
        { "scrollbar",          's', 0, G_OPTION_ARG_NONE,      &scrollbar,         _("Use Scrollbar"), NULL },
        { "transparency",       't', 0, G_OPTION_ARG_INT,       &transparency,      _("Opaqueness: 0-100%"), NULL },
        { "version",            'v', 0, G_OPTION_ARG_NONE,      &version,           _("Print the version, then exit"), NULL },
        { "working-dir",        'w', 0, G_OPTION_ARG_STRING,    &working_dir,       _("Set Initial Working Directory"), NULL },
        { "x-pos",              'x', 0, G_OPTION_ARG_INT,       &x_pos,             _("X Position"), NULL },
        { "y-pos",              'y', 0, G_OPTION_ARG_INT,       &y_pos,             _("Y Position"), NULL },
        { "image",              'B', 0, G_OPTION_ARG_STRING,    &image,             _("Set Background Image"), NULL },
        { "config",             'C', 0, G_OPTION_ARG_NONE,      &show_config,       _("Show Configuration Wizard"), NULL },
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
        const char *msg = _("Error parsing command-line options. Try \"tilda --help\"\nto see all possible options.\n\nError message: %s\n");
        g_printerr (msg, error->message);

        exit (EXIT_FAILURE);
    }

    /* If we need to show the version, show it then exit normally */
    if (version)
    {
        g_print ("%s\n\n", TILDA_VERSION);

        g_print ("Copyright (c) 2005-2008 Tristan Sloughter (sloutri@iit.edu)\n");
        g_print ("Copyright (c) 2005-2008 Ira W. Snyder (tilda@irasnyder.com)\n\n");

        g_print ("General Information: http://launchpad.net/tilda\n");
        g_print ("Bug Reports: http://bugs.launchpad.net/tilda\n\n");

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

/*
 * Finds the coordinate that will center the tilda window in the screen.
 *
 * If you want to center the tilda window on the top or bottom of the screen,
 * pass the screen width into screen_dimension and the tilda window's width
 * into the tilda_dimension variable. The result will be the x coordinate that
 * should be used in order to have the tilda window centered on the screen.
 *
 * Centering based on y coordinate is similar, just use the screen height and
 * tilda window height.
 */
int find_centering_coordinate (const int screen_dimension, const int tilda_dimension)
{
    DEBUG_FUNCTION ("find_centering_coordinate");

    const float screen_center = screen_dimension / 2.0;
    const float tilda_center  = tilda_dimension  / 2.0;

    return screen_center - tilda_center;
}

/**
 * Get a pointer to the config file name for this instance.
 *
 * NOTE: you MUST call free() on the returned value!!!
 *
 * @param tw the tilda_window structure corresponding to this instance
 * @return a pointer to a string representation of the config file's name
 */
static gchar *get_config_file_name (gchar *home_directory, gint instance)
{
    DEBUG_FUNCTION ("get_config_file_name");
    DEBUG_ASSERT (home_directory != NULL);
    DEBUG_ASSERT (instance >= 0);

    gchar *config_file_prefix = g_build_filename (home_directory, ".tilda", "config_", NULL);
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
static gint get_instance_number (gchar *home_directory)
{
    DEBUG_FUNCTION ("get_instance_number");
    DEBUG_ASSERT (home_directory != NULL);

    gint i;
    gchar *name;

    GDir *dir;
    GSList *list = NULL;
    struct lock_info *lock;
    gchar *lock_dir = g_build_filename (home_directory, ".tilda", "locks", NULL);

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
     * If it is a lock file, store it's instance number in the list. */
    while (name = (gchar*)g_dir_read_name (dir))
    {
        lock = islockfile (name);

        if (lock != NULL)
        {
            list = g_slist_append (list, GINT_TO_POINTER (lock->instance));
            g_free (lock);
        }
    }

    g_dir_close (dir);

    /* Find the lowest available instance.
     *
     * This is not the most efficient algorithm ever, but the
     * list should not be too big, so it's ok for now. */
    for (i=0; i<INT_MAX; i++)
        if (!g_slist_find (list, GINT_TO_POINTER (i)))
            break;

    g_free (lock_dir);
    g_slist_free (list);

    return i;
}

static void termination_handler (gint signum)
{
    DEBUG_FUNCTION ("termination_handler");

    gtk_main_quit ();
}

int main (int argc, char *argv[])
{
    DEBUG_FUNCTION ("main");

    tilda_window *tw = NULL;

    struct sigaction sa;
    struct lock_info lock;
    gboolean need_wizard = FALSE;
    gchar *home_dir, *config_file, *lock_file;

    home_dir = g_strdup (g_get_home_dir ());

    /* Remove stale lock files */
    remove_stale_lock_files (home_dir);

    lock.pid = getpid ();
    lock.instance = get_instance_number (home_dir);
    config_file = get_config_file_name (home_dir, lock.instance);
    lock_file = create_lock_file (home_dir, lock);

#if ENABLE_NLS
    /* Gettext Initialization */
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
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

    if (!g_thread_supported ())
        g_thread_init(NULL);

    /* Initialize GTK and libglade */
    gtk_init (&argc, &argv);

    /*
     * Not needed with the new glade:
     * ====================================
     * glade_init ();
     */

    /* create new tilda_window */
    tw = tilda_window_init (config_file, lock.instance);

    /* Check the allocations above */
    if (tw == NULL)
        goto tw_alloc_failed;

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
    if (need_wizard)
        wizard (tw);
    else
    {
        gint ret = tilda_keygrabber_bind (config_getstr ("key"), tw);

        if (!ret)
        {
            /* The key was unbindable, so we need to show the wizard */
            show_invalid_keybinding_dialog (NULL);
            wizard (tw);
        }
    }

    if (config_getbool ("hidden"))
    {
        /* It does not cause graphical glitches to make tilda hidden on start this way.
         * It does make tilda appear much faster on it's first appearance, so I'm leaving
         * it this way, because it has a good benefit, and no apparent drawbacks. */
        gtk_widget_show (GTK_WIDGET(tw->window));
        gtk_widget_hide (GTK_WIDGET(tw->window));
    }
    else
    {
        pull (tw, PULL_DOWN);
    }

    /* Whew! We're finally all set up and ready to run GTK ... */
    gtk_main();

    /* Ok, we're at the end of our run. Time to clean up ... */
    tilda_window_free (tw);

tw_alloc_failed:
    config_free (config_file);
    g_remove (lock_file);
    g_free (lock_file);
    g_free (config_file);
    g_free (home_dir);

    return 0;
}

