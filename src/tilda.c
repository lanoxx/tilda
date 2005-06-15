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
#include <pthread.h>
#include <stdio.h>
#include <signal.h>

#ifdef HAVE_XFT2
#include <fontconfig/fontconfig.h>
#endif

/* #include "vte.h" */
#include <vte/vte.h>
#include "config.h"
#include "tilda.h"

/* source file containing most of the callback functions used */
#include "callback_func.c"

#define DINGUS1 "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?"
#define DINGUS2 "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?/[-A-Za-z0-9_\\$\\.\\+\\!\\*\\(\\),;:@&=\\?/~\\#\\%]*[^]'\\.}>\\) ,\\\"]"

char config_file[80];
char *user, *display;
char lock_file[80];      /* stores the name of the socket used for accessing this instance */


//gboolean bool_grab_focus;

/* Removes the temporary file socket used to communicate with a running tilda */
void clean_up () 
{
    remove (lock_file);
    
    exit (0);
}

void fix_size_settings ()
{
    gtk_window_resize ((GtkWindow *) window, max_width, max_height);
    gtk_window_get_size ((GtkWindow *) window, &max_width, &max_height);
    gtk_window_resize ((GtkWindow *) window, min_width, min_height);
    gtk_window_get_size ((GtkWindow *) window, &min_width, &min_height);
}

void getinstance ()
{
    char tmp[80];
	char *home_dir;
    
    instance = 0;
    
    home_dir = getenv ("HOME");
    strlcpy (lock_file, home_dir, sizeof(lock_file));
    strlcat (lock_file, "/.tilda", sizeof(lock_file));
    strlcat (lock_file, "/locks/", sizeof(lock_file));
    
    mkdir (lock_file,  S_IRUSR | S_IWUSR | S_IXUSR);
 
    strlcat (lock_file, "lock", sizeof(lock_file));
    strlcpy (tmp, lock_file, sizeof (tmp));
    sprintf (tmp, "%s_%i", tmp, instance);

    for (;;)
    {
		if (access (tmp, R_OK) == 0)
        {
            instance++;
            strlcpy (tmp, lock_file, sizeof (tmp));
        	sprintf (tmp, "%s_%i", tmp, instance); 
        }
        else
        	break;
	}

    strlcpy (lock_file, tmp, sizeof (config_file));
    creat (lock_file, S_IRUSR | S_IWUSR | S_IXUSR);
}   

void cleantmp ()
{
    char cmd[128];
    char buf[BUFSIZ], filename[BUFSIZ];
    int  length, i;
    FILE *ptr, *ptr2;
    int error_to_null;
    
    strlcpy (cmd, "ls /tmp/tilda.", sizeof(cmd));
    strlcat (cmd, user, sizeof(cmd));
    length = strlen (cmd)-(strlen ("ls /tmp/") + 1);
    
    strlcat (cmd, "*", sizeof(cmd));
    
    /* Don't know if this is the smartest thing to do but
     * it fixes the problem of always saying there isnt a 
     * file in /tmp
     */
    if ((error_to_null = open ("/dev/null", O_RDWR)) != -1)
        dup2 (error_to_null, 2);
    
    
    if ((ptr = popen (cmd, "r")) != NULL)
    {
        while (fgets (buf, BUFSIZ, ptr) != NULL)
        {
            strncpy (filename, buf + length - 1, strlen (buf + length - 1) - 1);
            filename[strlen (buf + length - 1)] = '\0';
            strlcpy (buf, strstr (buf + length - 1, ".") + 1, sizeof(buf));
            length = strstr (buf, ".") - (char*)&buf;
            buf[(int)(strstr (buf, ".") - (char*)&buf)] = '\0';
            strlcpy (cmd, "ps x | grep ", sizeof(cmd));
            strlcat (cmd, buf, sizeof(cmd));
        
            if ((ptr2 = popen (cmd, "r")) != NULL)
            {
                for (i=0;fgets(buf, BUFSIZ, ptr2)!=NULL;i++);
        
                if (i <= 2)
                {
                    strlcpy (cmd, "/tmp/tilda.", sizeof(cmd));
                    strlcat (cmd, filename, sizeof(cmd));
                    remove (cmd);    
                }
                pclose (ptr2);
            }
        } 
    }
    
    if (error_to_null != -1)
        close (error_to_null);
    
    pclose (ptr);
}

int main (int argc, char **argv)
{
	GtkAccelGroup *accel_group;
    GClosure *clean;
    char *home_dir;
    FILE *fp;
    GError *error = NULL;
    char *env_add[] = {"FOO=BAR", "BOO=BIZ", NULL, NULL};
    int  env_add2_size;
    float tmp_val;
    gboolean audible = TRUE, blink = TRUE, dingus = FALSE, 
    		  geometry = TRUE, dbuffer = TRUE, console = FALSE, 
              scroll = FALSE, icon_title = FALSE, shell = TRUE;
	gboolean image_set_clo=FALSE, antialias_set_clo=FALSE, scroll_set_clo=FALSE; 
    const char *command = NULL;
    const char *working_directory = NULL;
    char env_var[14];
    //char **argv2;
    int  opt;
    int  i, j;
    GList *args = NULL;
    GdkColor fore, back, tint, highlight, cursor;
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
                "-T : pull down terminal\n"
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

    back.red = back.green = back.blue = 0xffff;
    fore.red = fore.green = fore.blue = 0x0000;
    highlight.red = highlight.green = highlight.blue = 0xc000;
    cursor.red = 0xffff;
    cursor.green = cursor.blue = 0x8000;
    tint.red = tint.green = tint.blue = 0;
    tint = back;
    
    strlcpy (s_font, "monospace 9", sizeof (s_font));
    user = getenv ("USER");
    display = getenv ("DISPLAY");
    
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
    //cleantmp ();
    getinstance ();
    i=instance;
    sprintf (env_var, "TILDA_NUM=%d", instance);

    /*check for -T argument, if there is one just write to the pipe and exit, this will bring down or move up the term*/
    while ((opt = getopt(argc, argv, "x:y:B:CDTab:c:df:ghkn:st:wl:-")) != -1) 
     {
        gboolean bail = FALSE;
        switch (opt) {
            case 'B':
            	image_set_clo = TRUE;
                strlcpy (s_image, optarg, sizeof (s_image));
                break;
            case 'b':
                strlcpy (s_background, optarg, sizeof (s_background));
                break;  
            case 'T':
                
                break;
            case 'C':
                if ((wizard (argc, argv)) == 1) { clean_up(); }
                break;
            case 's':
                scroll_set_clo = TRUE;
                
                break;
            case 'c':
                command = optarg;
                break;
            case 't':
                tmp_val = atoi (optarg);
                ///if (tmp_val <= 100 && tmp_val >=0 ) { TRANS_LEVEL = (tmp_val)/100; }
                break;
            case 'f':
                strlcpy (s_font, optarg, sizeof (s_font));
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
                lines = atoi (optarg);

                if (lines <= 0)
                    lines = DEFAULT_LINES;

                break;
            case '-':
                bail = TRUE;
                break;
            case 'x':
                x_pos = atoi (optarg);
                break;
            case 'y':
                y_pos = atoi (optarg);
                break;
            default:
                break;
        }
        if (bail) 
            break;
    } 
    
    home_dir = getenv ("HOME");
    strlcpy (config_file, home_dir, sizeof(config_file));
    strlcat (config_file, "/.tilda/config", sizeof(config_file));
	sprintf (config_file, "%s_%i", config_file, instance);

    /* Call the wizard if we cannot read the config file.
     * This fixes a crash that happened if there was not a config file, and
     * tilda was not called with "-C" */
    if ((fp = fopen(config_file, "r")) == NULL)
    {
        printf("Unable to open config file, showing the wizard\n");
        if ((wizard (argc, argv)) == 1) { clean_up(); }
    }
    else
        fclose (fp);

    if (read_config_file (argv[0], tilda_config, NUM_ELEM(tilda_config), config_file) < 0)
    {
        puts("There was an error in the config file, terminating");
        exit(1);
    }

    env_add2_size = (sizeof(char) * strlen (env_var)) + 1;
    env_add[2] = (char *) malloc (env_add2_size);
    strlcpy (env_add[2], env_var, env_add2_size);

	g_thread_init(NULL);
  	gdk_threads_init();
    
    gtk_init (&argc, &argv);

    /* Create a window to hold the scrolling shell, and hook its
     * delete event to the quit function.. */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_resize_mode (GTK_CONTAINER(window), GTK_RESIZE_IMMEDIATE);
    g_signal_connect (G_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(deleted_and_quit), window);

    /* Create a box to hold everything. */
    hbox = gtk_hbox_new (0, FALSE);
    gtk_container_add (GTK_CONTAINER(window), hbox);

    /* Create the terminal widget and add it to the scrolling shell. */
    widget = vte_terminal_new ();
    
    if (!dbuffer) 
        gtk_widget_set_double_buffered (widget, dbuffer);
    
    gtk_box_pack_start (GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    /* Connect to the "char_size_changed" signal to set geometry hints
     * whenever the font used by the terminal is changed. */
    if (geometry)
    {
        char_size_changed (widget, 0, 0, window);
        g_signal_connect (G_OBJECT(widget), "char-size-changed",
                          G_CALLBACK(char_size_changed), window);
    }

    /* Connect to the "window_title_changed" signal to set the main
     * window's title. */
    g_signal_connect (G_OBJECT(widget), "window-title-changed",
                      G_CALLBACK(window_title_changed), window);
    if (icon_title) 
    {
        g_signal_connect (G_OBJECT(widget), "icon-title-changed",
                          G_CALLBACK(icon_title_changed), window);
    }

    /* Connect to the "eof" signal to quit when the session ends. */
    g_signal_connect (G_OBJECT(widget), "eof",
                      G_CALLBACK(destroy_and_quit_eof), window);
    g_signal_connect (G_OBJECT(widget), "child-exited",
                      G_CALLBACK(destroy_and_quit_exited), window);

    /* Connect to the "status-line-changed" signal. */
    g_signal_connect (G_OBJECT(widget), "status-line-changed",
                      G_CALLBACK(status_line_changed), widget);

    /* Connect to the "button-press" event. */
    g_signal_connect (G_OBJECT(widget), "button-press-event",
                      G_CALLBACK(button_pressed), widget);

    /* Connect to application request signals. */
    g_signal_connect (G_OBJECT(widget), "iconify-window",
                      G_CALLBACK(iconify_window), window);
    g_signal_connect (G_OBJECT(widget), "deiconify-window",
                      G_CALLBACK(deiconify_window), window);
    g_signal_connect (G_OBJECT(widget), "raise-window",
                      G_CALLBACK(raise_window), window);
    g_signal_connect (G_OBJECT(widget), "lower-window",
                      G_CALLBACK(lower_window), window);
    g_signal_connect (G_OBJECT(widget), "maximize-window",
                      G_CALLBACK(maximize_window), window);
    g_signal_connect (G_OBJECT(widget), "restore-window",
                      G_CALLBACK(restore_window), window);
    g_signal_connect (G_OBJECT(widget), "refresh-window",
                      G_CALLBACK(refresh_window), window);
    g_signal_connect (G_OBJECT(widget), "resize-window",
                      G_CALLBACK(resize_window), window);
    g_signal_connect (G_OBJECT(widget), "move-window",
                      G_CALLBACK(move_window), window);

    /* Connect to font tweakage. */
    g_signal_connect (G_OBJECT(widget), "increase-font-size",
                      G_CALLBACK(increase_font_size), window);
    g_signal_connect (G_OBJECT(widget), "decrease-font-size",
                      G_CALLBACK(decrease_font_size), window);

	/* Exit on Ctrl-Q */
    clean = g_cclosure_new (clean_up, NULL, NULL);
	accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
	gtk_accel_group_connect (accel_group, 'q', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, clean);

    /* Create the scrollbar for the widget. */
    scrollbar = gtk_vscrollbar_new ((VTE_TERMINAL(widget))->adjustment);
    gtk_box_pack_start (GTK_BOX(hbox), scrollbar, FALSE, FALSE, 0);
        
    /* Set some defaults. */
    vte_terminal_set_audible_bell (VTE_TERMINAL(widget), audible);
    vte_terminal_set_visible_bell (VTE_TERMINAL(widget), !audible);
    vte_terminal_set_cursor_blinks (VTE_TERMINAL(widget), blink);
    vte_terminal_set_scroll_background (VTE_TERMINAL(widget), scroll);
    vte_terminal_set_scroll_on_output (VTE_TERMINAL(widget), FALSE);
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(widget), TRUE);
    vte_terminal_set_scrollback_lines (VTE_TERMINAL(widget), lines);
    vte_terminal_set_mouse_autohide (VTE_TERMINAL(widget), TRUE);        
    
    load_tilda (TRUE);

    /* Match "abcdefg". */
    vte_terminal_match_add (VTE_TERMINAL(widget), "abcdefg");
    
    if (dingus) 
    {
        i = vte_terminal_match_add (VTE_TERMINAL(widget), DINGUS1);
        vte_terminal_match_set_cursor_type (VTE_TERMINAL(widget), i, GDK_GUMBY);
        i = vte_terminal_match_add(VTE_TERMINAL (widget), DINGUS2);
        vte_terminal_match_set_cursor_type (VTE_TERMINAL(widget), i, GDK_HAND1);
    }

    if (!console) 
    {
        if (shell) 
        {
            /* Launch a shell. */
            if (command == NULL)
            {
                #ifdef DEBUG
                puts ("had to fix 'command'");
                #endif

                command = getenv ("SHELL"); /* possible buffer overflow? */
            }

            vte_terminal_fork_command (VTE_TERMINAL(widget),
                                       command, NULL, env_add,
                                       working_directory,
                                       TRUE, TRUE, TRUE);
        }
    }
    
    gtk_window_set_decorated ((GtkWindow *) window, FALSE);
    
    g_object_add_weak_pointer (G_OBJECT(widget), (gpointer*)&widget);
    g_object_add_weak_pointer (G_OBJECT(window), (gpointer*)&window);

    gtk_widget_set_size_request ((GtkWidget *) window, 0, 0);
    fix_size_settings ();
    gtk_window_resize ((GtkWindow *) window, min_width, min_height);

    signal (SIGINT, clean_up);
    signal (SIGQUIT, clean_up);
    signal (SIGABRT, clean_up);
    signal (SIGKILL, clean_up);
    signal (SIGABRT, clean_up);
    signal (SIGTERM, clean_up);
    
    if (!g_thread_create ((GThreadFunc) wait_for_signal, NULL, FALSE, &error))
        perror ("Fuck that thread!!!");
    
    gdk_threads_enter ();
    gtk_main();
	gdk_threads_leave ();
    
    printf ("remove %s\n", lock_file);
    remove (lock_file);

    return 0;
}
