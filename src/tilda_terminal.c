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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <tilda-config.h>

#include "debug.h"
#include "tilda.h"
#include "tilda_window.h"
#include "tilda_terminal.h"
#include "callback_func.h"
#include "configsys.h"
#include "wizard.h" /* wizard */

#include <stdio.h>
#include <stdlib.h> /* malloc */
#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <vte/vte.h>
#include <string.h>

#define HTTP_REGEXP "(ftp|http)s?://[\\[\\]-a-zA-Z0-9.?$%&/=_~#.,:;+]*"

GdkRGBA current_palette[TERMINAL_PALETTE_SIZE];

static gint start_shell (struct tilda_term_ *tt, gboolean ignore_custom_command, const char* working_dir);
static gint tilda_term_config_defaults (tilda_term *tt);

#ifdef VTE_290
    static void child_exited_cb (GtkWidget *widget, gpointer data);
#else
    static void eof_cb (GtkWidget *widget, gpointer data);
    /* VTE 2.91 introduced a new status argument to the "child-exited" signal */
    static void child_exited_cb (GtkWidget *widget, gint status, gpointer data);
#endif
static void window_title_changed_cb (GtkWidget *widget, gpointer data);
static int button_press_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean key_press_cb (GtkWidget *widget, GdkEvent  *event, tilda_term *terminal);
static void iconify_window_cb (GtkWidget *widget, gpointer data);
static void deiconify_window_cb (GtkWidget *widget, gpointer data);
static void raise_window_cb (GtkWidget *widget, gpointer data);
static void lower_window_cb (GtkWidget *widget, gpointer data);
static void maximize_window_cb (GtkWidget *widget, gpointer data);
static void restore_window_cb (GtkWidget *widget, gpointer data);
static void refresh_window_cb (GtkWidget *widget, gpointer data);
static void move_window_cb (GtkWidget *widget, guint x, guint y, gpointer data);
#ifdef VTE_290
static void status_line_changed_cb (GtkWidget *widget, gpointer data);
#endif

gint tilda_term_free (struct tilda_term_ *term)
{
    DEBUG_FUNCTION ("tilda_term_free");
    DEBUG_ASSERT (term != NULL);

    g_free (term);

    return 0;
}

static void tilda_terminal_switch_page_cb (GtkNotebook *notebook,
                                           GtkWidget   *page,
                                           guint        page_num,
                                           tilda_window *tw)
{
    DEBUG_FUNCTION ("tilda_terminal_switch_page_cb");
    guint counter = 0;
    tilda_term *term = NULL;
    for(GList *item=tw->terms; item != NULL; item=item->next) {
        if(counter == page_num) {
            term = (tilda_term*) item->data;
        }
        counter++;
    }
    const char* current_title = vte_terminal_get_window_title (VTE_TERMINAL (term->vte_term));
    gtk_window_set_title (GTK_WINDOW (tw->window), current_title);
}

struct tilda_term_ *tilda_term_init (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("tilda_term_init");
    DEBUG_ASSERT (tw != NULL);

    int ret;
    struct tilda_term_ *term;
    GError *error = NULL;
    tilda_term *current_tt;
    gint current_tt_index;
    char *current_tt_dir = NULL;

    term = g_malloc (sizeof (struct tilda_term_));

    /* Add to GList list of tilda_term structures in tilda_window structure */
    tw->terms = g_list_append (tw->terms, term);

    /* Check for a failed allocation */
    if (!term)
        return NULL;

    /* Set the PID to unset value */
    term->pid = -1;

    /* Add the parent window reference */
    term->tw = tw;

    /* Create a non-homogenous hbox, with 0px spacing between members */
    term->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    /* Create the terminal */
    term->vte_term = vte_terminal_new ();

    /* Create the scrollbar for the terminal */
    term->scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL,
        gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (VTE_TERMINAL(term->vte_term))));

    gtk_widget_set_no_show_all (term->scrollbar, TRUE);

    /* Initialize to false, we have not yet dropped to the default shell */
    term->dropped_to_default_shell = FALSE;

    /* Set properties of the terminal */
    tilda_term_config_defaults (term);

    /* Update the font scale because the newly created terminal uses the default font size */
    tilda_term_adjust_font_scale(term, tw->current_scale_factor);

    /* Pack everything into the hbox */
    gtk_box_pack_end (GTK_BOX(term->hbox), term->scrollbar, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX(term->hbox), term->vte_term, TRUE, TRUE, 0);
    gtk_widget_show (term->scrollbar);

    /* Set the scrollbar position */
    tilda_term_set_scrollbar_position (term, config_getint ("scrollbar_pos"));

    /** Signal Connection **/
    g_signal_connect (G_OBJECT(term->vte_term), "child-exited",
                      G_CALLBACK(child_exited_cb), term);
    g_signal_connect (G_OBJECT(term->vte_term), "window-title-changed",
                      G_CALLBACK(window_title_changed_cb), term);
#ifdef VTE_290
    g_signal_connect (G_OBJECT(term->vte_term), "eof",
                      G_CALLBACK(child_exited_cb), term);
    g_signal_connect (G_OBJECT(term->vte_term), "status-line-changed",
                      G_CALLBACK(status_line_changed_cb), term);
#else
    g_signal_connect (G_OBJECT(term->vte_term), "eof",
                      G_CALLBACK(eof_cb), term);
#endif
    g_signal_connect (G_OBJECT(term->vte_term), "button-press-event",
                      G_CALLBACK(button_press_cb), term);
    g_signal_connect (G_OBJECT(term->vte_term), "key-press-event",
		      G_CALLBACK(key_press_cb), term); //needs GDK_KEY_PRESS_MASK

    /* Connect to application request signals. */
    g_signal_connect (G_OBJECT(term->vte_term), "iconify-window",
                      G_CALLBACK(iconify_window_cb), tw->window);
    g_signal_connect (G_OBJECT(term->vte_term), "deiconify-window",
                      G_CALLBACK(deiconify_window_cb), tw->window);
    g_signal_connect (G_OBJECT(term->vte_term), "raise-window",
                      G_CALLBACK(raise_window_cb), tw->window);
    g_signal_connect (G_OBJECT(term->vte_term), "lower-window",
                      G_CALLBACK(lower_window_cb), tw->window);
    g_signal_connect (G_OBJECT(term->vte_term), "maximize-window",
                      G_CALLBACK(maximize_window_cb), tw->window);
    g_signal_connect (G_OBJECT(term->vte_term), "restore-window",
                      G_CALLBACK(restore_window_cb), tw->window);
    g_signal_connect (G_OBJECT(term->vte_term), "refresh-window",
                      G_CALLBACK(refresh_window_cb), tw->window);
    g_signal_connect (G_OBJECT(term->vte_term), "move-window",
                      G_CALLBACK(move_window_cb), tw->window);
    g_signal_connect (G_OBJECT (tw->notebook), "switch-page",
                      G_CALLBACK (tilda_terminal_switch_page_cb), tw);

    /* Match URL's, etc */
    term->http_regexp=g_regex_new(HTTP_REGEXP, G_REGEX_CASELESS, G_REGEX_MATCH_NOTEMPTY, &error);
    ret = vte_terminal_match_add_gregex(VTE_TERMINAL(term->vte_term), term->http_regexp,0);
    vte_terminal_match_set_cursor_type (VTE_TERMINAL(term->vte_term), ret, GDK_HAND2);

    /* Show the child widgets */
    gtk_widget_show (term->vte_term);
    gtk_widget_show (term->hbox);

    /* Get current term's working directory */
    current_tt_index = gtk_notebook_get_current_page (GTK_NOTEBOOK(tw->notebook));
    current_tt = g_list_nth_data (tw->terms, current_tt_index);
    if (current_tt != NULL)
    {
        current_tt_dir = tilda_term_get_cwd(current_tt);
    }

    /* Fork the appropriate command into the terminal */
    ret = start_shell (term, FALSE, current_tt_dir);

    g_free(current_tt_dir);

    if (ret)
        goto err_fork;

    return term;

err_fork:
    g_free (term);
    return NULL;
}

void tilda_term_set_scrollbar_position (tilda_term *tt, enum tilda_term_scrollbar_positions pos)
{
    DEBUG_FUNCTION ("tilda_term_set_scrollbar_position");
    DEBUG_ASSERT (tt != NULL);
    DEBUG_ASSERT (pos == LEFT || pos == RIGHT || pos == DISABLED);

    if (pos == DISABLED) {
        gtk_widget_hide (tt->scrollbar);
    } else {
        /* We have already asserted that it's either disabled (already taken care of),
         * left, or right, so no need to check twice. */
        gtk_box_reorder_child (GTK_BOX(tt->hbox), tt->scrollbar, (pos == LEFT) ? 0 : 1);
        gtk_widget_show (tt->scrollbar);
    }
}

static void window_title_changed_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("window_title_changed_cb");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    tilda_term *tt = TILDA_TERM(data);
    gchar *title = get_window_title (widget);
    GtkWidget *label;

    label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (tt->tw->notebook), tt->hbox);
    /* We need to check if the widget that received the title change is the currently
     * active tab. If not we should not update the window title. */
    gint page = gtk_notebook_get_current_page (GTK_NOTEBOOK (tt->tw->notebook));
    tilda_term *currente_term;
    gboolean active = FALSE;
    if (page > 0) {
        currente_term = g_list_nth_data (tt->tw->terms, (guint) page);
        active = widget == currente_term->vte_term;
    }

    guint length = (guint) config_getint ("title_max_length");

    if(config_getbool("title_max_length_flag") && strlen(title) > length) {
        gchar *titleOffset = title + strlen(title) - length;
        gchar *shortTitle = g_strdup_printf ("...%s", titleOffset);
        gtk_label_set_text (GTK_LABEL(label), shortTitle);
        if (active) {
            gtk_window_set_title (GTK_WINDOW (tt->tw->window), shortTitle);
        }
        g_free(shortTitle);
    } else {
        gtk_label_set_text (GTK_LABEL(label), title);
        if (active) {
            gtk_window_set_title (GTK_WINDOW (tt->tw->window), title);
        }
    }

    g_free (title);
}

#ifdef VTE_290
static void status_line_changed_cb (GtkWidget *widget, G_GNUC_UNUSED gpointer data)
{
    DEBUG_FUNCTION ("status_line_changed_cb");
    DEBUG_ASSERT (widget != NULL);

    g_print ("Status = `%s'.\n", vte_terminal_get_status_line (VTE_TERMINAL(widget)));
}
#endif

static void iconify_window_cb (G_GNUC_UNUSED GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("iconify_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if (gtk_widget_get_window ((GTK_WIDGET (data))))
            gdk_window_iconify (gtk_widget_get_window ((GTK_WIDGET (data))));
}

static void deiconify_window_cb (G_GNUC_UNUSED GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("deiconify_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if (gtk_widget_get_window ((GTK_WIDGET (data))))
            gdk_window_deiconify (gtk_widget_get_window ((GTK_WIDGET (data))));
}

static void raise_window_cb (G_GNUC_UNUSED GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("raise_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if (gtk_widget_get_window (GTK_WIDGET (data)))
            gdk_window_raise (gtk_widget_get_window (GTK_WIDGET (data)));
}

static void lower_window_cb (G_GNUC_UNUSED GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("lower_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if (gtk_widget_get_window (GTK_WIDGET (data)))
            gdk_window_lower (gtk_widget_get_window (GTK_WIDGET (data)));
}

static void maximize_window_cb (G_GNUC_UNUSED GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("maximize_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if (gtk_widget_get_window (GTK_WIDGET(data)))
            gdk_window_maximize (gtk_widget_get_window (GTK_WIDGET(data)));
}

static void restore_window_cb (G_GNUC_UNUSED GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("restore_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if (gtk_widget_get_window (GTK_WIDGET (data)))
            gdk_window_unmaximize (gtk_widget_get_window (GTK_WIDGET (data)));
}

static void refresh_window_cb (G_GNUC_UNUSED GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("refresh_window_cb");
    DEBUG_ASSERT (data != NULL);

    GdkRectangle rect;
    if (GTK_IS_WIDGET(data))
    {
        if (gtk_widget_get_window (GTK_WIDGET (data)))
        {
            rect.x = rect.y = 0;
            GtkAllocation allocation;
            gtk_widget_get_allocation (GTK_WIDGET (data), &allocation);
            rect.width = allocation.width;
            rect.height = allocation.height;
            gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET(data)), &rect, TRUE);
        }
    }
}

static void move_window_cb (G_GNUC_UNUSED GtkWidget *widgets, guint x, guint y, gpointer data)
{
    DEBUG_FUNCTION ("move_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if (gtk_widget_get_window (GTK_WIDGET (data)))
            gdk_window_move (gtk_widget_get_window (GTK_WIDGET (data)), x, y);
}

void tilda_term_adjust_font_scale(tilda_term *term, gdouble scale) {
    DEBUG_FUNCTION ("tilda_term_adjust_font_scale");

    VteTerminal *terminal = VTE_TERMINAL(term->vte_term);
    /* We need the tilda_term object to access the unscaled
     * font size and current scale factor */
    PangoFontDescription *desired;

    desired = pango_font_description_copy (vte_terminal_get_font(terminal));
    pango_font_description_set_size (desired, (gint) (term->tw->unscaled_font_size * scale));
    vte_terminal_set_font (terminal, desired);
    pango_font_description_free (desired);
}

/* Returns the working directory of the terminal
 *
 * @param tt the tilda_term to get working directory of
 *
 * SUCCESS: return non-NULL char* that should be freed with g_free when done
 * FAILURE: return NULL
 */
char* tilda_term_get_cwd(struct tilda_term_* tt)
{
    char *file;
    char *cwd;
    GError *error = NULL;

    if (tt->pid < 0)
    {
        return NULL;
    }

    file = g_strdup_printf ("/proc/%d/cwd", tt->pid);
    cwd = g_file_read_link (file, &error);
    g_free (file);

    if (cwd == NULL)
    {
        g_printerr (_("Problem reading link %s: %s\n"), file, error->message);
        g_error_free (error);
    }

    return cwd;
}

/* Fork a shell into the VTE Terminal
 *
 * @param tt the tilda_term to fork into
 *
 * SUCCESS: return 0
 * FAILURE: return non-zero
 */
static gint start_shell (struct tilda_term_ *tt, gboolean ignore_custom_command, const char* working_dir)
{
    DEBUG_FUNCTION ("start_shell");
    DEBUG_ASSERT (tt != NULL);

    gint ret;
    gint argc;
    gchar **argv;
    GError *error = NULL;

    gchar *default_command;

    if (working_dir == NULL || config_getbool ("inherit_working_dir") == FALSE)
    {
        working_dir = config_getstr ("working_dir");
    }

    if (config_getbool ("run_command") && !ignore_custom_command)
    {
        ret = g_shell_parse_argv (config_getstr ("command"), &argc, &argv, &error);

        /* Check for error */
        if (ret == FALSE)
        {
            g_printerr (_("Problem parsing custom command: %s\n"), error->message);
            g_printerr (_("Launching default shell instead\n"));

            g_error_free (error);
            goto launch_default_shell;
        }

        char **envv = malloc(2*sizeof(void *));
        envv[0] = getenv("PATH");
        envv[1] = NULL;
#ifdef VTE_290
        ret = vte_terminal_fork_command_full (VTE_TERMINAL (tt->vte_term),
            VTE_PTY_DEFAULT, /* VtePtyFlags pty_flags */
            working_dir, /* const char *working_directory */
            argv, /* char **argv */
            envv, /* char **envv */
            G_SPAWN_SEARCH_PATH,    /* GSpawnFlags spawn_flags */
            NULL, /* GSpawnChildSetupFunc child_setup */
            NULL, /* gpointer child_setup_data */
            &tt->pid, /* GPid *child_pid */
            NULL  /* GError **error */
            );
#else
        ret = vte_terminal_spawn_sync (VTE_TERMINAL (tt->vte_term),
            VTE_PTY_DEFAULT, /* VtePtyFlags pty_flags */
            working_dir, /* const char *working_directory */
            argv, /* char **argv */
            envv, /* char **envv */
            G_SPAWN_SEARCH_PATH,    /* GSpawnFlags spawn_flags */
            NULL, /* GSpawnChildSetupFunc child_setup */
            NULL, /* gpointer child_setup_data */
            &tt->pid, /* GPid *child_pid */
            NULL, /* GCancellable * cancellable, */
            NULL  /* GError **error */
            );
#endif
        g_strfreev (argv);
        g_free (envv);

        /* Check for error */
        if (ret == FALSE)
        {
            g_printerr (_("Unable to launch custom command: %s\n"), config_getstr ("command"));
            g_printerr (_("Launching default shell instead\n"));

            goto launch_default_shell;
        }

        return 0; /* SUCCESS: the early way out */
    }

launch_default_shell:

    /* If we have dropped to the default shell before, then this time, we
     * do not spawn a new shell, but instead close the current shell. This will
     * cause the current tab to close.
     */
    if (tt->dropped_to_default_shell) {
        gint index = gtk_notebook_page_num (GTK_NOTEBOOK(tt->tw->notebook),
            tt->hbox);
        tilda_window_close_tab (tt->tw, index, FALSE);
        return 0;
    }
    if (ignore_custom_command) {
        tt->dropped_to_default_shell = TRUE;
    }

    /* No custom command, get it from the environment */
    default_command = (gchar *) g_getenv ("SHELL");

    /* Check for error */
    if (default_command == NULL)
        default_command = "/bin/sh";

    /* We need to create a NULL terminated list of arguments.
     * The first item is the command to execute in the shell, in this
     * case there are no further arguments being passed. */
    GSpawnFlags flags = 0;
    gchar* argv1 = NULL;
    if(config_getbool("command_login_shell")) {
        argv1 = g_strdup_printf("-%s", default_command);
        argv = malloc(3 * sizeof(void *));
        argv[0] = default_command;
        argv[1] = argv1;
        argv[2] = NULL;
        /* This is needed so that argv[1] becomes the argv[0] of the new process. Otherwise
         * glib just duplicates argv[0] when it executes the command and it is not possible
         * to modify the argv[0] that the new command sees.
         */
        flags |= G_SPAWN_FILE_AND_ARGV_ZERO;
    } else {
        argv = malloc(1 * sizeof(void *));
        argv[0] = default_command;
        argv[1] = NULL;
    }
#ifdef VTE_290
    ret = vte_terminal_fork_command_full (VTE_TERMINAL (tt->vte_term),
        VTE_PTY_DEFAULT, /* VtePtyFlags pty_flags */
        working_dir, /* const char *working_directory */
        argv, /* char **argv */
        NULL, /* char **envv */
        flags,    /* GSpawnFlags spawn_flags */
        NULL, /* GSpawnChildSetupFunc child_setup */
        NULL, /* gpointer child_setup_data */
        &tt->pid, /* GPid *child_pid */
        NULL  /* GError **error */
        );
#else
    ret = vte_terminal_spawn_sync (VTE_TERMINAL (tt->vte_term),
        VTE_PTY_DEFAULT, /* VtePtyFlags pty_flags */
        working_dir, /* const char *working_directory */
        argv, /* char **argv */
        NULL, /* char **envv */
        flags,    /* GSpawnFlags spawn_flags */
        NULL, /* GSpawnChildSetupFunc child_setup */
        NULL, /* gpointer child_setup_data */
        &tt->pid, /* GPid *child_pid */
        NULL, /* GCancellable *cancellable */
        NULL  /* GError **error */
        );
#endif
    g_free(argv1);
    g_free (argv);

    if (ret == -1)
    {
        g_printerr (_("Unable to launch default shell: %s\n"), default_command);
        return ret;
    }

    return 0;
}

#ifdef VTE_291
static void eof_cb (GtkWidget *widget, gpointer data) {
    DEBUG_FUNCTION ("eof_cb");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    child_exited_cb (widget, 0, data);
}
#endif

#ifdef VTE_290
static void child_exited_cb (GtkWidget *widget, gpointer data)
#else
static void child_exited_cb (GtkWidget *widget, gint status, gpointer data)
#endif
{
    DEBUG_FUNCTION ("child_exited_cb");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    tilda_term *tt = TILDA_TERM(data);
    gint index = gtk_notebook_page_num (GTK_NOTEBOOK(tt->tw->notebook), tt->hbox);

    /* Make sure we got a valid index */
    if (index == -1)
    {
        DEBUG_ERROR ("Bad notebook tab\n");
        return;
    }

    /* These can stay here. They don't need to go into a header because
     * they are only used at this point in the code. */
    enum command_exit { DROP_TO_DEFAULT_SHELL, RESTART_COMMAND, EXIT_TERMINAL };

    /* Check the user's preference for what to do when the child terminal
     * is closed. Take the appropriate action */
    switch (config_getint ("command_exit"))
    {
        case EXIT_TERMINAL:
            tilda_window_close_tab (tt->tw, index, FALSE);
            break;
        case RESTART_COMMAND:
            vte_terminal_feed (VTE_TERMINAL(tt->vte_term), "\r\n\r\n", 4);
            start_shell (tt, FALSE, NULL);
            break;
        case DROP_TO_DEFAULT_SHELL:
            start_shell (tt, TRUE, NULL);
        default:
            break;
    }
}

/**
 * tilda_term_config_defaults ()
 *
 * Read and set all of the defaults for this terminal from the current configuration.
 *
 * Success: return 0
 * Failure: return non-zero
 */
static gint tilda_term_config_defaults (tilda_term *tt)
{
    DEBUG_FUNCTION ("tilda_term_config_defaults");
    DEBUG_ASSERT (tt != NULL);
    GdkRGBA fg, bg, cc;
    gchar* word_chars;
    gint i;
    gint cursor_shape;

    /** Colors & Palette **/
    bg.red   =    GUINT16_TO_FLOAT(config_getint ("back_red"));
    bg.green =    GUINT16_TO_FLOAT(config_getint ("back_green"));
    bg.blue  =    GUINT16_TO_FLOAT(config_getint ("back_blue"));
#ifdef VTE_290
    bg.alpha =    1.0;
    gdouble transparency_level = 0.0;
#else
    bg.alpha =    (config_getbool("enable_transparency") ? GUINT16_TO_FLOAT(config_getint ("back_alpha")) : 1.0);
#endif

    fg.red   =    GUINT16_TO_FLOAT(config_getint ("text_red"));
    fg.green =    GUINT16_TO_FLOAT(config_getint ("text_green"));
    fg.blue  =    GUINT16_TO_FLOAT(config_getint ("text_blue"));
    fg.alpha =    1.0;

    cc.red   =    GUINT16_TO_FLOAT(config_getint ("cursor_red"));
    cc.green =    GUINT16_TO_FLOAT(config_getint ("cursor_green"));
    cc.blue  =    GUINT16_TO_FLOAT(config_getint ("cursor_blue"));
    cc.alpha = 1.0;

    for(i = 0;i < TERMINAL_PALETTE_SIZE; i++) {
        current_palette[i].red   = GUINT16_TO_FLOAT(config_getnint ("palette", i*3));
        current_palette[i].green = GUINT16_TO_FLOAT(config_getnint ("palette", i*3+1));
        current_palette[i].blue  = GUINT16_TO_FLOAT(config_getnint ("palette", i*3+2));
        current_palette[i].alpha = 1.0;
    }

    vte_terminal_set_colors_rgba (VTE_TERMINAL(tt->vte_term), &fg, &bg, current_palette, TERMINAL_PALETTE_SIZE);

    /** Bells **/
    vte_terminal_set_audible_bell (VTE_TERMINAL(tt->vte_term), config_getbool ("bell"));
#ifdef VTE_290
    vte_terminal_set_visible_bell (VTE_TERMINAL(tt->vte_term), config_getbool ("bell"));
#endif
    /** Cursor **/
    vte_terminal_set_cursor_blink_mode (VTE_TERMINAL(tt->vte_term),
            (config_getbool ("blinks"))?VTE_CURSOR_BLINK_ON:VTE_CURSOR_BLINK_OFF);
    vte_terminal_set_color_cursor_rgba (VTE_TERMINAL(tt->vte_term), &cc);

    cursor_shape = config_getint("cursor_shape");
    if (cursor_shape < 0 || cursor_shape > 2) {
        config_setint("cursor_shape", 0);
        cursor_shape = 0;
    }
    vte_terminal_set_cursor_shape(VTE_TERMINAL(tt->vte_term), (VteTerminalCursorShape)cursor_shape);

    /** Scrolling **/
#ifdef VTE_290
    vte_terminal_set_scroll_background (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_background"));
#endif
    vte_terminal_set_scroll_on_output (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_on_output"));
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_on_key"));

    /** Mouse **/
    vte_terminal_set_mouse_autohide (VTE_TERMINAL(tt->vte_term), FALSE); /* TODO: make this configurable */

    /** Text Properties **/
    vte_terminal_set_allow_bold (VTE_TERMINAL(tt->vte_term), config_getbool ("bold"));
    gtk_widget_set_double_buffered (tt->vte_term, config_getbool("double_buffer"));
    PangoFontDescription *description =
        pango_font_description_from_string (config_getstr ("font"));
    vte_terminal_set_font (VTE_TERMINAL (tt->vte_term), description);

    /** Scrollback **/
    vte_terminal_set_scrollback_lines (VTE_TERMINAL(tt->vte_term), config_getbool("scroll_history_infinite") ? -1 : config_getint ("lines"));

    /** Keys **/
    switch (config_getint ("backspace_key"))
    {
        case ASCII_DELETE:
            vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_ASCII_DELETE);
            break;
        case DELETE_SEQUENCE:
            vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_DELETE_SEQUENCE);
            break;
        case ASCII_BACKSPACE:
            vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_ASCII_BACKSPACE);
            break;
        case AUTO:
        default:
            vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_AUTO);
            break;
    }

    switch (config_getint ("delete_key"))
    {
        case ASCII_DELETE:
            vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_ASCII_DELETE);
            break;
        case DELETE_SEQUENCE:
            vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_DELETE_SEQUENCE);
            break;
        case ASCII_BACKSPACE:
            vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_ASCII_BACKSPACE);
            break;
        case AUTO:
        default:
            vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_AUTO);
            break;
    }

    /** Word chars **/
    word_chars =  config_getstr ("word_chars");
    if (NULL == word_chars || '\0' == *word_chars) {
        word_chars = DEFAULT_WORD_CHARS;
    }

    /**
     * The word_chars feature was removed from VTE in 38.x and reintroduced with a different API function in VTE
     * 40.x, so we need the following compile guards to ensure we only include the word_chars feature in the
     * supported versions.
     **/

#ifdef VTE_290 /* VTE 2.90 only */
    vte_terminal_set_word_chars (VTE_TERMINAL(tt->vte_term), word_chars);

    /** Background **/
    if (config_getbool ("use_image"))
        vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), config_getstr ("image"));
    else
        vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), NULL);
    transparency_level = ((gdouble) config_getint ("transparency"))/100;

    if (config_getbool ("enable_transparency") && transparency_level > 0)
    {
        vte_terminal_set_background_saturation (VTE_TERMINAL (tt->vte_term), transparency_level);
        vte_terminal_set_opacity (VTE_TERMINAL (tt->vte_term), (1.0 - transparency_level) * 0xffff);
        vte_terminal_set_background_transparent (VTE_TERMINAL(tt->vte_term), !tt->tw->have_argb_visual);
    }
#else
    #if VTE_MINOR_VERSION >= 40
        vte_terminal_set_word_char_exceptions (VTE_TERMINAL (tt->vte_term), word_chars);
    #endif
#endif
    return 0;
}

static void
menu_copy_cb (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_copy_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_term *tt = TILDA_TERM(user_data);

    vte_terminal_copy_clipboard (VTE_TERMINAL (tt->vte_term));
}

static void
menu_paste_cb (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_paste_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_term *tt = TILDA_TERM(user_data);

    vte_terminal_paste_clipboard (VTE_TERMINAL (tt->vte_term));
}


static void
menu_preferences_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_config_cb");
    DEBUG_ASSERT (user_data != NULL);

    /* Show the config wizard */
    wizard (TILDA_WINDOW(user_data));
}

static void
menu_quit_cb (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_quit_cb");

    gtk_main_quit ();
}

static void
menu_add_tab_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_add_tab_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_window_add_tab (TILDA_WINDOW(user_data));
}

static void
menu_fullscreen_cb (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_fullscreen_cb");
    DEBUG_ASSERT (user_data != NULL);

    toggle_fullscreen_cb (TILDA_WINDOW(user_data));
}

static void
menu_searchbar_cb(GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_fullscreen_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_window_toggle_searchbar (TILDA_WINDOW(user_data));
}

static void
menu_close_tab_cb (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_close_tab_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_window_close_current_tab (TILDA_WINDOW(user_data));
}

static void on_popup_hide (GtkWidget *widget, tilda_window *tw)
{
    DEBUG_FUNCTION("on_popup_hide");
    DEBUG_ASSERT(widget != NULL);
    DEBUG_ASSERT(tw != NULL);

    tw->disable_auto_hide = FALSE;
}

static void popup_menu (tilda_window *tw, tilda_term *tt)
{
    DEBUG_FUNCTION ("popup_menu");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tt != NULL);

    GtkBuilder *builder = gtk_builder_new();

    gtk_builder_add_from_resource (builder, "/org/tilda/menu.ui", NULL);

    /* Create the action group */
    GSimpleActionGroup *action_group = g_simple_action_group_new();

    /* We need two different lists of entries because the
     * because the actions have different scope, some concern the
     * tilda_window and others concern the current terminal, so
     * when we add them to the action_group we need to pass different
     * user_data (tw or tt).
     *
     * Note: Using designated initializers here, allows us to skip the remaining fields which are NULL anyway and
     * also gets rid of missing field initializer warnings.
     */
    GActionEntry entries_for_tilda_window[] = {
        { .name="new-tab", menu_add_tab_cb },
        { .name="close-tab", menu_close_tab_cb },
        { .name="fullscreen", menu_fullscreen_cb },
        { .name="searchbar", menu_searchbar_cb },
        { .name="preferences", menu_preferences_cb },
        { .name="quit", menu_quit_cb }
    };

    GActionEntry entries_for_tilda_terminal[] = {
        { .name="copy", menu_copy_cb},
        { .name="paste", menu_paste_cb}
    };

    g_action_map_add_action_entries(G_ACTION_MAP(action_group),
            entries_for_tilda_window, G_N_ELEMENTS(entries_for_tilda_window), tw);
    g_action_map_add_action_entries(G_ACTION_MAP(action_group),
            entries_for_tilda_terminal, G_N_ELEMENTS(entries_for_tilda_terminal), tt);

    gtk_widget_insert_action_group(tw->window, "window", G_ACTION_GROUP(action_group));

    GMenuModel *menu_model = G_MENU_MODEL(gtk_builder_get_object(builder, "menu"));
    GtkWidget *menu = gtk_menu_new_from_model(menu_model);
    gtk_menu_attach_to_widget(GTK_MENU(menu), tw->window, NULL);

    gtk_menu_set_accel_group(GTK_MENU(menu), tw->accel_group);
    gtk_menu_set_accel_path(GTK_MENU(menu), "<tilda>/context");

    /* Disable auto hide */
    tw->disable_auto_hide = TRUE;
    g_signal_connect (G_OBJECT(menu), "unmap", G_CALLBACK(on_popup_hide), tw);

    /* Display the menu */
    gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
}

static int button_press_cb (G_GNUC_UNUSED GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    DEBUG_FUNCTION ("button_press_cb");
    DEBUG_ASSERT (data != NULL);

    VteTerminal *terminal;
    tilda_term *tt;
    gchar *match;
    gint tag;
    gint ypad;
    gchar *cmd;
    gchar *web_browser_cmd;
    gboolean ret = FALSE;

    tt = TILDA_TERM(data);

    switch (event->button)
    {
        case 9:
            tilda_window_next_tab (tt->tw);
            break;
        case 8:
            tilda_window_prev_tab (tt->tw);
            break;
        case 3: /* Right Click */
            popup_menu (tt->tw, tt);
            break;
        case 2: /* Middle Click */
            break;
        case 1: /* Left Click */
            terminal  = VTE_TERMINAL(tt->vte_term);
            ypad = gtk_widget_get_margin_bottom(GTK_WIDGET(terminal));

            glong column = (glong) ((event->x - ypad) /
                                    vte_terminal_get_char_width (terminal));
            glong row = (glong) ((event->y - ypad) /
                                 vte_terminal_get_char_height (terminal));

            match = vte_terminal_match_check (terminal,
                                              column,
                                              row,
                                              &tag);

            /* Check if we can launch a web browser, and do so if possible */
            if (match != NULL)
            {
#if DEBUG
                g_print ("Got a Left Click -- Matched: `%s' (%d)\n", match, tag);
#endif
                web_browser_cmd = g_strescape (config_getstr ("web_browser"), NULL);
                cmd = g_strdup_printf ("%s %s", web_browser_cmd, match);
#if DEBUG
                g_print ("Launching command: `%s'\n", cmd);
#endif
                ret = g_spawn_command_line_async(cmd, NULL);

                /* Check that the command launched */
                if (!ret)
                {
                    g_printerr (_("Failed to launch the web browser. The command was `%s'\n"), cmd);
                    TILDA_PERROR ();
                }

                g_free (web_browser_cmd);
                g_free (cmd);
                g_free (match);
            }

            break;
        default:
            break;
    }

    return FALSE;
}

gboolean key_press_cb (GtkWidget *widget,
                       GdkEvent  *event,
                       tilda_term *terminal)
{
    if(event->type == GDK_KEY_PRESS) {
        GdkEventKey *keyevent = (GdkEventKey*) event;
        if(keyevent->keyval == GDK_KEY_Menu) {
            popup_menu(terminal->tw, terminal);
        }
    }
    return GDK_EVENT_PROPAGATE;
}

/* vim: set ts=4 sts=4 sw=4 expandtab: */
