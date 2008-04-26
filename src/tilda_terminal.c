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
#include <tilda_window.h>
#include <tilda_terminal.h>
#include <callback_func.h>
#include <configsys.h>
#include <translation.h>
#include <wizard.h> /* wizard */

#include <stdio.h>
#include <stdlib.h> /* malloc */
#include <gtk/gtk.h>
#include <glib-object.h>
#include <vte/vte.h>
#include <string.h>


#define DINGUS1 "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?"
#define DINGUS2 "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?/[-A-Za-z0-9_\\$\\.\\+\\!\\*\\(\\),;:@&=\\?/~\\#\\%]*[^]'\\.}>\\) ,\\\"]"



static gint start_shell (struct tilda_term_ *tt);
static gint tilda_term_config_defaults (tilda_term *tt);
static void child_exited_cb (GtkWidget *widget, gpointer data);
static void window_title_changed_cb (GtkWidget *widget, gpointer data);
static void status_line_changed_cb (GtkWidget *widget, gpointer data);
static int button_press_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
static void status_line_changed_cb (GtkWidget *widget, gpointer data);
static void iconify_window_cb (GtkWidget *widget, gpointer data);
static void deiconify_window_cb (GtkWidget *widget, gpointer data);
static void raise_window_cb (GtkWidget *widget, gpointer data);
static void lower_window_cb (GtkWidget *widget, gpointer data);
static void maximize_window_cb (GtkWidget *widget, gpointer data);
static void restore_window_cb (GtkWidget *widget, gpointer data);
static void refresh_window_cb (GtkWidget *widget, gpointer data);
static void move_window_cb (GtkWidget *widget, guint x, guint y, gpointer data);
static void increase_font_size_cb (GtkWidget *widget, gpointer data);
static void decrease_font_size_cb (GtkWidget *widget, gpointer data);


gint tilda_term_free (struct tilda_term_ *term)
{
    DEBUG_FUNCTION ("tilda_term_free");
    DEBUG_ASSERT (term != NULL);

    g_free (term);

    return 0;
}



struct tilda_term_ *tilda_term_init (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("tilda_term_init");
    DEBUG_ASSERT (tw != NULL);

    int ret;
    struct tilda_term_ *term;

    term = g_malloc (sizeof (struct tilda_term_));

    /* Check for a failed allocation */
    if (!term)
        return NULL;

    /* Add the parent window reference */
    term->tw = tw;

    /* Create a non-homogenous hbox, with 0px spacing between members */
    term->hbox = gtk_hbox_new (FALSE, 0);

    /* Create the terminal */
    term->vte_term = vte_terminal_new ();

    /* Create the scrollbar for the terminal */
    term->scrollbar = gtk_vscrollbar_new (VTE_TERMINAL(term->vte_term)->adjustment);

    /* Set properties of the terminal */
    tilda_term_config_defaults (term);

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
    g_signal_connect (G_OBJECT(term->vte_term), "eof",
                      G_CALLBACK(child_exited_cb), term);
    g_signal_connect (G_OBJECT(term->vte_term), "status-line-changed",
                      G_CALLBACK(status_line_changed_cb), term);
    g_signal_connect (G_OBJECT(term->vte_term), "button-press-event",
                      G_CALLBACK(button_press_cb), term);

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

    /* Connect to font tweakage. */
    g_signal_connect (G_OBJECT(term->vte_term), "increase-font-size",
                      G_CALLBACK(increase_font_size_cb), tw->window);
    g_signal_connect (G_OBJECT(term->vte_term), "decrease-font-size",
                      G_CALLBACK(decrease_font_size_cb), tw->window);

    /* Match URL's, etc */
    ret = vte_terminal_match_add (VTE_TERMINAL(term->vte_term), DINGUS1);
    vte_terminal_match_set_cursor_type (VTE_TERMINAL(term->vte_term), ret, GDK_HAND2);
    ret = vte_terminal_match_add(VTE_TERMINAL (term->vte_term), DINGUS2);
    vte_terminal_match_set_cursor_type (VTE_TERMINAL(term->vte_term), ret, GDK_HAND2);

    /* Show the child widgets */
    gtk_widget_show (term->vte_term);
    gtk_widget_show (term->hbox);

    /* Fork the appropriate command into the terminal */
    ret = start_shell (term);

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

    switch (pos)
    {
        case LEFT:
            gtk_box_reorder_child (GTK_BOX(tt->hbox), tt->scrollbar, 0);
            gtk_widget_show (tt->scrollbar);
            break;

        case RIGHT:
            gtk_box_reorder_child (GTK_BOX(tt->hbox), tt->scrollbar, 1);
            gtk_widget_show (tt->scrollbar);
            break;

        case DISABLED:
        default:
            gtk_widget_hide (tt->scrollbar);
            break;
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
    gtk_label_set_text (GTK_LABEL(label), title);

    g_free (title);
}

static void status_line_changed_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("status_line_changed_cb");
    DEBUG_ASSERT (widget != NULL);

    g_print ("Status = `%s'.\n", vte_terminal_get_status_line (VTE_TERMINAL(widget)));
}

static void iconify_window_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("iconify_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if ((GTK_WIDGET(data))->window)
            gdk_window_iconify ((GTK_WIDGET(data))->window);
}

static void deiconify_window_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("deiconify_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if ((GTK_WIDGET(data))->window)
            gdk_window_deiconify ((GTK_WIDGET(data))->window);
}

static void raise_window_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("raise_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if ((GTK_WIDGET(data))->window)
            gdk_window_raise ((GTK_WIDGET(data))->window);
}

static void lower_window_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("lower_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if ((GTK_WIDGET(data))->window)
            gdk_window_lower ((GTK_WIDGET(data))->window);
}

static void maximize_window_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("maximize_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if ((GTK_WIDGET(data))->window)
            gdk_window_maximize ((GTK_WIDGET(data))->window);
}

static void restore_window_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("restore_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if ((GTK_WIDGET(data))->window)
            gdk_window_unmaximize ((GTK_WIDGET(data))->window);
}

static void refresh_window_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("refresh_window_cb");
    DEBUG_ASSERT (data != NULL);

    GdkRectangle rect;
    if (GTK_IS_WIDGET(data))
    {
        if ((GTK_WIDGET(data))->window)
        {
            rect.x = rect.y = 0;
            rect.width = (GTK_WIDGET(data))->allocation.width;
            rect.height = (GTK_WIDGET(data))->allocation.height;
            gdk_window_invalidate_rect ((GTK_WIDGET(data))->window, &rect, TRUE);
        }
    }
}

static void move_window_cb (GtkWidget *widget, guint x, guint y, gpointer data)
{
    DEBUG_FUNCTION ("move_window_cb");
    DEBUG_ASSERT (data != NULL);

    if (GTK_IS_WIDGET(data))
        if ((GTK_WIDGET(data))->window)
            gdk_window_move ((GTK_WIDGET(data))->window, x, y);
}

static void adjust_font_size (GtkWidget *widget, gpointer data, gint howmuch)
{
    DEBUG_FUNCTION ("adjust_font_size");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    VteTerminal *terminal;
    PangoFontDescription *desired;
    gint newsize;
    gint columns, rows, owidth, oheight;

    /* Read the screen dimensions in cells. */
    terminal = VTE_TERMINAL(widget);
    columns = terminal->column_count;
    rows = terminal->row_count;

    /* Take into account padding and border overhead. */
    gtk_window_get_size(GTK_WINDOW(data), &owidth, &oheight);
    owidth -= terminal->char_width * terminal->column_count;
    oheight -= terminal->char_height * terminal->row_count;

    /* Calculate the new font size. */
    desired = pango_font_description_copy (vte_terminal_get_font(terminal));
    newsize = pango_font_description_get_size (desired) / PANGO_SCALE;
    newsize += howmuch;
    pango_font_description_set_size (desired, CLAMP(newsize, 4, 144) * PANGO_SCALE);

    /* Change the font, then resize the window so that we have the same
     * number of rows and columns. */
    vte_terminal_set_font (terminal, desired);
    /*gtk_window_resize (GTK_WINDOW(data),
              columns * terminal->char_width + owidth,
              rows * terminal->char_height + oheight);*/

    pango_font_description_free (desired);
}

static void increase_font_size_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("increase_font_size");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    adjust_font_size (widget, data, 1);
}

static void decrease_font_size_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("decrease_font_size");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    adjust_font_size (widget, data, -1);
}

/* Fork a shell into the VTE Terminal
 *
 * @param tt the tilda_term to fork into
 *
 * SUCCESS: return 0
 * FAILURE: return non-zero
 */
static gint start_shell (struct tilda_term_ *tt)
{
    DEBUG_FUNCTION ("start_shell");
    DEBUG_ASSERT (tt != NULL);

    gint ret;
    gint argc;
    gchar **argv;
    GError *error = NULL;

    gchar *default_command;

    if (config_getbool ("run_command"))
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

        ret = vte_terminal_fork_command (VTE_TERMINAL(tt->vte_term),
                    argv[0], argv, NULL,
                    config_getstr ("working_dir"),
                    TRUE, TRUE, TRUE);

        g_strfreev (argv);

        /* Check for error */
        if (ret == -1)
        {
            g_printerr (_("Unable to launch custom command: %s\n"), config_getstr ("command"));
            g_printerr (_("Launching default shell instead\n"));

            goto launch_default_shell;
        }

        return 0; /* SUCCESS: the early way out */
    }

launch_default_shell:

    /* No custom command, get it from the environment */
    default_command = (gchar *) g_getenv ("SHELL");

    /* Check for error */
    if (default_command == NULL)
        default_command = "/bin/sh";

    ret = vte_terminal_fork_command (VTE_TERMINAL(tt->vte_term),
                default_command, NULL, NULL,
                config_getstr ("working_dir"),
                TRUE, TRUE, TRUE);

    if (ret == -1)
    {
        g_printerr (_("Unable to launch default shell: %s\n"), default_command);
        return ret;
    }

    return 0;
}


static void child_exited_cb (GtkWidget *widget, gpointer data)
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
    enum command_exit { HOLD_TERMINAL_OPEN, RESTART_COMMAND, EXIT_TERMINAL };

    /* Check the user's preference for what to do when the child terminal
     * is closed. Take the appropriate action */
    switch (config_getint ("command_exit"))
    {
        case EXIT_TERMINAL:
            tilda_window_close_tab (tt->tw, index);
            break;
        case RESTART_COMMAND:
            vte_terminal_feed (VTE_TERMINAL(tt->vte_term), "\r\n\r\n", 4);
            start_shell (tt);
            break;
        case HOLD_TERMINAL_OPEN:
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

    gdouble transparency_level = 0.0;
    GdkColor fg, bg /*, tint, highlight, cursor, black */;


    /** Colors **/
    bg.red   =    config_getint ("back_red");
    bg.green =    config_getint ("back_green");
    bg.blue  =    config_getint ("back_blue");

    fg.red   =    config_getint ("text_red");
    fg.green =    config_getint ("text_green");
    fg.blue  =    config_getint ("text_blue");

    vte_terminal_set_colors (VTE_TERMINAL(tt->vte_term), &fg, &bg, NULL, 0);

    /** Bells **/
    vte_terminal_set_audible_bell (VTE_TERMINAL(tt->vte_term), config_getbool ("bell"));
    vte_terminal_set_visible_bell (VTE_TERMINAL(tt->vte_term), config_getbool ("bell"));

    /** Cursor **/
    vte_terminal_set_cursor_blinks (VTE_TERMINAL(tt->vte_term), config_getbool ("blinks"));

    /** Scrolling **/
    vte_terminal_set_scroll_background (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_background"));
    vte_terminal_set_scroll_on_output (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_on_output"));
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_on_key"));

    /** Mouse **/
    vte_terminal_set_mouse_autohide (VTE_TERMINAL(tt->vte_term), FALSE); /* TODO: make this configurable */

    /** Text Properties **/
    vte_terminal_set_allow_bold (VTE_TERMINAL(tt->vte_term), config_getbool ("bold"));
    gtk_widget_set_double_buffered (tt->vte_term, config_getbool("double_buffer"));
    vte_terminal_set_font_from_string_full (VTE_TERMINAL(tt->vte_term), config_getstr ("font"), config_getbool ("antialias"));

    /** Scrollback **/
    vte_terminal_set_scrollback_lines (VTE_TERMINAL(tt->vte_term), config_getint ("lines"));

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

    return 0;
}

static void
menu_copy_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("menu_copy_cb");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    tilda_term *tt = TILDA_TERM(data);

    vte_terminal_copy_clipboard (VTE_TERMINAL (tt->vte_term));
}

static void
menu_paste_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("menu_paste_cb");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    tilda_term *tt = TILDA_TERM(data);

    vte_terminal_paste_clipboard (VTE_TERMINAL (tt->vte_term));
}


static void
menu_preferences_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("menu_config_cb");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    /* Show the config wizard */
    wizard (TILDA_WINDOW(data));
}

static void
menu_quit_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("menu_quit_cb");

    gtk_main_quit ();
}

static void
menu_add_tab_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("menu_add_tab_cb");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    tilda_window_add_tab (TILDA_WINDOW(data));
}

static void
menu_close_tab_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("menu_close_tab_cb");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (data != NULL);

    tilda_window_close_current_tab (TILDA_WINDOW(data));
}

static void popup_menu (tilda_window *tw, tilda_term *tt)
{
    DEBUG_FUNCTION ("popup_menu");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tt != NULL);

    GtkAction *action;
    GtkActionGroup *action_group;
    GtkUIManager *ui_manager;
    GError *error = NULL;
    GtkWidget *menu;

    /* Just use a static string here to initialize the GtkUIManager,
     * rather than installing and reading from a file all the time. */
    static const gchar menu_str[] =
        "<ui>"
            "<popup name=\"popup-menu\">"
                "<menuitem action=\"new-tab\" />"
                "<menuitem action=\"close-tab\" />"
                "<separator />"
                "<menuitem action=\"copy\" />"
                "<menuitem action=\"paste\" />"
                "<separator />"
                "<menuitem action=\"preferences\" />"
                "<separator />"
                "<menuitem action=\"quit\" />"
            "</popup>"
        "</ui>";

    /* Create the action group */
    action_group = gtk_action_group_new ("popup-menu-action-group");

    /* Add Actions and connect callbacks */
    action = gtk_action_new ("new-tab", _("_New Tab"), NULL, GTK_STOCK_ADD);
    gtk_action_group_add_action_with_accel (action_group, action, "<Ctrl><Shift>t");
    g_signal_connect (G_OBJECT(action), "activate", G_CALLBACK(menu_add_tab_cb), tw);

    action = gtk_action_new ("close-tab", _("_Close Tab"), NULL, GTK_STOCK_CLOSE);
    gtk_action_group_add_action_with_accel (action_group, action, "<Ctrl><Shift>w");
    g_signal_connect (G_OBJECT(action), "activate", G_CALLBACK(menu_close_tab_cb), tw);

    action = gtk_action_new ("copy", NULL, NULL, GTK_STOCK_COPY);
    gtk_action_group_add_action_with_accel (action_group, action, "<Ctrl><Shift>c");
    g_signal_connect (G_OBJECT(action), "activate", G_CALLBACK(menu_copy_cb), tt);

    action = gtk_action_new ("paste", NULL, NULL, GTK_STOCK_PASTE);
    gtk_action_group_add_action_with_accel (action_group, action, "<Ctrl><Shift>v");
    g_signal_connect (G_OBJECT(action), "activate", G_CALLBACK(menu_paste_cb), tt);

    action = gtk_action_new ("preferences", NULL, NULL, GTK_STOCK_PREFERENCES);
    gtk_action_group_add_action (action_group, action);
    g_signal_connect (G_OBJECT(action), "activate", G_CALLBACK(menu_preferences_cb), tw);

    action = gtk_action_new ("quit", NULL, NULL, GTK_STOCK_QUIT);
    gtk_action_group_add_action_with_accel (action_group, action, "<Ctrl><Shift>q");
    g_signal_connect (G_OBJECT(action), "activate", G_CALLBACK(menu_quit_cb), tw);

    /* Create and add actions to the GtkUIManager */
    ui_manager = gtk_ui_manager_new ();
    gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
    gtk_ui_manager_add_ui_from_string (ui_manager, menu_str, -1, &error);

    /* Check for an error (REALLY REALLY unlikely, unless the developers screwed up */
    if (error)
    {
        DEBUG_ERROR ("GtkUIManager problem\n");
        g_printerr ("Error message: %s\n", error->message);
        g_error_free (error);
    }

    /* Get the popup menu out of the GtkUIManager */
    menu = gtk_ui_manager_get_widget (ui_manager, "/ui/popup-menu");

    /* Display the menu */
    gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
    gtk_widget_show_all(menu);
}

static int button_press_cb (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    DEBUG_FUNCTION ("button_press_cb");
    DEBUG_ASSERT (data != NULL);

    VteTerminal *terminal;
    tilda_term *tt;
    gchar *match;
    gint tag;
    gint xpad, ypad;
    gchar *cmd;
    gchar *web_browser_cmd;
    gboolean ret = FALSE;

    tt = TILDA_TERM(data);

    switch (event->button)
    {
        case 3: /* Right Click */
            popup_menu (tt->tw, tt);
            break;
        case 2: /* Middle Click */
            break;
        case 1: /* Left Click */
            terminal  = VTE_TERMINAL(tt->vte_term);
            vte_terminal_get_padding (terminal, &xpad, &ypad);
            match = vte_terminal_match_check (terminal,
                    (event->x - ypad) /
                    terminal->char_width,
                    (event->y - ypad) /
                    terminal->char_height,
                    &tag);

            /* Check if we can launch a web browser, and do so if possible */
            if ((event->state & GDK_CONTROL_MASK) && match != NULL)
            {
#if DEBUG
                g_print ("Got a Ctrl+Left Click -- Matched: `%s' (%d)\n", match, tag);
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

                g_free (cmd);
            }

            /* Always free match if it is non NULL */
            if (match)
                g_free (match);

            break;
        default:
            break;
    }

    return FALSE;
}

/* vim: set ts=4 sts=4 sw=4 expandtab: */
