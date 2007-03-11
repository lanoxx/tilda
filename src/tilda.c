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
#include <stdio.h>
#include <signal.h>
#include <pwd.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>

#include <vte/vte.h>
#include <glib/gstdio.h>

/**
 * Print a message to stderr, then exit.
 *
 * @param message the message to print
 * @param exitval the value to call exit with
 */
void print_and_exit (const gchar *message, gint exitval)
{
    DEBUG_FUNCTION ("print_and_exit");
    DEBUG_ASSERT (message != NULL);

    fprintf (stderr, "%s\n", message);
    exit (exitval);
}

/**
 * Exit on OOM condition.
 *
 * Always terminates with EXIT_FAILURE.
 */
void oom_exit ()
{
	DEBUG_FUNCTION ("oom_exit");

	print_and_exit (_("Out of memory, exiting ..."), EXIT_FAILURE);
}

/**
 * Get the next instance number to use
 *
 * @param tw the tilda_window structure for this instance
 */
static gint getnextinstance (tilda_window *tw)
{
    DEBUG_FUNCTION ("getnextinstance");
    DEBUG_ASSERT (tw != NULL);

    GArray *list = NULL;

    gchar *lock_dir;
    gchar *name;
    gchar **tokens;
    const gchar lock_subdir[] = "/.tilda/locks";
    gint lock_dir_size = 0;
    gint count = 0;
    gint current = 0;
    gint i;
    GDir *dir;

    /* We create a new array to store gint values.
     * We don't want it zero-terminated or cleared to 0's. */
    list = g_array_new (FALSE, FALSE, sizeof(gint));

    /* Figure out the size of the lock_dir variable. Make sure to
     * account for the null-terminator at the end of the string. */
    lock_dir_size = strlen (tw->home_dir) + strlen (lock_subdir) + 1;

    /* Make sure our allocation did not fail */
    if ((lock_dir = (gchar*) malloc (lock_dir_size * sizeof(gchar))) == NULL)
        oom_exit ();

    /* Get the lock directory for this user, and open the directory */
    g_snprintf (lock_dir, lock_dir_size, "%s%s", tw->home_dir, lock_subdir);
    dir = g_dir_open (lock_dir, 0, NULL);

    while (dir != NULL && (name = (gchar*)g_dir_read_name (dir)))
    {
        tokens = g_strsplit (name, "_", 3);

        if (tokens != NULL)
        {
            current = atoi (tokens[2]);
            g_strfreev (tokens);

            /* Do a sorted insert into the GArray */
            for (i=0; i<count; i++)
                if (g_array_index (list, gint, i) != i)
                    break;

            g_array_insert_val (list, i, current);
            count++;
        }
    }

    /* Find the lowest non-consecutive available place in the array. This
     * will be this tilda's number. */
    for (i=0; i<count; i++)
        if (g_array_index (list, gint, i) != i)
            break;

    /* Free memory that we allocated */
    if (dir != NULL)
        g_dir_close (dir);

    g_array_free (list, TRUE);
    free (lock_dir);

    return i;
}

/**
 * Set the instance number for tw, and create the lock file
 *
 * @param tw the tilda_window to manipulate
 */
void getinstance (tilda_window *tw)
{
    DEBUG_FUNCTION ("getinstance");
    DEBUG_ASSERT (tw != NULL);

    gchar pid[6];
    gchar instance[6];
    const gchar lock_subdir[] = "/.tilda/locks";
    gint lock_file_size = 0;

    /* Get the number of existing locks */
    tw->instance = getnextinstance (tw);

    /* Set up the temporary variables used for calculating the
     * size of the tw->lock_file string. */
    g_snprintf (pid, sizeof(pid), "%d", getpid());
    g_snprintf (instance, sizeof(instance), "%d", tw->instance);

    /* Calculate tw->lock_file's size, and allocate it */
    lock_file_size = strlen (tw->home_dir) + strlen (lock_subdir)
                   + strlen ("/lock_") + strlen (pid) + strlen ("_")
                   + strlen (instance) + 1;

    if ((tw->lock_file = (gchar*) malloc (lock_file_size * sizeof(gchar))) == NULL)
        oom_exit ();

    /* Make the ~/.tilda/locks directory */
    g_snprintf (tw->lock_file, lock_file_size, "%s%s", tw->home_dir, lock_subdir);
    g_mkdir_with_parents (tw->lock_file,  S_IRUSR | S_IWUSR | S_IXUSR);

    /* Set tw->lock_file to the lock name for this instance */
    g_snprintf (tw->lock_file, lock_file_size, "%s%s/lock_%s_%s",
            tw->home_dir, lock_subdir, pid, instance);

    /* Create the lock file */
    g_creat (tw->lock_file, S_IRUSR | S_IWUSR | S_IXUSR);
}

/**
 * Check if a filename corresponds to a valid lockfile. Note that this
 * routine does NOT check whether it is a stale lock, however. This
 * will return the lock file's corresponding pid, if it is a valid lock.
 *
 * @param filename the filename to check
 * @param lock_pid the pid of the lock file (if it was valid)
 * @return TRUE if this is a lockfile, FALSE otherwise
 */
static gboolean islockfile (const gchar *filename, gint *lock_pid)
{
    DEBUG_FUNCTION ("islockfile");
    DEBUG_ASSERT (filename != NULL);
    DEBUG_ASSERT (lock_pid != NULL);

    gboolean matches = g_str_has_prefix (filename, "lock_");
    gboolean islock = FALSE;
    gchar *pid, *conf_num;
    gint int_pid, int_cnum;

    if (matches) /* we are prefixed with "lock_" and are probably a lock */
    {
        pid = strstr (filename, "_");

        if (pid) /* we have a valid pid */
        {
            int_pid = atoi (pid+1);
            conf_num = strstr (pid+1, "_");

            if (int_pid > 0 && conf_num)
            {
                int_cnum = atoi (conf_num+1);
                *lock_pid = int_pid;
                islock = TRUE; /* we parsed everything, so yes, we were a lock */
            }
        }
    }

    return islock;
}

/**
 * Remove stale locks in the ~/.tilda/locks/ directory.
 *
 * @param tw the tilda_window structure for this instance
 */
static void clean_tmp (tilda_window *tw)
{
    DEBUG_FUNCTION ("clean_tmp");
    DEBUG_ASSERT (tw != NULL);

    FILE *ptr;
    const gchar cmd[] = "ps -C tilda -o pid=";
    gchar buf[16]; /* Really shouldn't need more than 6 */
    gint i;

    gint num_pids = 0;
    gint *running_pids;

    /* Allocate just one pid, for now */
    if ((running_pids = (gint*) malloc (1 * sizeof(gint))) == NULL)
        oom_exit ();

    /* Get all running tilda pids, and store them in an array */
    if ((ptr = popen (cmd, "r")) != NULL)
    {
        while (fgets (buf, sizeof(buf), ptr) != NULL)
        {
            i = atoi(buf); /* get the pid */

            if (i > 0 && i < 32768)
            {
                running_pids[num_pids] = i;
                num_pids++;

                /* Allocate space for the next pid */
                if ((running_pids = (gint*) realloc (running_pids, (num_pids+1) * sizeof(gint))) == NULL)
                    oom_exit ();
            }
        }

        pclose (ptr);
    }

    gchar *lock_dir;
    const gchar lock_subdir[] = "/.tilda/locks";
    gchar *remove_file;
    gchar *filename;
    GDir *dir;
    gint pid;
    gint lock_dir_size = 0;
    gint remove_file_size = 0;
    gboolean stale;

    lock_dir_size = strlen (tw->home_dir) + strlen (lock_subdir) + 1;

    if ((lock_dir = (gchar*) malloc (lock_dir_size * sizeof(gchar))) == NULL)
        oom_exit ();

    g_snprintf (lock_dir, lock_dir_size, "%s%s", tw->home_dir, lock_subdir);
    dir = g_dir_open (lock_dir, 0, NULL);

    /* For each possible lock file, check if it is a lock, and see if
     * it matches one of the running tildas */
    while (dir != NULL && (filename = (gchar*)g_dir_read_name (dir)))
    {
        if (islockfile (filename, &pid))
        {
            stale = TRUE;

            for (i=0; i<num_pids; i++)
                if (running_pids[i] == pid)
                    stale = FALSE;

            if (stale)
            {
                /* Calculate the size of remove_file, but make sure to allocate space
                 * for the null-terminator and the "/" in the g_snprintf() below. */
                remove_file_size = strlen (lock_dir) + strlen (filename) + 2;

                if ((remove_file = (gchar*) malloc (remove_file_size * sizeof(gchar))) == NULL)
                    oom_exit ();

                g_snprintf (remove_file, remove_file_size, "%s/%s", lock_dir, filename);
                g_remove (remove_file);

                /* Free the memory we just allocated, since we don't need it anymore */
                free (remove_file);
            }
        }
    }

    if (dir != NULL)
        g_dir_close (dir);

    free (running_pids);
    free (lock_dir);
}

/**
 * Parse all of the Command-Line Options given to tilda.
 * This can modify argv and argc, and will set values in tw->tc.
 *
 * @param argc argc from main
 * @param argv argv from main
 * @param tw the tilda_window from main. Used for setting values in tw->tc.
 * @param tt the tilda_term, from main. Used for calling the wizard.
 */
static void parse_cli (int *argc, char ***argv, tilda_window *tw, tilda_term *tt)
{
    DEBUG_FUNCTION ("parse_cli");
    DEBUG_ASSERT (argc != NULL);
    DEBUG_ASSERT (argv != NULL);
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tt != NULL);
    DEBUG_ASSERT (tw->tc != NULL);

    /* Set default values */
    gchar *background_color = cfg_getstr (tw->tc, "background_color");
    gchar *command = cfg_getstr (tw->tc, "command");
    gchar *font = cfg_getstr (tw->tc, "font");
    gchar *image = cfg_getstr (tw->tc, "image");
    gchar *working_dir = cfg_getstr (tw->tc, "working_dir");

    gint lines = cfg_getint (tw->tc, "lines");
    gint transparency = cfg_getint (tw->tc, "transparency");
    gint x_pos = cfg_getint (tw->tc, "x_pos");
    gint y_pos = cfg_getint (tw->tc, "y_pos");

    gboolean antialias = cfg_getbool (tw->tc, "antialias");
    gboolean scrollbar = cfg_getbool (tw->tc, "scrollbar");
    gboolean show_config = FALSE;
    gboolean version = FALSE;
    gboolean hidden = cfg_getbool (tw->tc, "hidden");

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


    /* If the config file doesn't exist open up the wizard */
    if (access (tw->config_file, R_OK) == -1)
        show_config = TRUE;

    /* Set up the command-line parser */
    GError *error = NULL;
    GOptionContext *context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, cl_opts, NULL);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_parse (context, argc, argv, &error);
    g_option_context_free (context);

    /* Check for unknown options, and give a nice message if there are some */
    if (error)
    {
        const char *msg = _("Error parsing command-line options. Try \"tilda --help\"\nto see all possible options.\n\nError message: %s\n");
        fprintf (stderr, msg, error->message);

        exit (EXIT_FAILURE);
    }

    /* If we need to show the version, show it then exit normally */
    if (version)
    {
        printf ("%s\n\n", TILDA_VERSION);

        printf ("Copyright (c) 2005,2006 Tristan Sloughter (sloutri@iit.edu)\n");
        printf ("Copyright (c) 2005,2006 Ira W. Snyder (tilda@irasnyder.com)\n\n");

        printf ("This program comes with ABSOLUTELY NO WARRANTY.\n");
        printf ("This is free software, and you are welcome to redistribute it\n");
        printf ("under certain conditions. See the file COPYING for details.\n");

        exit (EXIT_SUCCESS);
    }

    /* Now set the options in the config, if they changed */
    if (background_color != cfg_getstr (tw->tc, "background_color"))
        cfg_setstr (tw->tc, "background_color", background_color);
    if (command != cfg_getstr (tw->tc, "command"))
        cfg_setstr (tw->tc, "command", command);
    if (font != cfg_getstr (tw->tc, "font"))
        cfg_setstr (tw->tc, "font", font);
    if (image != cfg_getstr (tw->tc, "image"))
        cfg_setstr (tw->tc, "image", image);
    if (working_dir != cfg_getstr (tw->tc, "working_dir"))
        cfg_setstr (tw->tc, "working_dir", working_dir);

    if (lines != cfg_getint (tw->tc, "lines"))
        cfg_setint (tw->tc, "lines", lines);
    if (transparency != cfg_getint (tw->tc, "transparency"))
    {
        cfg_setbool (tw->tc, "enable_transparency", transparency);
        cfg_setint (tw->tc, "transparency", transparency);
    }
    if (x_pos != cfg_getint (tw->tc, "x_pos"))
        cfg_setint (tw->tc, "x_pos", x_pos);
    if (y_pos != cfg_getint (tw->tc, "y_pos"))
        cfg_setint (tw->tc, "y_pos", y_pos);

    if (antialias != cfg_getbool (tw->tc, "antialias"))
        cfg_setbool (tw->tc, "antialias", antialias);
    if (hidden != cfg_getbool (tw->tc, "hidden"))
        cfg_setbool (tw->tc, "hidden", hidden);
    if (scrollbar != cfg_getbool (tw->tc, "scrollbar"))
        cfg_setbool (tw->tc, "scrollbar", scrollbar);

    /* Show the config wizard, if it was requested */
    if (show_config)
        if ((wizard (*argc, *argv, tw, tt)) == 1) {
          clean_up(tw);
        }
}

int get_display_dimension (int dimension)
{
    DEBUG_FUNCTION ("get_display_dimension");
    DEBUG_ASSERT (dimension == HEIGHT || dimension == WIDTH);

    Display                 *dpy;
    XRRScreenSize           *sizes;
    XRRScreenConfiguration  *sc;
    Window                  root;
    int                     nsize;
    int                     screen = -1;
    char                    *display_name = NULL;

    static int height = -1;
    static int width  = -1;

/* Turn this on by default, if we need to turn if off for some people, we
 * should probably add a configure option, such as
 * --enable-cache-display-dimensions or something similar. */
#define CACHE_DISPLAY_DIMENSIONS
#ifdef CACHE_DISPLAY_DIMENSIONS
    if (height != -1 && dimension == HEIGHT)
        return height;

    if (width != -1 && dimension == WIDTH)
        return width;
#endif

    dpy = XOpenDisplay (display_name);

    if (dpy == NULL)
    {
        /* We can't just exit, so print a message and don't attempt to
         * do anything since we won't know what to do! */
        DEBUG_ERROR ("Cannot open dislay");
        fprintf (stderr, _("Can't open display %s\n"), XDisplayName(display_name));
        return -1;
    }

    screen = DefaultScreen (dpy);
    root = RootWindow (dpy, screen);
    sc = XRRGetScreenInfo (dpy, root);

    if (sc == NULL)
    {
        /* We can't just exit, so print a message and don't attempt to
         * do anything since we don't know what to do. */
        DEBUG_ERROR ("Cannot get screen info");
        fprintf (stderr, _("Can't get screen info\n"));
    }

    sizes = XRRConfigSizes(sc, &nsize);

    XRRFreeScreenConfigInfo(sc);
    XCloseDisplay (dpy);

    height = sizes->height;
    width  = sizes->width;

    if (dimension == HEIGHT)
        return height;

    if (dimension == WIDTH)
        return width;

    return -1; // bad choice
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

int write_config_file (tilda_window *tw)
{
    DEBUG_FUNCTION ("write_config_file");
    DEBUG_ASSERT (tw!= NULL);
    DEBUG_ASSERT (tw->config_file != NULL);

    FILE *fp;

    /* Check to see if writing is disabled. Leave early if it is. */
    if (tw->config_writing_disabled)
        return 1;

    fp = fopen(tw->config_file, "w");

    if (fp != NULL)
    {
        cfg_print (tw->tc, fp);

        if (fsync (fileno(fp)))
        {
            // Error occurred during sync
            TILDA_PERROR ();
            DEBUG_ERROR ("Unable to sync file");

            fprintf (stderr, _("Unable to sync the config file to disk\n"));
        }

        if (fclose (fp))
        {
            // An error occurred
            TILDA_PERROR ();
            DEBUG_ERROR ("Unable to close config file");
            fprintf (stderr, _("Unable to close the config file\n"));
        }

    }
    else
    {
        TILDA_PERROR ();
        DEBUG_ERROR ("Unable to write config file");
        fprintf (stderr, _("Unable to write the config file to %s\n"), tw->config_file);
    }

    return 0;
}

/*
 * Compares two config versions together.
 *
 * Returns -1 if config1 is older than config2 (UPDATE REQUIRED)
 * Returns  0 if config1 is equal to   config2 (NORMAL USAGE)
 * Returns  1 if config1 is newer than config2 (DISABLE WRITE)
 */
static gboolean compare_config_versions (const gchar *config1, const gchar *config2)
{
    DEBUG_FUNCTION ("compare_config_versions");
    DEBUG_ASSERT (config1 != NULL);
    DEBUG_ASSERT (config2 != NULL);

    /*
     * 1) Split apart both strings using the .'s
     * 2) Compare the major-major version
     * 3) Compare the major version
     * 4) Compare the minor version
     */

    gchar **config1_tokens;
    gchar **config2_tokens;
    gint  config1_version[3];
    gint  config2_version[3];
    gint  i;

    config1_tokens = g_strsplit (config1, ".", 3);
    config2_tokens = g_strsplit (config2, ".", 3);

    for (i=0; i<3; i++)
    {
        config1_version[i] = atoi (config1_tokens[i]);
        config2_version[i] = atoi (config2_tokens[i]);
    }

    g_strfreev (config1_tokens);
    g_strfreev (config2_tokens);

    /* We're done splitting things, so compare now */
    for (i=0; i<3; i++)
    {
        if (config1_version[i] > config2_version[i])
            return CONFIG1_NEWER;

        if (config1_version[i] < config2_version[i])
            return CONFIG1_OLDER;
    }

    return CONFIGS_SAME;
}

/*
 * Compare config file versions and see if we know about a new version.
 *
 * If we have a version newer than what we know about, we need to stop
 * anything from writing to the config file.
 *
 * If we are at the same config version, we're done, so exit early.
 *
 * If the config is older than we are now, update it and then write
 * it to disk.
 */
static void try_to_update_config_file (tilda_window *tw)
{
    DEBUG_FUNCTION ("try_to_update_config_file");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tw->tc != NULL);

    gchar *current_config = cfg_getstr (tw->tc, "tilda_config_version");

    if (compare_config_versions (current_config, PACKAGE_VERSION) == CONFIGS_SAME)
        return; // Same version as ourselves, we're done!

    if (compare_config_versions (current_config, PACKAGE_VERSION) == CONFIG1_NEWER)
    {
        // We have a version newer than ourselves!
        // Disable writing to the config for safety.
        tw->config_writing_disabled = TRUE;
        return; // Out early, since we won't be able to update anyway!
    }

    /* NOTE: Start with the oldest config version first, and work our way up to the
     * NOTE: newest that we support, updating our current version each time.
     *
     * NOTE: You may need to re-read the config each time! Probably not though,
     * NOTE: since you should be updating VALUES not names directly in the config.
     * NOTE: Try to rely on libconfuse to generate the configs :) */

    /* Below is a template for creating new entries in the updater. If you ever
     * change anything between versions, copy this, replacing YOUR_VERSION
     * with the new version that you are making. */
#if 0
    if (compare_config_versions (current_config, YOUR_VERSION) == CONFIG1_OLDER)
    {
        // TODO: Add things here to migrate from whatever we are to YOUR_VERSION
        current_config = YOUR_VERSION;
    }
#endif

    if (compare_config_versions (current_config, "0.09.4") == CONFIG1_OLDER)
    {
        /* Nothing to update here. All we did was add an option, there is no
         * need to rewrite the config file here, since the writer at the end
         * will automatically add the default value of the new option. */
        current_config = "0.09.4";
    }

    /* We've run through all the updates, so set our config file version to the
     * version we're at now, then write out the config file.
     *
     * NOTE: this only happens if we upgraded the config, due to some early-exit
     * logic above.
     */

    cfg_setstr (tw->tc, "tilda_config_version", current_config);
    write_config_file (tw);
}

int main (int argc, char **argv)
{
    DEBUG_FUNCTION ("main");

    tilda_window *tw;
    tilda_term *tt;

    /* create new tilda window and terminal */
    tw = (tilda_window *) malloc (sizeof (tilda_window));
    tt = (tilda_term *) malloc (sizeof (tilda_term));

    /* Check the allocations above, since they are extremely critical,
     * and segfaults suck */
    if (tw == NULL || tt == NULL)
    {
        DEBUG_ERROR ("Out of memory");
        DEBUG_ASSERT (tw != NULL);
        DEBUG_ASSERT (tt != NULL);

        oom_exit ();
    }

#if HAVE_LIBINTL_H
    /* Gettext Initialization */
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
#endif

    /* Get the user's home directory */
    tw->home_dir = g_strdup(g_get_home_dir ());

    /* Gotta do this first to make sure no lock files are left over */
    clean_tmp (tw);

    /* Set: tw->instance, tw->config_file, and parse the config file */
    init_tilda_window_instance (tw);

    /* Check if the config file is old. If it is, update it */
    try_to_update_config_file (tw);

#ifdef DEBUG
    /* Have to do this early. */
    if (getenv ("VTE_PROFILE_MEMORY"))
        if (atol (getenv ("VTE_PROFILE_MEMORY")) != 0)
            g_mem_set_vtable (glib_mem_profiler_table);
#endif

    /* Parse all of the command-line options */
    parse_cli (&argc, &argv, tw, tt);

    /* We're about to startup X, so set the error handler. */
    XSetErrorHandler (xerror_handler);

    if (!g_thread_supported ())
        g_thread_init(NULL);

    gdk_threads_init();

    gtk_init (&argc, &argv);

    init_tilda_window (tw, tt);

    signal (SIGINT, clean_up_no_args);
    signal (SIGQUIT, clean_up_no_args);
    signal (SIGABRT, clean_up_no_args);
    signal (SIGKILL, clean_up_no_args);
    signal (SIGABRT, clean_up_no_args);
    signal (SIGTERM, clean_up_no_args);

    gdk_threads_enter ();
    gtk_main();
    gdk_threads_leave ();

    g_remove (tw->lock_file);
    cfg_free(tw->tc);
    free (tw->home_dir);
    free (tw->lock_file);
    free (tw->config_file);
    free (tw);

    return 0;
}

