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

int main (int argc, char **argv)
{
    tilda_window *tw;
    tilda_term *tt;

    FILE *fp;
    gint  opt;
    gint  i, j;
    GList *args = NULL;
    gint tmp_trans = -1, x_pos_arg = -1, y_pos_arg = -1;
    gchar s_font_arg[64];
    gchar s_background_arg[7];
    gchar s_image_arg[100];

    const gchar *usage = "Usage: %s "
        "[-B image] "
        "[-T] "
        "[-C] "
        "[-b [white][black] ] "
        "[-f font] "
        "[-a]"
        "[-h] "
        "[-s] "
        "[-w directory] "
        "[-c command] "
        "[-t level] "
        "[-x position] "
        "[-y position] "
        "[-l lines]\n\n"
        "-B image : set background image\n"
        "-T : Sorry this no longer does anything\n"
        "-C : bring up tilda configuration wizard\n"
        "-b [white][black] : set the background color either white or black\n"
        "-f font : set the font to the following string, ie \"monospace 11\"\n"
        "-a : use antialias fonts\n"
        "-h : show this message\n"
        "-s : use scrollbar\n"
        "-w directory : switch working directory\n"
        "-c command : run command\n"
        "-t level: set transparent to true and set the level of transparency to level, 0-100\n"
        "-x postion: sets the number of pixels from the top left corner to move tilda over\n"
        "-y postion: sets the number of pixels from the top left corner to move tilda down\n"
        "-l lines : set number of scrollback lines\n";

    s_font_arg[0] = '\0';
    s_image_arg[0] = '\0';
    s_background_arg[0] = '\0';

    /* Get the user's home directory */
    home_dir = (gchar*)g_get_home_dir ();

    /* Gotta do this first to make sure no lock files are left over */
    clean_tmp ();

    /* create new tilda window and terminal */
    tw = (tilda_window *) malloc (sizeof (tilda_window));
    tw->tc = (tilda_conf *) malloc (sizeof (tilda_conf));
    tt = (tilda_term *) malloc (sizeof (tilda_term));

    init_tilda_window_configs (tw);

#ifdef DEBUG
    /* Have to do this early. */
    if (getenv ("VTE_PROFILE_MEMORY"))
        if (atol (getenv ("VTE_PROFILE_MEMORY")) != 0)
            g_mem_set_vtable (glib_mem_profiler_table);
#endif

    /* Pull out long options for GTK+. */
    for (i=j=1;i<argc;i++)
    {
        if (g_ascii_strncasecmp ("--", argv[i], 2) == 0)
        {
            args = g_list_append (args, argv[i]);

            for (j=i;j<argc;j++)
                argv[j] = argv[j + 1];

            argc--;
            i--;
        }
    }

    /* set the instance number and place a env in the array of envs
     * to be set when the tilda terminal is created */
    getinstance (tw);

    g_snprintf (tw->config_file, sizeof(tw->config_file),
            "%s/.tilda/config_%i", home_dir, tw->instance);

    /* check for -T argument, if there is one just write to the pipe and exit,
     * this will bring down or move up the term */
    while ((opt = getopt(argc, argv, "x:y:B:CDTab:c:df:ghkn:st:w:l:-")) != -1)
    {
        gboolean bail = FALSE;
        switch (opt)
        {
            case 'B':
                image_set_clo = TRUE;
                g_strlcpy (s_image_arg, optarg, sizeof (s_image_arg));
                break;
            case 'b':
                g_strlcpy (s_background_arg, optarg, sizeof (s_background_arg));
                break;
            case 'T':
                printf ("-T no longer does anything :(, tilda no longer uses xbindkeys\n");
                printf ("If there is a demand I can fix it up so both new and old work.\n");
                printf ("I see this as extra overhead for no reason however, sorry.\n");
                break;
            case 'C':
                if ((wizard (argc, argv, tw, tt)) == 1) { clean_up(tw); }
                break;
            case 's':
                scroll_set_clo = TRUE;
                break;
            case 'c':
                command = optarg;
                break;
            case 't':
                tmp_trans= atoi (optarg);
                break;
            case 'f':
                g_strlcpy (s_font_arg, optarg, sizeof (s_font_arg));
                break;
            case 'w':
                working_directory = optarg;
                break;
            case 'a':
                antialias_set_clo = TRUE;
                break;
            case 'h':
                g_print(usage, argv[0]);
                exit(1);
                break;
            case 'l':
                tw->tc->lines = atoi (optarg);

                if (tw->tc->lines <= 0)
                    tw->tc->lines = DEFAULT_LINES;

                break;
            case '-':
                bail = TRUE;
                break;
            case 'x':
                x_pos_arg = atoi (optarg);
                break;
            case 'y':
                y_pos_arg = atoi (optarg);
                break;
            default:
                break;
        }

        if (bail)
            break;
    }

    /* Call the wizard if we cannot read the config file.
     * This fixes a crash that happened if there was not a config file, and
     * tilda was not called with "-C" */
    if ((fp = fopen(tw->config_file, "r")) == NULL)
    {
        perror ("Unable to open config file, showing the wizard\n");
        if ((wizard (argc, argv, tw, tt)) == 1) { clean_up_no_main (tw); }
    }
    else
        fclose (fp);

    if (read_config_file (argv[0], tw->tilda_config, NUM_ELEM, tw->config_file) < 0)
    {
        perror ("There was an error in the config file, terminating");
        exit(1);
    }

    if (tmp_trans >= 0)
        tw->tc->transparency = tmp_trans;

    if (x_pos_arg >= 0)
        tw->tc->x_pos = x_pos_arg;

    if (y_pos_arg >= 0)
        tw->tc->y_pos = y_pos_arg;

    if (strlen (s_font_arg) > 0)
        g_strlcpy (tw->tc->s_font, s_font_arg, sizeof (tw->tc->s_font));

    if (strlen (s_background_arg) > 0)
        g_strlcpy (tw->tc->s_background, s_background_arg, sizeof (tw->tc->s_background));

    if (strlen (s_image_arg) > 0)
        g_strlcpy (tw->tc->s_image, s_image_arg, sizeof (tw->tc->s_image));

    if (strcasecmp (tw->tc->s_key, "null") == 0)
        g_snprintf (tw->tc->s_key, sizeof(tw->tc->s_key), "None+F%i", tw->instance+1);

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
    free (tw->tc);
    free (tw);

    return 0;
}

