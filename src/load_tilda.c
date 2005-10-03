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
#include <string.h>
#include <strings.h>

#include "tilda.h"
#include "callback_func.h"
#include "../tilda-config.h"

void window_title_change_all (tilda_window *tw) 
{
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
    gdouble TRANS_LEVEL = 0;       /* how transparent the window is, percent from 0-100 */
    VteTerminalAntiAlias antialias = VTE_ANTI_ALIAS_USE_DEFAULT;
    gboolean scroll = FALSE, scroll_background = FALSE, highlight_set = FALSE, cursor_set = FALSE,
             use_antialias = FALSE, bool_use_image = FALSE, allow_bold=FALSE;
    gboolean audible = FALSE, blink = FALSE, scroll_on_output = FALSE, scroll_on_keystroke = FALSE;
    GdkColor fore, back, tint, highlight, cursor, black;

    black.red = black.green = black.blue = 0x0000;

    back.red = tw->tc->back_red;
    back.green = tw->tc->back_green;
    back.blue = tw->tc->back_blue;
    
    fore.red = tw->tc->text_red;
    fore.green = tw->tc->text_green;
    fore.blue = tw->tc->text_blue;
    
    highlight.red = highlight.green = highlight.blue = 0xc000;
    cursor.red = 0xffff;
    cursor.green = cursor.blue = 0x8000;
    tint.red = tint.green = tint.blue = 0;
    tint = black;

    TRANS_LEVEL = ((gdouble) tw->tc->transparency)/100;
    
    if (QUICK_STRCMP (tw->tc->s_bell, "TRUE") == 0)
        audible = TRUE;
    
    if (QUICK_STRCMP (tw->tc->s_blinks, "TRUE") == 0)
        blink = TRUE;
        
    if (QUICK_STRCMP (tw->tc->s_scroll_background, "TRUE") == 0)
        scroll_background = TRUE;
    
    if (QUICK_STRCMP (tw->tc->s_scroll_on_output, "TRUE") == 0)
        scroll_on_output = TRUE;
         
    if (QUICK_STRCMP (tw->tc->s_scroll_on_key, "TRUE") == 0)
        scroll_on_keystroke = TRUE; 
        
    if (QUICK_STRCMP (tw->tc->s_bold, "TRUE") == 0)
        allow_bold = TRUE;     
    
    /* Set some defaults. */
    vte_terminal_set_audible_bell (VTE_TERMINAL(tt->vte_term), audible);
    vte_terminal_set_visible_bell (VTE_TERMINAL(tt->vte_term), !audible);
    vte_terminal_set_cursor_blinks (VTE_TERMINAL(tt->vte_term), blink);
    vte_terminal_set_scroll_background (VTE_TERMINAL(tt->vte_term), scroll_background);
    vte_terminal_set_scroll_on_output (VTE_TERMINAL(tt->vte_term), scroll_on_output);
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(tt->vte_term), scroll_on_keystroke);
    vte_terminal_set_mouse_autohide (VTE_TERMINAL(tt->vte_term), TRUE);
    vte_terminal_set_allow_bold (VTE_TERMINAL(tt->vte_term), allow_bold);
    
    /* What to do when child command exits */
    after_command = tw->tc->command_exit;
    
    switch (tw->tc->backspace_key)
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
    
    switch (tw->tc->delete_key)
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
    
    if (QUICK_STRCMP (tw->tc->s_use_image, "TRUE") == 0 || image_set_clo == TRUE)
        bool_use_image = TRUE;
    else
        bool_use_image = FALSE;

    if (QUICK_STRCMP (tw->tc->s_antialias, "TRUE") == 0 || antialias_set_clo == TRUE)
        use_antialias = TRUE;
    else
        use_antialias = FALSE;

    if (QUICK_STRCMP (tw->tc->s_scrollbar, "TRUE") == 0 || scroll_set_clo == TRUE)
        scroll = TRUE;
    else
        scroll = FALSE;

    if (bool_use_image)
        vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), tw->tc->s_image);
    else
        vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), NULL);

    if (TRANS_LEVEL > 0)
    {
        vte_terminal_set_background_saturation (VTE_TERMINAL (tt->vte_term), TRANS_LEVEL);
        vte_terminal_set_background_transparent (VTE_TERMINAL(tt->vte_term), TRUE);
    } else
        vte_terminal_set_background_transparent (VTE_TERMINAL(tt->vte_term), FALSE);
        
    vte_terminal_set_background_tint_color (VTE_TERMINAL(tt->vte_term), &tint);
    vte_terminal_set_colors (VTE_TERMINAL(tt->vte_term), &fore, &back, NULL, 0);

    if (highlight_set)
        vte_terminal_set_color_highlight (VTE_TERMINAL(tt->vte_term), &highlight);

    if (cursor_set)
        vte_terminal_set_color_cursor (VTE_TERMINAL(tt->vte_term), &cursor);

    if (use_antialias)
        vte_terminal_set_font_from_string_full (VTE_TERMINAL(tt->vte_term), tw->tc->s_font, antialias);
    else
        vte_terminal_set_font_from_string (VTE_TERMINAL(tt->vte_term), tw->tc->s_font);

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
    
    gtk_box_reorder_child ((GtkBox *) tt->hbox, tt->scrollbar, tw->tc->scrollbar_pos);

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

        if ((strcasecmp (tw->tc->s_pinned, "true")) == 0)
            gtk_window_stick (GTK_WINDOW (tw->window));

        if ((strcasecmp (tw->tc->s_above, "true")) == 0)
            gtk_window_set_keep_above (GTK_WINDOW (tw->window), TRUE);
    }
    else
    {
        if (scroll)
            gtk_widget_show (tt->scrollbar);
        else
            gtk_widget_hide (tt->scrollbar);
    }

    vte_terminal_set_scrollback_lines (VTE_TERMINAL(tt->vte_term), tw->tc->lines);

    if (tw->tc->tab_pos == 0)
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_TOP);
    else if (tw->tc->tab_pos == 1)
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_BOTTOM);
    else if (tw->tc->tab_pos == 2)
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_LEFT);
    else if (tw->tc->tab_pos == 3)
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_RIGHT);

    if ((strcasecmp (tw->tc->s_notaskbar, "true")) == 0)
        gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tw->window), TRUE);
    else
        gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tw->window), FALSE);

    gtk_window_move ((GtkWindow *) tw->window, tw->tc->x_pos, tw->tc->y_pos);

    if (tw->tc->max_height != old_max_height || tw->tc->max_width != old_max_width)
        gtk_window_resize ((GtkWindow *) tw->window, tw->tc->max_width, tw->tc->max_height);

    if (QUICK_STRCMP (tw->tc->s_notebook_border, "TRUE") == 0)
        gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook), TRUE);
    else
        gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook), FALSE);
    
    window_title_change_all (tw);

    return TRUE;
}

