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

#ifndef TILDA_CALLBACK_FUNC_C
#define TILDA_CALLBACK_FUNC_C

void copy (GtkWidget *widget, gpointer data);
void paste (GtkWidget *widget, gpointer data);
void config_and_update (GtkWidget *widget, gpointer data);
void menu_quit (GtkWidget *widget, gpointer data);

static GtkItemFactoryEntry menu_items[] = {
    { "/_Copy", NULL, copy, 0, "<StockItem>", GTK_STOCK_COPY},
    { "/_Paste", NULL, paste, 0, "<StockItem>", GTK_STOCK_PASTE},
    { "/sep1",     NULL,      NULL,         0, "<Separator>" },
    { "/_Preferences...", NULL, config_and_update, 0, "<StockItem>", GTK_STOCK_PREFERENCES},
    { "/sep1",     NULL,      NULL,         0, "<Separator>" },
    { "/_Quit", "<Ctrl>Q", menu_quit, 0, "<StockItem>", GTK_STOCK_QUIT } };

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


void fix_size_settings ()
{
    gtk_window_resize ((GtkWindow *) window, max_width, max_height);
    gtk_window_get_size ((GtkWindow *) window, &max_width, &max_height);
    gtk_window_resize ((GtkWindow *) window, min_width, min_height);
    gtk_window_get_size ((GtkWindow *) window, &min_width, &min_height);
}

int resize (GtkWidget *window, gint w, gint h)
{
    gtk_window_resize ((GtkWindow *) window, w, h);

    return 0;
}

static void window_title_changed (GtkWidget *widget, gpointer win)
{
    GtkWindow *window;

    g_return_if_fail (VTE_TERMINAL(widget));
    g_return_if_fail (GTK_IS_WINDOW(win));
    g_return_if_fail (VTE_TERMINAL(widget)->window_title != NULL);
    window = GTK_WINDOW(win);

    gtk_window_set_title (window, VTE_TERMINAL(widget)->window_title);
}

static void icon_title_changed (GtkWidget *widget, gpointer win)
{
    GtkWindow *window;

    g_return_if_fail (VTE_TERMINAL(widget));
    g_return_if_fail (GTK_IS_WINDOW(win));
    g_return_if_fail (VTE_TERMINAL(widget)->icon_title != NULL);
    window = GTK_WINDOW(win);

    g_message ("Icon title changed to \"%s\".\n",
          VTE_TERMINAL(widget)->icon_title);
}

static void char_size_changed (GtkWidget *widget, guint width, guint height, gpointer data)
{
    /*VteTerminal *terminal;
    GtkWindow *window;
    GdkGeometry geometry;
    int xpad, ypad;

    g_return_if_fail (GTK_IS_WINDOW(data));
    g_return_if_fail (VTE_IS_TERMINAL(widget));

    terminal = VTE_TERMINAL(widget);
    window = GTK_WINDOW(data);

    vte_terminal_get_padding (terminal, &xpad, &ypad);

    geometry.width_inc = terminal->char_width;
    geometry.height_inc = terminal->char_height;
    geometry.base_width = xpad;
    geometry.base_height = ypad;
    geometry.min_width = xpad + terminal->char_width * 2;
    geometry.min_height = ypad + terminal->char_height * 2;

    gtk_window_set_geometry_hints (window, widget, &geometry,
                      GDK_HINT_RESIZE_INC |
                      GDK_HINT_BASE_SIZE |
                      GDK_HINT_MIN_SIZE);*/
}

static void deleted_and_quit (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
    gtk_main_quit();
}

static void destroy_and_quit (GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy (GTK_WIDGET(data));
    gtk_main_quit ();
}

static void destroy_and_quit_eof (GtkWidget *widget, gpointer data)
{
    /* g_print("Detected EOF.\n"); */
}

static void destroy_and_quit_exited (GtkWidget *widget, gpointer data)
{
    //g_print("Detected child exit.\n");
    destroy_and_quit (widget, data);
}

static void status_line_changed (GtkWidget *widget, gpointer data)
{
    g_print ("Status = `%s'.\n", vte_terminal_get_status_line (VTE_TERMINAL(widget)));
}

void copy (GtkWidget *w, gpointer data)
{
    vte_terminal_copy_clipboard ((VteTerminal *) widget);
}

void paste (GtkWidget *w, gpointer data)
{
    vte_terminal_paste_clipboard ((VteTerminal *) widget);
}

void config_and_update (GtkWidget *widget, gpointer data)
{
    wizard (-1, NULL);
}

void menu_quit (GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy (GTK_WIDGET(window));
    gtk_main_quit ();
}

void popup_menu ()
{
    GtkItemFactory *item_factory;
    GtkWidget *menu;

    item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>",
                                        NULL);

    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

    menu = gtk_item_factory_get_widget (item_factory, "<main>");

    gtk_menu_popup (GTK_MENU(menu), NULL, NULL,
                   NULL, NULL, 3, gtk_get_current_event_time());

    gtk_widget_show_all(menu);
}

static int button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    VteTerminal *terminal;
    char *match;
    int tag;
    gint xpad, ypad;

    switch (event->button) {
    case 3:
        popup_menu ();

        terminal = VTE_TERMINAL(widget);
        vte_terminal_get_padding (terminal, &xpad, &ypad);
        match = vte_terminal_match_check (terminal,
                         (event->x - ypad) /
                         terminal->char_width,
                         (event->y - ypad) /
                         terminal->char_height,
                         &tag);
        if (match != NULL) {
            g_print ("Matched `%s' (%d).\n", match, tag);
            g_free (match);
            if (GPOINTER_TO_INT(data) != 0) {
                vte_terminal_match_remove (terminal, tag);
            }
        }
        break;
    case 1:
    case 2:
    default:
        break;
    }
    return FALSE;
}

static void iconify_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_iconify ((GTK_WIDGET(data))->window);
        }
    }
}

static void deiconify_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_deiconify ((GTK_WIDGET(data))->window);
        }
    }
}

static void raise_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_raise ((GTK_WIDGET(data))->window);
        }
    }
}

static void lower_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_lower ((GTK_WIDGET(data))->window);
        }
    }
}

static void maximize_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_maximize ((GTK_WIDGET(data))->window);
        }
    }
}

static void restore_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_unmaximize ((GTK_WIDGET(data))->window);
        }
    }
}

static void refresh_window (GtkWidget *widget, gpointer data)
{
    GdkRectangle rect;
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            rect.x = rect.y = 0;
            rect.width = (GTK_WIDGET(data))->allocation.width;
            rect.height = (GTK_WIDGET(data))->allocation.height;
            gdk_window_invalidate_rect ((GTK_WIDGET(data))->window,
                           &rect, TRUE);
        }
    }
}

static void resize_window (GtkWidget *widget, guint width, guint height, gpointer data)
{
    VteTerminal *terminal;
    gint owidth, oheight, xpad, ypad;
    if ((GTK_IS_WINDOW(data)) && (width >= 2) && (height >= 2)) {
        terminal = VTE_TERMINAL(widget);

        /* Take into account border overhead. */
        gtk_window_get_size (GTK_WINDOW(data), &owidth, &oheight);
        owidth -= terminal->char_width * terminal->column_count;
        oheight -= terminal->char_height * terminal->row_count;

        /* Take into account padding, which needn't be re-added. */
        vte_terminal_get_padding (VTE_TERMINAL(widget), &xpad, &ypad);
        owidth -= xpad;
        oheight -= ypad;
        gtk_window_resize (GTK_WINDOW(data),
                  width + owidth, height + oheight);
    }
}

static void move_window (GtkWidget *widget, guint x, guint y, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_move ((GTK_WIDGET(data))->window, x, y);
        }
    }
}

static void adjust_font_size (GtkWidget *widget, gpointer data, gint howmuch)
{
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

static void increase_font_size(GtkWidget *widget, gpointer data)
{
    adjust_font_size (widget, data, 1);
}

static void decrease_font_size (GtkWidget *widget, gpointer data)
{
    adjust_font_size (widget, data, -1);
}

/*
static gboolean read_and_feed (GIOChannel *source, GIOCondition condition, gpointer data)
{
    char buf[2048];
    gsize size;
    GIOStatus status;
    g_return_val_if_fail (VTE_IS_TERMINAL(data), FALSE);
    status = g_io_channel_read_chars (source, buf, sizeof(buf), &size, NULL);

    if ((status == G_IO_STATUS_NORMAL) && (size > 0)) {
        vte_terminal_feed (VTE_TERMINAL(data), buf, size);
        return TRUE;
    }
    return FALSE;
}
*/

/*
static void disconnect_watch (GtkWidget *widget, gpointer data)
{
    g_source_remove (GPOINTER_TO_INT(data));
}
*/

/*
static void clipboard_get (GtkClipboard *clipboard, GtkSelectionData *selection_data,
          guint info, gpointer owner)
{
    return;
}
*/

/*
static void take_xconsole_ownership (GtkWidget *widget, gpointer data)
{
    char *name, hostname[255];
    GdkAtom atom;
    GtkClipboard *clipboard;
    GtkTargetEntry targets[] = {
        {"UTF8_STRING", 0, 0},
        {"COMPOUND_TEXT", 0, 0},
        {"TEXT", 0, 0},
        {"STRING", 0, 0},
    };

    memset (hostname, '\0', sizeof(hostname));
    gethostname (hostname, sizeof(hostname) - 1);

    name = g_strdup_printf ("MIT_CONSOLE_%s", hostname);
    atom = gdk_atom_intern (name, FALSE);
#if GTK_CHECK_VERSION(2,2,0)
    clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (widget),
                          atom);
#else
    clipboard = gtk_clipboard_get (atom);
#endif
    g_free(name);

    gtk_clipboard_set_with_owner (clipboard,
                     targets,
                     G_N_ELEMENTS(targets),
                     clipboard_get,
                     (GtkClipboardClearFunc)gtk_main_quit,
                     G_OBJECT(widget));
}
*/

/*
static void add_weak_pointer(GObject *object, GtkWidget **target)
{
    _weak_pointer(object, (gpointer*)target);
}
*/

#endif
