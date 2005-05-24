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

GtkWidget *window;
char config_file[80];
char *user, *display;
char *filename_global;      /* stores the name of the socket used for accessing this instance */
int  filename_global_size;  /* stores the size of filename_global */
int  instance;              /* stores this instance's number */
int TRANS_LEVEL = 0;       /* how transparent the window is, percent from 0-100 */
int pos_x = 0;              /* x position of tilda on screen */
int pos_y = 0;              /* y position of tilda on screen */

/* Removes the temporary file socket used to communicate with a running tilda */
void clean_up () 
{
    if (filename_global != NULL) 
    {
        remove (filename_global);
        free (filename_global);
    }
    
    exit (0);
}

void start_process (char process[]) 
{
    /* This really should be rewritten to use fork()/exec()
       and it should ideally have a way of communicating with
       "process" in preferably a socket or something.
       As far as I can tell we only ever use it to start
       xbindkeys - should consider either talkin to the developers
       of xbindkeys and find a way to talk to it or simply not
       start xbindkeys on our own.

       Uses mkstemp() instead of tmpname(), tmpname() is
       insecure and open to race conditions and security
       problems - mkstemp() isn't quite as standard as it should
       be, but at least available on linux systems
    */
   
    FILE *tmpfile;
    char tmpname[] = "/tmp/tildaXXXXXX";
    int  tmpdesc;
    char tmp_string[256];
    char command[128];

    /* Get a filedescriptor for a temporary file */
    tmpdesc = mkstemp (tmpname);
   
    if (tmpdesc != -1) 
    {
    /* Managed to get a file ?
       Associate a FILE* with the descriptor */
        if ((tmpfile = fdopen (tmpdesc, "w+")) == NULL) 
        {
            /* Failed to associate FILE* */
            perror ("fdopen tmpdesc");
            exit (1);
        }
    }
    else 
    {
        /* Failed to create a temporary file */
        perror ("mkstemp(tmpname)");
        exit (1);
    }
    
    strlcpy (command, "ps aux > ", sizeof(command));
    strlcat (command, tmpname, sizeof(command));

    system (command);

    while (!feof (tmpfile)) 
    {
        fgets (tmp_string, sizeof(tmp_string), tmpfile);

        if (strstr (tmp_string, process) != NULL) 
        {   
            goto LABEL;
        }
    }

    system (process);

LABEL:

    fclose (tmpfile);
    remove (tmpname);
}

void fix_size_settings ()
{
    gtk_window_resize ((GtkWindow *) window, max_width, max_height);
    gtk_window_get_size ((GtkWindow *) window, &max_width, &max_height);
    gtk_window_resize ((GtkWindow *) window, min_width, min_height);
    gtk_window_get_size ((GtkWindow *) window, &min_width, &min_height);
}

/* Pulls "down" a running tilda instance */
void pull_down (char *instance)
{
    char buf[BUFSIZ];
    FILE *fp, *ptr;
    char filename[128], *tmp;
    int  tmp_size;
    
    /* generate socket name into 'filename' */
    strlcpy (filename, "ls /tmp/tilda.", sizeof(filename));
    strlcat (filename, user, sizeof(filename));
    
    if (instance == NULL)
    {
        instance = (char *) malloc (sizeof (char));
        strlcpy (instance, "0", sizeof(char));
    }
    
    tmp_size = (sizeof(char) * strlen (filename)) + 1;
    tmp = (char *) malloc (tmp_size);
    strlcpy (tmp, filename, tmp_size);
    sprintf (filename, "%s.*.%s", tmp, instance); 
    strlcat (filename, display, sizeof(filename));
    /* finished generating socket name */    

    if ((ptr = popen (filename, "r")) != NULL) /* open 'filename' into ptr for reading */
    {
        if (fgets (buf, BUFSIZ, ptr) != NULL)  /* get the name of this instance's socket */
        {
            buf[strlen (buf)-1] = '\0';
            
            #ifdef DEBUG
            printf ("%s\n", buf);
            #endif

            if ((fp = fopen (buf, "w")) == NULL) /* open this instance's socket */
            {
                perror ("fopen");
                exit (1);
            }
            
            /* write the 'pulldown' command into the socket.
             * NOTE: the value of the command doesn't matter at all */
            fputs ("shits", fp);
            
            fclose (fp);
        }
        fclose (ptr);
    }
    
    exit (0); /* pull the terminal down */
}

void *wait_for_signal ()
{
    FILE *fp = NULL;   
    char filename[128], *tmp;
    int  tmp_size;
    char c[10];
    int  flag;
    gint w, h;
    
    strlcpy (filename, "/tmp/tilda.", sizeof(filename));
    strlcat (filename, user, sizeof(filename));

    tmp_size = (sizeof(char) * strlen (filename)) + 1;
    tmp = (char *) malloc (tmp_size);
    strlcpy (tmp, filename, tmp_size);
    sprintf (filename, "%s.%d.%d", tmp, getpid(), instance);
    free (tmp);
    
    strlcat (filename, display, sizeof(filename));
    
    filename_global_size = (sizeof(char) * strlen (filename)) + 1;
    filename_global = (char *) malloc (filename_global_size);
    strlcpy (filename_global, filename, filename_global_size);

    umask (0);
    mknod (filename, S_IFIFO|0666, 0);

    signal (SIGINT, clean_up);
    signal (SIGQUIT, clean_up);
    signal (SIGABRT, clean_up);
    signal (SIGKILL, clean_up);
    signal (SIGABRT, clean_up);
    signal (SIGTERM, clean_up);
    
    flag = 0;
    
    for (;;)
    {
        if (flag)
        {
            fp = fopen (filename, "r");

            if (fp != NULL)
                fgets (c, 10, fp);
            else
            {
                printf("Failed to open: %s\n", filename);
                exit(1);
            }
        }
    
        gtk_window_get_size ((GtkWindow *) window, &w, &h);
        /* gtk_window_get_position ((GtkWindow *) window, &x, &y); */
    
        if (h == min_height)
        {
            resize ((GtkWidget *) window, max_width, max_height);
            gtk_widget_show ((GtkWidget *) window);
            gtk_window_move ((GtkWindow *) window, pos_x, pos_y);
            
            if ((strcasecmp (s_pinned, "true")) == 0)
                gtk_window_stick (GTK_WINDOW (window));

            if ((strcasecmp (s_above, "true")) == 0)
                gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
        
        }
        else if (h == max_height)
        {   
            resize ((GtkWidget *) window, min_width, min_height);
            /* gtk_window_move((GtkWindow *) window, 0, -min_height); */
        
            gtk_widget_hide ((GtkWidget *) window);     
        }
        
        if (flag)
        {
            fclose (fp);
            fp = NULL;
        }
        else
            flag = 1; 

        sleep (.1);
    }
    
    return NULL;
}

void getinstance ()
{
    char buf[BUFSIZ];
    char filename[BUFSIZ], tmp[100];
    FILE *ptr;
    
    instance = 0;
    
    for (;;)
    {
        strlcpy (tmp, "ls /tmp/tilda.", sizeof(tmp));
        strlcat (tmp, user, sizeof(tmp));
        sprintf (filename, "%s.*.%d", tmp, instance);
        strlcat (filename, display, sizeof(filename));
        
        if ((ptr = popen (filename, "r")) != NULL)
        {
            if (fgets (buf, BUFSIZ, ptr) != NULL)
                instance++;
            else
            {
                pclose (ptr);
                break;
            }
            pclose (ptr);
        } 
    }
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
    pthread_t child; 
    int  tid;
    char *home_dir;
    FILE *fp;
    GtkWidget *hbox, *scrollbar, *widget;
    char *env_add[] = {"FOO=BAR", "BOO=BIZ", NULL, NULL};
    int  env_add2_size;
    float tmp_val;
    gboolean audible = TRUE, blink = TRUE,
         dingus = FALSE, geometry = TRUE, dbuffer = TRUE,
         console = FALSE, scroll = FALSE,
         icon_title = FALSE, shell = TRUE, highlight_set = FALSE,
         cursor_set = FALSE, use_antialias = FALSE, bool_use_image = FALSE;
    VteTerminalAntiAlias antialias = VTE_ANTI_ALIAS_USE_DEFAULT;
    const char *terminal = NULL;
    const char *command = NULL;
    const char *working_directory = NULL;
    char env_var[14];
    char **argv2;
    int  opt;
    int  i, j;
    GList *args = NULL;
    GdkColor fore, back, tint, highlight, cursor;
    const char *usage = "Usage: %s "
                "[-B image] "
                "[-T N] "
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
                "-T N : pull down terminal N if already running\n"
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
    
    home_dir = getenv ("HOME");
    strlcpy (config_file, home_dir, sizeof(config_file));
    strlcat (config_file, "/.tilda/config", sizeof(config_file));

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
    
    if (strcmp (s_use_image, "TRUE") == 0)
        bool_use_image = TRUE;
    
    if (strcmp (s_antialias, "TRUE") == 0)
        use_antialias = TRUE;
    
    if (strcmp (s_scrollbar, "TRUE") == 0)
        scroll = TRUE;          

    TRANS_LEVEL = (transparency)/100; 
    
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
    
    argv2 = g_malloc0 (sizeof(char*) * (g_list_length (args) + 2));
    argv2[0] = argv[0];
    
    for (i=1;i<=g_list_length(args);i++) 
    	argv2[i] = (char*) g_list_nth (args, i - 1);
    
    argv2[i] = NULL;
    g_assert (i < (g_list_length (args) + 2));


    /*check for -T argument, if there is one just write to the pipe and exit, this will bring down or move up the term*/
    while ((opt = getopt(argc, argv, "x:y:B:CDT:2ab:c:df:ghkn:st:wl:-")) != -1) 
     {
        gboolean bail = FALSE;
        switch (opt) {
            case 'B':
            	bool_use_image = TRUE;
                strlcpy (s_image, optarg, sizeof (s_image));
                break;
            case 'b':
                strlcpy (s_background, optarg, sizeof (s_background));
                break;  
            case 'T':
                pull_down (optarg);
                break;
            case 'C':
                if ((wizard (argc, argv)) == 1) { clean_up(); }
                break;
            case 's':
                scroll = TRUE;
                break;
            case 'c':
                command = optarg;
                break;
            case 't':
                tmp_val = atoi (optarg);
                if (tmp_val <= 100 && tmp_val >=0 ) { TRANS_LEVEL = (tmp_val)/100; }
                break;
            case 'f':
                strlcpy (s_font, optarg, sizeof (s_font));
                break;
            case 'w':
                working_directory = optarg;
                break;
            case 'a':
                use_antialias = TRUE;
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
    
     /* set the instance number and place a env in the array of envs 
      * to be set when the tilda terminal is created */
    cleantmp ();
    getinstance ();
    i=instance;
    sprintf (env_var, "TILDA_NUM=%d", instance);

    env_add2_size = (sizeof(char) * strlen (env_var)) + 1;
    env_add[2] = (char *) malloc (env_add2_size);
    strlcpy (env_add[2], env_var, env_add2_size);

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

    if (bool_use_image) 
    {
        vte_terminal_set_background_image_file (VTE_TERMINAL(widget), s_image);
    }
    
    if (TRANS_LEVEL > 0) 
    {
        vte_terminal_set_background_saturation (VTE_TERMINAL (widget), TRANS_LEVEL);
        vte_terminal_set_background_transparent (VTE_TERMINAL(widget), TRUE);
    }
    
    if (strcasecmp (s_background, "black") == 0)
    {
        back.red = back.green = back.blue = 0x0000;
        fore.red = fore.green = fore.blue = 0xffff;
    }

    vte_terminal_set_background_tint_color (VTE_TERMINAL(widget), &tint);
    vte_terminal_set_colors (VTE_TERMINAL(widget), &fore, &back, NULL, 0);
    
    if (highlight_set) 
    {
        vte_terminal_set_color_highlight (VTE_TERMINAL(widget), &highlight);
    }
    
    if (cursor_set) 
    {
        vte_terminal_set_color_cursor (VTE_TERMINAL(widget), &cursor);
    }
    
    if (terminal != NULL) 
    {
        vte_terminal_set_emulation (VTE_TERMINAL(widget), terminal);
    }

    if (use_antialias)
        vte_terminal_set_font_from_string_full (VTE_TERMINAL(widget), s_font, antialias);
    else
        vte_terminal_set_font_from_string (VTE_TERMINAL(widget), s_font);
    

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
    
    if (!scroll)
    {
        gtk_widget_show ((GtkWidget *) widget);
        gtk_widget_show ((GtkWidget *) hbox);
        gtk_widget_show ((GtkWidget *) window);
    }
    else
        gtk_widget_show_all (window);

    if ((strcasecmp (s_xbindkeys, "true")) == 0)
        start_process ("xbindkeys");
    
    if ((strcasecmp (s_above, "true")) == 0)
        gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
    
    if ((strcasecmp (s_pinned, "true")) == 0)
        gtk_window_stick (GTK_WINDOW (window));
    
    if ((strcasecmp (s_notaskbar, "true")) == 0)
        gtk_window_set_skip_taskbar_hint (GTK_WINDOW(window), TRUE);
    
    gtk_window_move ((GtkWindow *) window, pos_x, pos_y);
    
    signal (SIGINT, clean_up);
    signal (SIGQUIT, clean_up);
    signal (SIGABRT, clean_up);
    signal (SIGKILL, clean_up);
    signal (SIGABRT, clean_up);
    signal (SIGTERM, clean_up);
    
    if ((tid = pthread_create (&child, NULL, &wait_for_signal, NULL)) != 0)
        perror ("Fuck that thread!!!");
    
    gtk_main();

    pthread_cancel (child);
    pthread_join (child, NULL);
    
    remove (filename_global);
    free (filename_global);

    return 0;
}

