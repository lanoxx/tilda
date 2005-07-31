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

#include <gtk/gtk.h>
#include <vte/vte.h>
#include "tilda.h"
#include "../tilda-config.h"
#include "tilda_window.h"
#include "tilda_terminal.h"
#include "wizard.h"

void copy (gpointer data, guint callback_action, GtkWidget *w);
void paste (gpointer data, guint callback_action, GtkWidget *w);
void config_and_update (gpointer data, guint callback_action, GtkWidget *w);
void menu_quit (gpointer data, guint callback_action, GtkWidget *w);

static GtkItemFactoryEntry menu_items[] = {
    { "/_New Tab",        NULL,      add_tab_menu_call,     0, "<Item>"                             },
    { "/_Close Tab",      NULL,      close_tab,             0, "<Item>"                             },
    { "/sep1",            NULL,      NULL,                  0, "<Separator>"                        },
    { "/_Copy",           NULL,      copy,                  0, "<StockItem>", GTK_STOCK_COPY        },
    { "/_Paste",          NULL,      paste,                 0, "<StockItem>", GTK_STOCK_PASTE       },
    { "/sep1",            NULL,      NULL,                  0, "<Separator>"                        },
    { "/_Preferences...", NULL,      config_and_update,     0, "<StockItem>", GTK_STOCK_PREFERENCES },
    { "/sep1",            NULL,      NULL,                  0, "<Separator>"                        },
    { "/_Quit",         "<Ctrl>Q",  menu_quit,              0, "<StockItem>", GTK_STOCK_QUIT        }
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


void fix_size_settings (tilda_window *tw)
{
    gtk_window_resize ((GtkWindow *) tw->window, tw->tc->max_width, tw->tc->max_height);
    gtk_window_get_size ((GtkWindow *) tw->window, &tw->tc->max_width, &tw->tc->max_height);
    gtk_window_resize ((GtkWindow *) tw->window, tw->tc->min_width, tw->tc->min_height);
    gtk_window_get_size ((GtkWindow *) tw->window, &tw->tc->min_width, &tw->tc->min_height);
}

int resize (GtkWidget *window, gint w, gint h)
{
    gtk_window_resize ((GtkWindow *) window, w, h);

    return 0;
}

void clean_up_no_args ()
{
     gtk_main_quit ();
}

/* Removes the temporary file socket used to communicate with a running tilda */
void clean_up (tilda_window *tw)
{
    remove (tw->lock_file);

    gtk_main_quit ();
}

void close_tab_on_exit (GtkWidget *widget, gpointer data)
{
    gint pos;
    tilda_term *tt;
    tilda_window *tw;
    tilda_collect *tc = (tilda_collect *) data;

    tw = tc->tw;
    tt = tc->tt;

    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) < 2)
    {
        clean_up (tw);
    } else {
        pos = gtk_notebook_page_num (GTK_NOTEBOOK (tw->notebook), tt->hbox);
        gtk_notebook_remove_page (GTK_NOTEBOOK (tw->notebook), pos);

        if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) == 1)
            gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), FALSE);

        g_free (tt);
    }
}

 void window_title_changed (GtkWidget *widget, gpointer win)
{
    GtkWindow *window;

    g_return_if_fail (VTE_TERMINAL(widget));
    g_return_if_fail (GTK_IS_WINDOW(win));
    g_return_if_fail (VTE_TERMINAL(widget)->window_title != NULL);
    window = GTK_WINDOW(win);

    gtk_window_set_title (window, VTE_TERMINAL(widget)->window_title);
}

 void icon_title_changed (GtkWidget *widget, gpointer win)
{
    GtkWindow *window;

    g_return_if_fail (VTE_TERMINAL(widget));
    g_return_if_fail (GTK_IS_WINDOW(win));
    g_return_if_fail (VTE_TERMINAL(widget)->icon_title != NULL);
    window = GTK_WINDOW(win);

    g_message ("Icon title changed to \"%s\".\n",
          VTE_TERMINAL(widget)->icon_title);
}

void deleted_and_quit (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
    gtk_main_quit();
}

void destroy_and_quit (GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy (GTK_WIDGET(data));
    gtk_main_quit ();
}

void destroy_and_quit_eof (GtkWidget *widget, gpointer data)
{
   close_tab_on_exit (widget, data);
}

void destroy_and_quit_exited (GtkWidget *widget, gpointer data)
{
    //g_print("Detected child exit.\n");
    destroy_and_quit (widget, data);
}

void status_line_changed (GtkWidget *widget, gpointer data)
{
    g_print ("Status = `%s'.\n", vte_terminal_get_status_line (VTE_TERMINAL(widget)));
}

void copy (gpointer data, guint callback_action, GtkWidget *w)
{
    tilda_window *tw;
    tilda_term *tt;
    tilda_collect *tc = (tilda_collect *) data;

    tw = tc->tw;
    tt = tc->tt;

    vte_terminal_copy_clipboard ((VteTerminal *) tt->vte_term);
}

void paste (gpointer data, guint callback_action, GtkWidget *w)
{
    tilda_window *tw;
    tilda_term *tt;
    tilda_collect *tc = (tilda_collect *) data;

    tw = tc->tw;
    tt = tc->tt;

    vte_terminal_paste_clipboard ((VteTerminal *) tt->vte_term);
}

void config_and_update (gpointer data, guint callback_action, GtkWidget *w)
{
    tilda_window *tw;
    tilda_term *tt;
    tilda_collect *tc = (tilda_collect *) data;

    tw = tc->tw;
    tt = tc->tt;

    wizard (-1, NULL, tw, tt);
}

void menu_quit (gpointer data, guint callback_action, GtkWidget *w)
{
    //gtk_widget_destroy (GTK_WIDGET(tw->window));
    gtk_main_quit ();
}

void popup_menu (tilda_collect *tc)
{
    GtkItemFactory *item_factory;
    GtkWidget *menu;

    item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", NULL);

    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, tc);

    menu = gtk_item_factory_get_widget (item_factory, "<main>");

    gtk_menu_popup (GTK_MENU(menu), NULL, NULL,
                   NULL, NULL, 3, gtk_get_current_event_time());

    gtk_widget_show_all(menu);
}

int button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    VteTerminal *terminal;
    tilda_term *tt;
    tilda_collect *tc;
    char *match;
    int tag;
    gint xpad, ypad;

    tc = (tilda_collect *) data;
    tt = tc->tt;

    switch (event->button) {
        case 3:
            popup_menu (tc);

            terminal  = VTE_TERMINAL(tt->vte_term);
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

void iconify_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_iconify ((GTK_WIDGET(data))->window);
        }
    }
}

void deiconify_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_deiconify ((GTK_WIDGET(data))->window);
        }
    }
}

void raise_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_raise ((GTK_WIDGET(data))->window);
        }
    }
}

void lower_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_lower ((GTK_WIDGET(data))->window);
        }
    }
}

void maximize_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_maximize ((GTK_WIDGET(data))->window);
        }
    }
}

void restore_window (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_unmaximize ((GTK_WIDGET(data))->window);
        }
    }
}

void refresh_window (GtkWidget *widget, gpointer data)
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

void resize_window (GtkWidget *widget, guint width, guint height, gpointer data)
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

void move_window (GtkWidget *widget, guint x, guint y, gpointer data)
{
    if (GTK_IS_WIDGET(data)) {
        if ((GTK_WIDGET(data))->window) {
            gdk_window_move ((GTK_WIDGET(data))->window, x, y);
        }
    }
}
void focus_term (GtkWidget *widget, gpointer data)
{
	GList *list;
	GtkWidget *box;
	GtkWidget *n = (GtkWidget *) data;
	
	box = gtk_notebook_get_nth_page ((GtkNotebook *) n, gtk_notebook_get_current_page((GtkNotebook *) n));
	list = gtk_container_children ((GtkContainer *) box);
	gtk_widget_grab_focus (list->data);
}

void adjust_font_size (GtkWidget *widget, gpointer data, gint howmuch)
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

void increase_font_size(GtkWidget *widget, gpointer data)
{
    adjust_font_size (widget, data, 1);
}

void decrease_font_size (GtkWidget *widget, gpointer data)
{
    adjust_font_size (widget, data, -1);
}

