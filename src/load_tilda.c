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

#include <load_tilda.h>
#include <configsys.h>
#include <debug.h>
#include <callback_func.h>
#include <key_grabber.h>
#include <tilda.h>
#include <translation.h>

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <string.h>
#include <strings.h>

static void window_title_change_all (tilda_window *tw)
{
    DEBUG_FUNCTION ("window_title_change_all");
    DEBUG_ASSERT (tw != NULL);

    GtkWidget *page;
    GtkWidget *label;
    GtkWidget *widget;
    tilda_term *tt;
    char *title;
    int i;
    int size, list_count;

    size = gtk_notebook_get_n_pages ((GtkNotebook *) tw->notebook);
    list_count = size-1;

    for (i=0;i<size;i++,list_count--)
    {
        tt = g_list_nth (tw->terms, list_count)->data;
        widget = tt->vte_term;
        title = get_window_title (widget, tw);
        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook), i);
        label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (tw->notebook), page);
        gtk_label_set_label ((GtkLabel *) label, title);
    }
}

gboolean update_tilda (tilda_window *tw, tilda_term *tt, gboolean from_main)
{
    DEBUG_FUNCTION ("update_tilda");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tt != NULL);

    gdouble transparency_level = 0;       /* how transparent the window is, percent from 0-100 */
    VteTerminalAntiAlias antialias = VTE_ANTI_ALIAS_USE_DEFAULT;
    gboolean scroll = FALSE, highlight_set = FALSE, cursor_set = FALSE,
             use_antialias = FALSE, bool_use_image = FALSE;
    GdkColor fore, back, tint, highlight, cursor, black;

    black.red = black.green = black.blue = 0x0000;

    back.red   =    config_getint ("back_red");
    back.green =    config_getint ("back_green");
    back.blue  =    config_getint ("back_blue");

    fore.red   =    config_getint ("text_red");
    fore.green =    config_getint ("text_green");
    fore.blue  =    config_getint ("text_blue");

    highlight.red = highlight.green = highlight.blue = 0xc000;
    cursor.red = 0xffff;
    cursor.green = cursor.blue = 0x8000;
    tint.red = tint.green = tint.blue = 0;
    tint = black;

    transparency_level = ((gdouble) config_getint ("transparency"))/100;

    /* Set some defaults. */
    vte_terminal_set_audible_bell (VTE_TERMINAL(tt->vte_term), config_getbool ("bell"));
    vte_terminal_set_visible_bell (VTE_TERMINAL(tt->vte_term), !config_getbool ("bell"));
    vte_terminal_set_cursor_blinks (VTE_TERMINAL(tt->vte_term), config_getbool ("blinks"));
    vte_terminal_set_scroll_background (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_background"));
    vte_terminal_set_scroll_on_output (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_on_output"));
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(tt->vte_term), config_getbool ("scroll_on_key"));
    vte_terminal_set_mouse_autohide (VTE_TERMINAL(tt->vte_term), TRUE);
    vte_terminal_set_allow_bold (VTE_TERMINAL(tt->vte_term), config_getbool ("bold"));

    /* Causes terminal to blink if TRUE for some people */
    gtk_widget_set_double_buffered (tt->vte_term, config_getbool("double_buffer"));

    switch (config_getint ("backspace_key"))
    {
        case 0:
            vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_ASCII_DELETE);
            break;
        case 1:
            vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_DELETE_SEQUENCE);
            break;
        case 2:
            vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_ASCII_BACKSPACE);
            break;
        default:
            vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_AUTO);
            break;
    }

    switch (config_getint ("delete_key"))
    {
        case 0:
            vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_ASCII_DELETE);
            break;
        case 1:
            vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_DELETE_SEQUENCE);
            break;
        case 2:
            vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_ASCII_BACKSPACE);
            break;
        default:
            vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), VTE_ERASE_AUTO);
            break;
    }

    bool_use_image = config_getbool ("use_image");
    use_antialias = config_getbool ("antialias");
    scroll = config_getbool ("scrollbar");

    if (bool_use_image)
        vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), config_getstr ("image"));
    else
        vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), NULL);

    if (config_getbool ("enable_transparency") && transparency_level > 0)
    {
        vte_terminal_set_background_saturation (VTE_TERMINAL (tt->vte_term), transparency_level);
        vte_terminal_set_opacity (VTE_TERMINAL (tt->vte_term), (1.0 - transparency_level) * 0xffff);
        vte_terminal_set_background_transparent (VTE_TERMINAL(tt->vte_term), !tw->have_argb_visual);
    }

    vte_terminal_set_background_tint_color (VTE_TERMINAL(tt->vte_term), &tint);
    vte_terminal_set_colors (VTE_TERMINAL(tt->vte_term), &fore, &back, NULL, 0);

    if (highlight_set)
        vte_terminal_set_color_highlight (VTE_TERMINAL(tt->vte_term), &highlight);

    if (cursor_set)
        vte_terminal_set_color_cursor (VTE_TERMINAL(tt->vte_term), &cursor);

    if (use_antialias)
        vte_terminal_set_font_from_string_full (VTE_TERMINAL(tt->vte_term), config_getstr ("font"), antialias);
    else
        vte_terminal_set_font_from_string (VTE_TERMINAL(tt->vte_term), config_getstr ("font"));

    if (gtk_notebook_get_n_pages ((GtkNotebook *) tw->notebook) > 1)
    {
        gtk_widget_hide (tw->notebook);
        gtk_notebook_set_show_tabs ((GtkNotebook *) tw->notebook, TRUE);
        gtk_widget_show (tw->notebook);
    }
    else
    {
       gtk_notebook_set_show_tabs ((GtkNotebook *) tw->notebook, FALSE);
    }

    gtk_box_reorder_child ((GtkBox *) tt->hbox, tt->scrollbar, config_getint ("scrollbar_pos"));

    if (!from_main)
    {
        if (scroll)
        {
            gtk_widget_hide (tw->window);
            gtk_widget_show (tt->scrollbar);
            gtk_widget_show (tw->window);
        }
        else
        {
            gtk_widget_hide (tt->scrollbar);
        }

        refresh_window (tw->window, tw->window);

        if (config_getbool ("pinned"))
            gtk_window_stick (GTK_WINDOW (tw->window));

        gtk_window_set_keep_above (GTK_WINDOW (tw->window), config_getbool ("above"));
    }
    else
    {
        if (scroll)
            gtk_widget_show (tt->scrollbar);
        else
            gtk_widget_hide (tt->scrollbar);
    }

    vte_terminal_set_scrollback_lines (VTE_TERMINAL(tt->vte_term), config_getint ("lines"));

    switch (config_getint ("tab_pos"))
    {
        case 0:
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_TOP);
            break;
        case 1:
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_BOTTOM);
            break;
        case 2:
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_LEFT);
            break;
        case 3:
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_RIGHT);
            break;
        default:
            DEBUG_ERROR ("Tab position");
            fprintf (stderr, _("Bad tab_pos, not changing\n"));
            break;
    }

    if (config_getbool ("centered_horizontally"))
        config_setint ("x_pos", find_centering_coordinate (get_physical_width_pixels(), config_getint ("max_width")));

    if (config_getbool ("centered_vertically"))
        config_setint ("y_pos", find_centering_coordinate (get_physical_height_pixels(), config_getint ("max_height")));

    /* redo animation positions */
    generate_animation_positions (tw);

    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tw->window), config_getbool ("notaskbar"));

    gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));

    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook), config_getbool ("notebook_border"));

    window_title_change_all (tw);

    return TRUE;
}

