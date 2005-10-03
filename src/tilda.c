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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <stdio.h>
#include <signal.h>

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

char *user, *display;

void getinstance (tilda_window *tw)
{
    char *home_dir;
    char buf[1024];
    char filename[1024], tmp[100];
    FILE *ptr;

    tw->instance = 0;

    home_dir = getenv ("HOME");
    g_strlcpy (tw->lock_file, home_dir, sizeof(tw->lock_file));
    g_strlcat (tw->lock_file, "/.tilda", sizeof(tw->lock_file));
    g_strlcat (tw->lock_file, "/locks/", sizeof(tw->lock_file));

    mkdir (tw->lock_file,  S_IRUSR | S_IWUSR | S_IXUSR);

    for (;;)
    {
        g_strlcpy (tmp, "ls ~/.tilda/locks/lock", sizeof(tmp));
        sprintf (filename, "%s_*_%d", tmp, tw->instance);
        sprintf (filename, "%s 2> /dev/null", filename);

        if ((ptr = popen (filename, "r")) != NULL)
        {
            if (fgets (buf, BUFSIZ, ptr) != NULL)
                tw->instance++;
            else
            {
                pclose (ptr);
                break;
            }
            pclose (ptr);
        }
    }

    sprintf (filename, "%slock_%d_%d", tw->lock_file, getpid(), tw->instance);
    g_strlcpy (tw->lock_file, filename, sizeof (tw->config_file));
    creat (tw->lock_file, S_IRUSR | S_IWUSR | S_IXUSR);
}

void clean_tmp ()
{
    char *home_dir;
    char pid[10];
    char cmd[128];
    char buf[1024], filename[1024];
    char tmp[100];
    char *throw_away;
    FILE *ptr, *ptr2;
    gboolean old = TRUE;

    home_dir = getenv ("HOME");
    g_strlcpy (cmd, "ls ", sizeof(cmd));
    g_strlcat (cmd, home_dir, sizeof(cmd));
    g_strlcat (cmd, "/.tilda/locks/lock_* 2> /dev/null", sizeof(cmd));

    if ((ptr = popen (cmd, "r")) != NULL)
    {
        while (fgets (buf, 1024, ptr) != NULL)
        {
            g_strlcpy (filename, buf, sizeof (filename));
            throw_away = strtok (buf, "/");
            while (throw_away)
            {
                g_strlcpy (tmp, throw_away, sizeof (tmp));
                throw_away = strtok (NULL, "/");

                if (!throw_away)
                {
                    throw_away = strtok (tmp, "_");
                    throw_away = strtok (NULL, "_");
                    g_strlcpy (pid, throw_away, sizeof (pid));
                    break;
                }
            }

            g_strlcpy (cmd, "ps x | grep ", sizeof(cmd));
            g_strlcat (cmd, pid, sizeof (cmd));

            if ((ptr2 = popen (cmd, "r")) != NULL)
            {
                while (fgets (tmp, 1024, ptr2) != NULL)
                {
                    if (strstr (tmp, "tilda") != NULL)
                    {
                        old = FALSE;
                        break;
                    }
                }

                if (old)
                {
                    filename[strlen(filename)-1] = '\0';
                    remove (filename);
                }
                else
                    old = TRUE;

                pclose (ptr2);
            }
        }
    }

    pclose (ptr);
}

int main (int argc, char **argv)
{
    tilda_window *tw;
    tilda_term *tt;

    gchar *home_dir;
    FILE *fp;
    gint  opt;
    gint  i, j;
    GList *args = NULL;
    gint tmp_trans = -1, x_pos_arg = -1, y_pos_arg = -1;
    gchar s_font_arg[64];
    gchar s_background_arg[7];
    gchar s_image_arg[100];

    const char *usage = "Usage: %s "
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

    /* Gotta do this first to make sure no lock files are left over */
    clean_tmp ();

    /* create new tilda window and terminal */
    tw = (tilda_window *) malloc (sizeof (tilda_window));
    tw->tc = (tilda_conf *) malloc (sizeof (tilda_conf));
    tt = (tilda_term *) malloc (sizeof (tilda_term));

    init_tilda_window_configs (tw);

    /* Have to do this early. */
    if (getenv ("VTE_PROFILE_MEMORY"))
    {
        if (atol (getenv ("VTE_PROFILE_MEMORY")) != 0)
        {
            g_mem_set_vtable (glib_mem_profiler_table);
        }
    }

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
    
    home_dir = getenv ("HOME");
    g_strlcpy (tw->config_file, home_dir, sizeof(tw->config_file));
    g_strlcat (tw->config_file, "/.tilda/config", sizeof(tw->config_file));
    sprintf (tw->config_file, "%s_%i", tw->config_file, tw->instance);

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
        if ((wizard (argc, argv, tw, tt)) == 1) { clean_up (tw); }
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
        sprintf (tw->tc->s_key, "None+F%i", tw->instance+1);

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

    remove (tw->lock_file);
    free (tw->tc);
    free (tw);

    return 0;
}

