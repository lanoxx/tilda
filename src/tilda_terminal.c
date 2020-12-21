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

#include "tilda_terminal.h"

#include "config.h"
#include "configsys.h"
#include "debug.h"
#include "tilda.h"
#include "tilda-context-menu.h"
#include "tilda-url-spawner.h"
#include "tilda_window.h"

#include <stdio.h>
#include <stdlib.h> /* malloc */
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <string.h>

static void start_shell (tilda_term *tt, gboolean ignore_custom_command);
static void start_default_shell (tilda_term *tt);

static gint tilda_term_config_defaults (tilda_term *tt);

static void child_exited_cb (GtkWidget *widget, gint status, gpointer data);
static void window_title_changed_cb (GtkWidget *widget, gpointer data);
static gboolean button_press_cb (GtkWidget *widget, GdkEvent *event, tilda_term *terminal);
static gboolean key_press_cb (GtkWidget *widget, GdkEvent  *event, tilda_term *terminal);
static void handle_left_button_click (GtkWidget * window,
                                      GdkEventButton * button_event,
                                      const char * link,
                                      TildaMatchRegistryEntry * entry);
static void iconify_window_cb (GtkWidget *widget, gpointer data);
static void deiconify_window_cb (GtkWidget *widget, gpointer data);
static void raise_window_cb (GtkWidget *widget, gpointer data);
static void lower_window_cb (GtkWidget *widget, gpointer data);
static void maximize_window_cb (GtkWidget *widget, gpointer data);
static void restore_window_cb (GtkWidget *widget, gpointer data);
static void refresh_window_cb (GtkWidget *widget, gpointer data);
static void move_window_cb (GtkWidget *widget, guint x, guint y, gpointer data);

static gchar *get_default_command (void);
gchar *get_working_directory (tilda_term *terminal);

static void handle_gdk_event (G_GNUC_UNUSED GtkWidget *widget,
                              GdkEvent *event,
                              tilda_term *tt);

gint tilda_term_free (tilda_term *term)
{
    DEBUG_FUNCTION ("tilda_term_free");
    DEBUG_ASSERT (term != NULL);

    g_free (term->initial_working_dir);

    g_signal_handlers_disconnect_by_func (term->vte_term, child_exited_cb, term);

    g_clear_object (&term->hbox);
    g_clear_object (&term->scrollbar);
    g_clear_object (&term->vte_term);

    if (term->http_regexp != NULL) {
        g_regex_unref (term->http_regexp);
    }

    if (term->vte_regexp != NULL) {
        vte_regex_unref (term->vte_regexp);
    }

    term->http_regexp = NULL;
    term->vte_regexp = NULL;

    if (term->registry) {
        tilda_match_registry_free(term->registry);
        term->registry = NULL;
    }

    g_free (term);

    return 0;
}

static gboolean check_flavor_enabled (TildaMatchRegistryFlavor flavor) {

    switch (flavor) {
        case TILDA_MATCH_FLAVOR_EMAIL:
            return config_getbool("match_email_addresses");
        case TILDA_MATCH_FLAVOR_NUMBER:
            return config_getbool("match_numbers");
        case TILDA_MATCH_FLAVOR_FILE:
            return config_getbool("match_file_uris");
        case TILDA_MATCH_FLAVOR_URL:
        case TILDA_MATCH_FLAVOR_DEFAULT_TO_HTTP:
            return config_getbool("match_web_uris");
        default:
            g_assert_not_reached();
    }
}

static int
register_match (VteRegex * regex,
                TildaMatchRegistryFlavor flavor,
                gpointer user_data)
{
    tilda_term * term = user_data;
    int tag;

    gboolean flavor_enabled = FALSE;

    flavor_enabled = check_flavor_enabled(flavor);

    if (!flavor_enabled) {
        return TILDA_MATCH_REGISTRY_IGNORE;
    }

    tag = vte_terminal_match_add_regex (VTE_TERMINAL (term->vte_term), regex, 0);

    vte_terminal_match_set_cursor_name (VTE_TERMINAL (term->vte_term), tag, "pointer");

    return tag;
}

struct tilda_term_ *tilda_term_init (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("tilda_term_init");
    DEBUG_ASSERT (tw != NULL);

    struct tilda_term_ *term;
    tilda_term *current_tt;
    gint current_tt_index;

    term = g_new0 (tilda_term, 1);

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
    g_object_ref (term->hbox);

    /* Create the terminal */
    term->vte_term = vte_terminal_new ();
    g_object_ref (term->vte_term);

    /* Create the scrollbar for the terminal */
    term->scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL,
        gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (VTE_TERMINAL(term->vte_term))));
    g_object_ref (term->scrollbar);

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

    TildaMatchRegistry * registry = tilda_match_registry_new ();
    term->registry = registry;

    tilda_match_registry_for_each (term->registry, register_match, term);

    /* Show the child widgets */
    gtk_widget_show (term->vte_term);
    gtk_widget_show (term->hbox);

    /* Get current term's working directory */
    current_tt_index = gtk_notebook_get_current_page (GTK_NOTEBOOK(tw->notebook));
    current_tt = g_list_nth_data (tw->terms, current_tt_index);
    if (current_tt != NULL)
    {
        term->initial_working_dir = tilda_term_get_cwd (current_tt);
    }

    /* Fork the appropriate command into the terminal */
    start_shell (term, FALSE);

    return term;
}

void tilda_terminal_update_matches (tilda_term *tt) {

    vte_terminal_match_remove_all (VTE_TERMINAL (tt->vte_term));

    tilda_match_registry_for_each (tt->registry, register_match, tt);
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
    gchar * title = tilda_terminal_get_title (tt);
    gchar * full_title = tilda_terminal_get_full_title (tt);
    GtkWidget *label;

    label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (tt->tw->notebook), tt->hbox);

    /* We need to check if the widget that received the title change is the currently
     * active tab. If not we should not update the window title. */
    gint page = gtk_notebook_get_current_page (GTK_NOTEBOOK (tt->tw->notebook));

    gboolean active;
    if (page >= 0) {
        tilda_term *currente_term;
        currente_term = g_list_nth_data (tt->tw->terms, (guint) page);
        active = widget == currente_term->vte_term;
    } else {
        active = TRUE;
    }

    gtk_label_set_text (GTK_LABEL(label), title);

    if (active) {
        gtk_window_set_title (GTK_WINDOW (tt->tw->window), title);
    }

    if(config_getbool("show_title_tooltip"))
      gtk_widget_set_tooltip_text(label, full_title);
    else
      gtk_widget_set_tooltip_text(label, "");

    g_free (title);
}

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

static void
shell_spawned_cb (VteTerminal *terminal,
                  GPid pid,
                  GError *error,
                  gpointer user_data)
{
    DEBUG_FUNCTION ("shell_spawned_cb");

    tilda_term *tt;

    tt = user_data;

    if (error)
    {
        if (tt->dropped_to_default_shell)
        {
            g_printerr (_("Unable to launch default shell: %s\n"), get_default_command ());
        } else {
            g_printerr (_("Unable to launch custom command: %s\n"), config_getstr ("command"));
            g_printerr (_("Launching custom command failed with error: %s\n"), error->message);
            g_printerr (_("Launching default shell instead\n"));

            start_default_shell (tt);
            tt->dropped_to_default_shell = TRUE;
        }

        return;
    }

    tt->pid = pid;
}

/* Fork a shell into the VTE Terminal
 *
 * @param tt the tilda_term to fork into
 */
static void start_shell (tilda_term *tt, gboolean ignore_custom_command)
{
    DEBUG_FUNCTION ("start_shell");
    DEBUG_ASSERT (tt != NULL);

    gint ret;
    gint argc;
    gchar **argv;
    GError *error = NULL;

    if (config_getbool ("run_command") && !ignore_custom_command)
    {
        ret = g_shell_parse_argv (config_getstr ("command"), &argc, &argv, &error);

        /* Check for error */
        if (ret == FALSE)
        {
            g_printerr (_("Problem parsing custom command: %s\n"), error->message);
            g_printerr (_("Launching default shell instead\n"));

            g_error_free (error);

            start_default_shell (tt);
        }

        gchar *working_dir = get_working_directory (tt);
        gint command_timeout = config_getint ("command_timeout_ms");

        char **envv = malloc(2*sizeof(void *));
        char *path_value = getenv("PATH");

        gchar *path_prefixed = g_strconcat("PATH=", path_value, NULL);

        envv[0] = path_prefixed;
        envv[1] = NULL;

        vte_terminal_spawn_async (VTE_TERMINAL (tt->vte_term),
                                  VTE_PTY_DEFAULT, /* VtePtyFlags pty_flags */
                                  working_dir, /* const char *working_directory */
                                  argv, /* char **argv */
                                  envv, /* char **envv */
                                  G_SPAWN_SEARCH_PATH,    /* GSpawnFlags spawn_flags */
                                  NULL, /* GSpawnChildSetupFunc child_setup */
                                  NULL, /* gpointer child_setup_data */
                                  NULL, /* GDestroyNotify child_setup_data_destroy */
                                  command_timeout, /* timeout in ms */
                                  NULL, /* GCancellable * cancellable, */
                                  shell_spawned_cb,  /* VteTerminalSpawnAsyncCallback callback */
                                  tt);   /* user_data */

        g_strfreev (argv);
        g_free (envv);
        g_free(path_prefixed);
    } else {
        start_default_shell (tt);
    }
}

static void
start_default_shell (tilda_term *tt)
{
    gchar  **argv;


    /* If we have dropped to the default shell before, then this time, we
     * do not spawn a new shell, but instead close the current shell. This will
     * cause the current tab to close.
     */
    if (tt->dropped_to_default_shell) {
        gint index = gtk_notebook_page_num (GTK_NOTEBOOK(tt->tw->notebook),
            tt->hbox);
        tilda_window_close_tab (tt->tw, index, FALSE);

        return;
    }

    gchar *default_command = get_default_command ();
    gchar *working_dir = get_working_directory (tt);
    gint command_timeout = config_getint ("command_timeout_ms");

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

    vte_terminal_spawn_async (VTE_TERMINAL (tt->vte_term),
                              VTE_PTY_DEFAULT, /* VtePtyFlags pty_flags */
                              working_dir, /* const char *working_directory */
                              argv, /* char **argv */
                              NULL, /* char **envv */
                              flags,    /* GSpawnFlags spawn_flags */
                              NULL, /* GSpawnChildSetupFunc child_setup */
                              NULL, /* gpointer child_setup_data */
                              NULL, /* GDestroyNotify child_setup_data_destroy */
                              command_timeout, /* timeout in ms */
                              NULL, /* GCancellable * cancellable, */
                              shell_spawned_cb,  /* VteTerminalSpawnAsyncCallback callback */
                              tt);   /* user_data */

    g_free(argv1);
    g_free (argv);
}

gchar *
get_default_command ()
{
    /* No custom command, get it from the environment */
    gchar *default_command = (gchar *) g_getenv ("SHELL");

    /* Check for error */
    if (default_command == NULL)
        default_command = "/bin/sh";

    return default_command;
}

gchar *get_working_directory (tilda_term *terminal)
{
    gchar *working_dir;

    working_dir = terminal->initial_working_dir;

    if (working_dir == NULL || config_getbool ("inherit_working_dir") == FALSE)
    {
        working_dir = config_getstr ("working_dir");
    }

    return working_dir;
}

static void child_exited_cb (GtkWidget *widget, gint status, gpointer data)
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
            start_shell (tt, FALSE);
            break;
        case DROP_TO_DEFAULT_SHELL:
            tt->initial_working_dir = NULL;
            start_default_shell (tt);
            tt->dropped_to_default_shell = TRUE;
            break;
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
    GdkRGBA *current_palette;
    gchar* word_chars;
    gint cursor_shape;

    /** Colors & Palette **/
    bg.red   =    GUINT16_TO_FLOAT(config_getint ("back_red"));
    bg.green =    GUINT16_TO_FLOAT(config_getint ("back_green"));
    bg.blue  =    GUINT16_TO_FLOAT(config_getint ("back_blue"));

    bg.alpha =    (config_getbool("enable_transparency") ? GUINT16_TO_FLOAT(config_getint ("back_alpha")) : 1.0);

    fg.red   =    GUINT16_TO_FLOAT(config_getint ("text_red"));
    fg.green =    GUINT16_TO_FLOAT(config_getint ("text_green"));
    fg.blue  =    GUINT16_TO_FLOAT(config_getint ("text_blue"));
    fg.alpha =    1.0;

    cc.red   =    GUINT16_TO_FLOAT(config_getint ("cursor_red"));
    cc.green =    GUINT16_TO_FLOAT(config_getint ("cursor_green"));
    cc.blue  =    GUINT16_TO_FLOAT(config_getint ("cursor_blue"));
    cc.alpha = 1.0;

    current_palette = tilda_palettes_get_current_palette ();

    for(guint i = 0; i < TILDA_COLOR_PALETTE_SIZE; i++) {
        current_palette[i].red   = GUINT16_TO_FLOAT(config_getnint ("palette", i*3));
        current_palette[i].green = GUINT16_TO_FLOAT(config_getnint ("palette", i*3+1));
        current_palette[i].blue  = GUINT16_TO_FLOAT(config_getnint ("palette", i*3+2));
        current_palette[i].alpha = 1.0;
    }

    vte_terminal_set_colors (VTE_TERMINAL(tt->vte_term),
                             &fg,
                             &bg,
                             current_palette,
                             TILDA_COLOR_PALETTE_SIZE);

    /** Bells **/
    vte_terminal_set_audible_bell (VTE_TERMINAL(tt->vte_term), config_getbool ("bell"));

    /** Cursor **/
    vte_terminal_set_cursor_blink_mode (VTE_TERMINAL(tt->vte_term),
            (config_getbool ("blinks"))?VTE_CURSOR_BLINK_ON:VTE_CURSOR_BLINK_OFF);
    vte_terminal_set_color_cursor (VTE_TERMINAL(tt->vte_term), &cc);
    vte_terminal_set_color_cursor_foreground (VTE_TERMINAL(tt->vte_term), &bg);

    cursor_shape = config_getint("cursor_shape");
    if (cursor_shape < 0 || cursor_shape > 2) {
        config_setint("cursor_shape", 0);
        cursor_shape = 0;
    }
    vte_terminal_set_cursor_shape(VTE_TERMINAL(tt->vte_term),
                                  (VteCursorShape) cursor_shape);

    /** Scrolling **/
    vte_terminal_set_scroll_on_output (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_on_output"));
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_on_key"));

    /** Mouse **/
    vte_terminal_set_mouse_autohide (VTE_TERMINAL(tt->vte_term), FALSE); /* TODO: make this configurable */

    vte_terminal_set_allow_hyperlink(VTE_TERMINAL(tt->vte_term), TRUE);

    /** Text Properties **/
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

    vte_terminal_set_word_char_exceptions (VTE_TERMINAL (tt->vte_term), word_chars);

    return 0;
}

static gboolean
button_press_cb (GtkWidget *widget, GdkEvent *event, tilda_term * terminal)
{
    DEBUG_FUNCTION ("button_press_cb");
    DEBUG_ASSERT (terminal != NULL);

    handle_gdk_event (widget, event, terminal);

    return GDK_EVENT_PROPAGATE;
}

static gboolean
key_press_cb (GtkWidget *widget,
              GdkEvent  *event,
              tilda_term *terminal)
{
    DEBUG_ASSERT (terminal != NULL);

    handle_gdk_event (widget, event, terminal);

    return GDK_EVENT_PROPAGATE;
}

static void
handle_gdk_event (G_GNUC_UNUSED GtkWidget *widget,
                  GdkEvent *event,
                  tilda_term *tt)
{
    VteTerminal *terminal;
    gchar *match;
    TildaMatchRegistryEntry * match_entry;
    gchar *hyperlink;
    gint tag;
    GtkWidget * window;

    window = tt->tw->window;

    terminal  = VTE_TERMINAL(tt->vte_term);

    // see https://gist.github.com/egmontkob/eb114294efbcd5adb1944c9f3cb5feda
    // for more details about OSC 8 hyperlinks.
    hyperlink = vte_terminal_hyperlink_check_event (terminal, event);

    if (hyperlink != NULL) {
        match = hyperlink;
        match_entry = tilda_match_registry_get_hyperlink_entry (tt->registry);
    } else {
        match = vte_terminal_match_check_event (terminal,
                                                event,
                                                &tag);

        match_entry = tilda_match_registry_lookup_by_tag (tt->registry,
                                                          tag);
    }

    if (event->type == GDK_BUTTON_PRESS)
    {
        GdkEventButton * button_event = (GdkEventButton *) event;
        switch (button_event->button)
        {
            case 9:
                tilda_window_next_tab (tt->tw);
                break;
            case 8:
                tilda_window_prev_tab (tt->tw);
                break;
            case 3: /* Right Click */
            {
                GtkWidget * menu = tilda_context_menu_popup (tt->tw, tt, match, match_entry);
                gtk_menu_popup_at_pointer (GTK_MENU (menu), event);
                break;
            }
            case 2: /* Middle Click */
                break;
            case 1: /* Left Click */
                handle_left_button_click (window, button_event, match, match_entry);
                break;
            default:
                break;
        }
    }
    else if(event->type == GDK_KEY_PRESS)
    {
        GdkEventKey *keyevent = (GdkEventKey*) event;
        if(keyevent->keyval == GDK_KEY_Menu) {
            GtkWidget * menu = tilda_context_menu_popup (tt->tw, tt, match, match_entry);
            gtk_menu_popup_at_pointer (GTK_MENU (menu), event);
        }
    }

    g_free (match);
}

static void handle_left_button_click (GtkWidget * window,
                                      GdkEventButton * button_event,
                                      const char *link,
                                      TildaMatchRegistryEntry * match_entry)
{
    gboolean activate_with_control = config_getbool("control_activates_match");

    if (!activate_with_control || button_event->state & GDK_CONTROL_MASK) {
        /* Check if we can open the matched token, and do so if possible */
        tilda_url_spawner_spawn_browser_for_match (GTK_WINDOW (window),
                                                   link,
                                                   match_entry);
    } else {
        g_debug ("Match activation skipped.");
    }
}

gchar * tilda_terminal_get_full_title (tilda_term *tt)
{
    DEBUG_FUNCTION ("tilda_terminal_get_title");

    const gchar *vte_title;
    gchar *window_title;
    gchar *initial;
    gchar *title;

    vte_title = vte_terminal_get_window_title (VTE_TERMINAL (tt->vte_term));
    window_title = g_strdup (vte_title);
    initial = g_strdup (config_getstr ("title"));

    /* These are not needed anywhere else. If they ever are, move them to a header file */
    enum d_set_title { NOT_DISPLAYED, AFTER_INITIAL, BEFORE_INITIAL, REPLACE_INITIAL };

    switch (config_getint ("d_set_title"))
    {
        case REPLACE_INITIAL:
            title = (window_title != NULL) ? g_strdup (window_title)
                                           : g_strdup (_("Untitled"));
            break;

        case BEFORE_INITIAL:
            title = (window_title != NULL) ? g_strdup_printf ("%s - %s", window_title, initial)
                                           : g_strdup (initial);
            break;

        case AFTER_INITIAL:
            title = (window_title != NULL) ? g_strdup_printf ("%s - %s", initial, window_title)
                                           : g_strdup (initial);
            break;

        case NOT_DISPLAYED:
            title = g_strdup (initial);
            break;

        default:
            g_printerr (_("Bad value for \"d_set_title\" in config file\n"));
            title = g_strdup ("");
            break;
    }

    g_free (window_title);
    g_free (initial);

    return title;
}

gchar *tilda_terminal_get_title (tilda_term *tt)
{
    DEBUG_FUNCTION ("tilda_terminal_get_title");
    DEBUG_ASSERT (tt != NULL);

    gchar * title;
    gchar * final_title;

    title = tilda_terminal_get_full_title (tt);

    /* These are not needed anywhere else. If they ever are, move them to a header file */
    enum { SHOW_FULL_TITLE, SHOW_FIRST_N_CHARS, SHOW_LAST_N_CHARS };

    guint max_length = (guint) config_getint ("title_max_length");
    guint title_behaviour = config_getint("title_behaviour");

    if (strlen (title) > max_length) {
        if (title_behaviour == SHOW_FULL_TITLE) {
            final_title = g_strdup (title);
        } else if (title_behaviour == SHOW_FIRST_N_CHARS) {
            final_title = g_strdup_printf ("%.*s...", max_length, title);
        } else if (title_behaviour == SHOW_LAST_N_CHARS) {
            gchar *titleOffset = title + strlen(title) - max_length;
            final_title = g_strdup_printf ("...%s", titleOffset);
        } else {
            g_printerr ("Bad value for \"title_behaviour\" in config file.\n");
            final_title = g_strdup (title);
        }
    } else {
        final_title = g_strdup (title);
    }

    g_free (title);

    return final_title;
}
