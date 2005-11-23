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
#include "wizard.h"

gchar *user, *display;

/**
 * Get the next instance number to use
 */
gint getnextinstance ()
{
#ifdef DEBUG
    puts("getnextinstance");
#endif

    gchar lock_dir[1024];
    gint count = 0;
    GDir *dir;

    g_snprintf (lock_dir, sizeof(lock_dir), "%s/.tilda/locks", home_dir);
    dir = g_dir_open (lock_dir, 0, NULL);

    //FIXME: check that this name corresponds to a valid lock
    //FIXME: check the last char(s) to find out the correct next instance
    while (g_dir_read_name (dir))
        count++;

    g_dir_close (dir);

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

    /* Make the ~/.tilda/locks directory */
    g_snprintf (tw->lock_file, sizeof(tw->lock_file), "%s/.tilda/locks", home_dir);
    g_mkdir_with_parents (tw->lock_file,  S_IRUSR | S_IWUSR | S_IXUSR);

    /* Get the number of existing locks */
    tw->instance = getnextinstance ();

    /* Set tw->lock_file to the lock name for this instance */
    g_snprintf (tw->lock_file, sizeof(tw->lock_file), "%s/.tilda/locks/lock_%d_%d",
            home_dir, getpid(), tw->instance);

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
 */
void clean_tmp ()
{
#ifdef DEBUG
    puts("clean_tmp");
#endif

    FILE *ptr;
    gchar cmd[1024];
    gchar buf[1024];
    gint i;

    gint num_pids = 0;
    gint running_pids[128];

    /* Get all running tilda pids, and store them in an array */
    g_strlcpy (cmd, "ps -C tilda -o pid=", sizeof(cmd));

    if ((ptr = popen (cmd, "r")) != NULL)
    {
        while (fgets (buf, sizeof(buf), ptr) != NULL)
        {
            i = atoi(buf); /* get the pid */

            if (i > 0 && i < 32768)
            {
                running_pids[num_pids] = i;
                num_pids++;
            }
        }
    }

    gchar lock_dir[1024];
    gchar remove_file[1024];
    gchar *filename;
    GDir *dir;
    gint pid;
    gboolean stale;

    g_snprintf (lock_dir, sizeof(lock_dir), "%s/.tilda/locks", home_dir);
    dir = g_dir_open (lock_dir, 0, NULL);

    /* For each possible lock file, check if it is a lock, and see if
     * it matches one of the running tildas */
    while (filename = (gchar*)g_dir_read_name (dir))
    {
        if (islockfile (filename, &pid))
        {
            stale = TRUE;

            for (i=0; i<num_pids; i++)
                if (running_pids[i] == pid)
                    stale = FALSE;

            if (stale)
            {
                g_snprintf (remove_file, sizeof(remove_file), "%s/%s", lock_dir, filename);
                g_remove (remove_file);
            }
        }
    }

    g_dir_close (dir);
}

void parse_cli (int *argc, char ***argv, tilda_window *tw, tilda_term *tt)
{
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

    /* All of the various command-line options */
    GOptionEntry cl_opts[] = {
        { "antialias",          'a', 0, G_OPTION_ARG_NONE,      &antialias,         "Use Antialiased Fonts", NULL },
        { "background-color",   'b', 0, G_OPTION_ARG_STRING,    &background_color,  "Set the background color", NULL },
        { "command",            'c', 0, G_OPTION_ARG_STRING,    &command,           "Run a command at startup", NULL },
        { "font",               'f', 0, G_OPTION_ARG_STRING,    &font,              "Set the font to the following string", NULL },
        { "lines",              'l', 0, G_OPTION_ARG_INT,       &lines,             "Scrollback Lines", NULL },
        { "scrollbar",          's', 0, G_OPTION_ARG_NONE,      &scrollbar,         "Use Scrollbar", NULL },
        { "transparency",       't', 0, G_OPTION_ARG_INT,       &transparency,      "Opaqueness: 0-100%", NULL },
        { "working-dir",        'w', 0, G_OPTION_ARG_STRING,    &working_dir,       "Set Initial Working Directory", NULL },
        { "x-pos",              'x', 0, G_OPTION_ARG_INT,       &x_pos,             "X Position", NULL },
        { "y-pos",              'y', 0, G_OPTION_ARG_INT,       &y_pos,             "Y Position", NULL },
        { "image",              'B', 0, G_OPTION_ARG_STRING,    &image,             "Set Background Image", NULL },
        { "config",             'C', 0, G_OPTION_ARG_NONE,      &show_config,       "Show Configuration Wizard", NULL },
        { NULL }
    };

    GError *error = NULL;
    GOptionContext *context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, cl_opts, NULL);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_parse (context, argc, argv, &error);

    /* Now set the options in the config, if they changed */
    if (background_color != cfg_getstr (tw->tc, "background_color"))
        cfg_setstr (tw->tc, "background_color", background_color);
    if (command != cfg_getstr (tw->tc, "command"))
        cfg_setstr (tw->tc, "command", background_color);
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

    if (show_config)
        if ((wizard (*argc, *argv, tw, tt)) == 1) { clean_up(tw); }
}

int main (int argc, char **argv)
{
#ifdef DEBUG
    puts("main");
#endif

    tilda_window *tw;
    tilda_term *tt;

    FILE *fp;
    gint  i, j;

    gboolean show_config = FALSE;

    /* Get the user's home directory */
    home_dir = (gchar*)g_get_home_dir ();

    /* Gotta do this first to make sure no lock files are left over */
    clean_tmp ();

    /* create new tilda window and terminal */
    tw = (tilda_window *) malloc (sizeof (tilda_window));
    tt = (tilda_term *) malloc (sizeof (tilda_term));

    /* FIXME BAD COMMENT
     * set the instance number and place a env in the array of envs
     * to be set when the tilda terminal is created */
    getinstance (tw);

    /* Set the config file name */
    g_snprintf (tw->config_file, sizeof(tw->config_file),
            "%s/.tilda/config_%i", home_dir, tw->instance);

    /* Sets up tw->tc.
     * MUST HAVE tw->config_file set by now!!! */
    init_tilda_window_configs (tw);

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

    gdk_threads_enter ();
    gtk_main();
    gdk_threads_leave ();

    g_remove (tw->lock_file);
    //free (tw->tc);
    cfg_free(tw->tc);
    free (tw);

    return 0;
}

