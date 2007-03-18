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
 * This can modify argv and argc, and will set values in the config.
 *
 * @param argc argc from main
 * @param argv argv from main
 * @param tw the tilda_window from main.
 * @param tt the tilda_term, from main.
 */
static void parse_cli (int *argc, char ***argv, tilda_window *tw, tilda_term *tt)
{
    DEBUG_FUNCTION ("parse_cli");
    DEBUG_ASSERT (argc != NULL);
    DEBUG_ASSERT (argv != NULL);
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tt != NULL);

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
    if (background_color != config_getstr ("background_color"))
        config_setstr ("background_color", background_color);
    if (command != config_getstr ("command"))
        config_setstr ("command", command);
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

    /* Show the config wizard, if it was requested */
    if (show_config)
        if (wizard (tw) != 0) {
          clean_up(tw);
        }
}

gint get_physical_height_pixels ()
{
    DEBUG_FUNCTION ("get_physical_height_pixels");

    const GdkScreen *screen = gdk_screen_get_default ();

    return gdk_screen_get_height (screen);
}

gint get_physical_width_pixels ()
{
    DEBUG_FUNCTION ("get_physical_width_pixels");

    const GdkScreen *screen = gdk_screen_get_default ();

    return gdk_screen_get_width (screen);
}

gint get_display_dimension (const gint dimension)
{
    DEBUG_FUNCTION ("get_display_dimension");
    DEBUG_ASSERT (dimension == HEIGHT || dimension == WIDTH);

    if (dimension == HEIGHT)
        return get_physical_height_pixels ();

    if (dimension == WIDTH)
        return get_physical_width_pixels ();

    return -1;
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

#if ENABLE_NLS
    /* Gettext Initialization */
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
#endif

    /* Initialize the connection to the X server for the key grabbing code */
    init_key_grabber ();

    /* Get the user's home directory */
    tw->home_dir = g_strdup(g_get_home_dir ());

    /* Gotta do this first to make sure no lock files are left over */
    clean_tmp (tw);

    /* Set: tw->instance, tw->config_file, and parse the config file */
    init_tilda_window_instance (tw);

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

    /* Initialize GDK / GTK thread support */
    gdk_threads_init();

    /* Initialize GTK and libglade */
    gtk_init (&argc, &argv);
    glade_init ();

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
    config_free (tw->config_file);
    free (tw->home_dir);
    free (tw->lock_file);
    free (tw->config_file);
    free (tw);

    return 0;
}

