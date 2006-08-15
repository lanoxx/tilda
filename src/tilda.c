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

#ifdef HAVE_XFT2
#include <fontconfig/fontconfig.h>
#endif

#include <vte/vte.h>
#include "config.h"
#include "tilda.h"
#include "../tilda-config.h"
#include "callback_func.h"
#include "tilda_window.h"
#include "key_grabber.h"
#include "wizard.h"

/* unneeded right now (not used) */
/* gchar *user, *display; */

static tilda_window *tw;

void pull_no_args()
{
    pull(tw);
}

/**
 * Print a message to stderr, then exit.
 *
 * @param message the message to print
 * @param exitval the value to call exit with
 */
void print_and_exit (gchar *message, gint exitval)
{
    fprintf (stderr, "%s\n", message);
    exit (exitval);
}

/**
 * Get the next instance number to use
 *
 * @param tw the tilda_window structure for this instance
 */
gint getnextinstance (tilda_window *tw)
{
#ifdef DEBUG
    puts("getnextinstance");
#endif

    gchar *lock_dir;    
    gchar *name;
    gchar **tokens;
    gint tmp, current;
    gchar *lock_subdir = "/.tilda/locks";
    gint lock_dir_size = 0;
    gint count = 0;
    GDir *dir;

    /* Figure out the size of the lock_dir variable. Make sure to
     * account for the null-terminator at the end of the string. */
    lock_dir_size = strlen (tw->home_dir) + strlen (lock_subdir) + 1;

    /* Make sure our allocation did not fail */
    if ((lock_dir = (gchar*) malloc (lock_dir_size * sizeof(gchar))) == NULL)
        print_and_exit ("You ran out of memory... exiting!", 1);

    /* Get the lock directory for this user, and open the directory */
    g_snprintf (lock_dir, lock_dir_size, "%s%s", tw->home_dir, lock_subdir);
    dir = g_dir_open (lock_dir, 0, NULL);

    while (dir != NULL && (name = g_dir_read_name (dir))) 
    {
        tokens = g_strsplit (name, "_", 3);
        
        if (tokens != NULL) 
        {
            current = atoi (tokens[2]);        
            g_strfreev (tokens);
             
            if (current - tmp > 1) {
                count = tmp + 1;           
                break;           
            }
            
            count++;        
                    
            tmp = current;        
        }
    }

    /* Free memory that we allocated */
    if (dir != NULL)
        g_dir_close (dir);

    free (lock_dir);

    return count;
}

/**
 * Set the instance number for tw, and create the lock file
 *
 * @param tw the tilda_window to manipulate
 */
void getinstance (tilda_window *tw)
{
#ifdef DEBUG
    puts("getinstance");
#endif

    gchar pid[6];
    gchar instance[6];
    gchar *lock_subdir = "/.tilda/locks";
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
        print_and_exit ("Out of memory, exiting...", 1);

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
gboolean islockfile (gchar *filename, gint *lock_pid)
{
#ifdef DEBUG
    puts("islockfile");
#endif

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
void clean_tmp (tilda_window *tw)
{
#ifdef DEBUG
    puts("clean_tmp");
#endif

    FILE *ptr;
    const gchar *cmd = "ps -C tilda -o pid=";
    gchar buf[16]; /* Really shouldn't need more than 6 */
    gint i;

    gint num_pids = 0;
    gint *running_pids;

    /* Allocate just one pid, for now */
    if ((running_pids = (gint*) malloc (1 * sizeof(gint))) == NULL)
        print_and_exit ("Out of memory, exiting...", 1);

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
                    print_and_exit ("Out of memory, exiting...", 1);
            }
        }

        pclose (ptr);
    }

    gchar *lock_dir;
    gchar *lock_subdir = "/.tilda/locks";
    gchar *remove_file;
    gchar *filename;
    GDir *dir;
    gint pid;
    gint lock_dir_size = 0;
    gint remove_file_size = 0;
    gboolean stale;

    lock_dir_size = strlen (tw->home_dir) + strlen (lock_subdir) + 1;

    if ((lock_dir = (gchar*) malloc (lock_dir_size * sizeof(gchar))) == NULL)
        print_and_exit ("Out of memory, exiting...", 1);

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
                    print_and_exit ("Out of memory, exiting...", 1);

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
void parse_cli (int *argc, char ***argv, tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("parse_cli");
#endif

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

    /* All of the various command-line options */
    GOptionEntry cl_opts[] = {
        { "antialias",          'a', 0, G_OPTION_ARG_NONE,      &antialias,         "Use Antialiased Fonts", NULL },
        { "background-color",   'b', 0, G_OPTION_ARG_STRING,    &background_color,  "Set the background color", NULL },
        { "command",            'c', 0, G_OPTION_ARG_STRING,    &command,           "Run a command at startup", NULL },
        { "font",               'f', 0, G_OPTION_ARG_STRING,    &font,              "Set the font to the following string", NULL },
        { "lines",              'l', 0, G_OPTION_ARG_INT,       &lines,             "Scrollback Lines", NULL },
        { "scrollbar",          's', 0, G_OPTION_ARG_NONE,      &scrollbar,         "Use Scrollbar", NULL },
        { "transparency",       't', 0, G_OPTION_ARG_INT,       &transparency,      "Opaqueness: 0-100%", NULL },
        { "version",            'v', 0, G_OPTION_ARG_NONE,      &version,           "Print the version, then exit", NULL },
        { "working-dir",        'w', 0, G_OPTION_ARG_STRING,    &working_dir,       "Set Initial Working Directory", NULL },
        { "x-pos",              'x', 0, G_OPTION_ARG_INT,       &x_pos,             "X Position", NULL },
        { "y-pos",              'y', 0, G_OPTION_ARG_INT,       &y_pos,             "Y Position", NULL },
        { "image",              'B', 0, G_OPTION_ARG_STRING,    &image,             "Set Background Image", NULL },
        { "config",             'C', 0, G_OPTION_ARG_NONE,      &show_config,       "Show Configuration Wizard", NULL },
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
        printf ("Error parsing command-line options. Try \"tilda --help\"\n");
        printf ("to see all possible options.\n\n");

        printf ("Error message: %s\n", error->message);
        
        exit (1);
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

        exit (0);
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
        cfg_setint (tw->tc, "transparency", transparency);
    if (x_pos != cfg_getint (tw->tc, "x_pos"))
        cfg_setint (tw->tc, "x_pos", x_pos);
    if (y_pos != cfg_getint (tw->tc, "y_pos"))
        cfg_setint (tw->tc, "y_pos", y_pos);

    if (antialias != cfg_getbool (tw->tc, "antialias"))
        cfg_setbool (tw->tc, "antialias", antialias);
    if (scrollbar != cfg_getbool (tw->tc, "scrollbar"))
        cfg_setbool (tw->tc, "scrollbar", scrollbar);

    /* Show the config wizard, if it was requested */
    if (show_config)
        if ((wizard (*argc, *argv, tw, tt)) == 1) { 
          clean_up(tw); 
        }
}

int main (int argc, char **argv)
{
#ifdef DEBUG
    puts("main");
#endif

    tilda_window *tw;
    tilda_term *tt;

    /* create new tilda window and terminal */
    tw = (tilda_window *) malloc (sizeof (tilda_window));
    tt = (tilda_term *) malloc (sizeof (tilda_term));

    /* Check the allocations above, since they are extremely critical,
     * and segfaults suck */
    if (tw == NULL || tt == NULL)
        print_and_exit ("You ran out of memory... exiting", 1);

    /* Get the user's home directory */
    tw->home_dir = strdup(g_get_home_dir ());

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
    signal (SIGUSR1, pull_no_args);

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

