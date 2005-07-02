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

gboolean update_tilda (tilda_window *tw, tilda_term *tt, gboolean from_main)
{
    gint x, y;
    double TRANS_LEVEL = 0;       /* how transparent the window is, percent from 0-100 */
    VteTerminalAntiAlias antialias = VTE_ANTI_ALIAS_USE_DEFAULT;
    gboolean scroll = FALSE, highlight_set = FALSE, cursor_set = FALSE,
             use_antialias = FALSE, bool_use_image = FALSE;
    GdkColor fore, back, tint, highlight, cursor;

    back.red = back.green = back.blue = 0xffff;
    fore.red = fore.green = fore.blue = 0x0000;
    highlight.red = highlight.green = highlight.blue = 0xc000;
    cursor.red = 0xffff;
    cursor.green = cursor.blue = 0x8000;
    tint.red = tint.green = tint.blue = 0;
    tint = back;

    if (TRANS_LEVEL_arg == -1)
        TRANS_LEVEL = ((double) tw->tc->transparency)/100;
    else
        TRANS_LEVEL = TRANS_LEVEL_arg;


    if (strcmp (tw->tc->s_use_image, "TRUE") == 0 || image_set_clo == TRUE)
        bool_use_image = TRUE;
    else
        bool_use_image = FALSE;

    if (strcmp (tw->tc->s_antialias, "TRUE") == 0 || antialias_set_clo == TRUE)
        use_antialias = TRUE;
    else
        use_antialias = FALSE;

    if (strcmp (tw->tc->s_scrollbar, "TRUE") == 0 || scroll_set_clo == TRUE)
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

    if (strcasecmp (tw->tc->s_background, "black") == 0)
    {
        back.red = back.green = back.blue = 0x0000;
        fore.red = fore.green = fore.blue = 0xffff;
    }

    vte_terminal_set_background_tint_color (VTE_TERMINAL(tt->vte_term), &tint);
    vte_terminal_set_colors (VTE_TERMINAL(tt->vte_term), &fore, &back, NULL, 0);

    if (highlight_set)
    {
        vte_terminal_set_color_highlight (VTE_TERMINAL(tt->vte_term), &highlight);
    }

    if (cursor_set)
    {
        vte_terminal_set_color_cursor (VTE_TERMINAL(tt->vte_term), &cursor);
    }

    if (strcasecmp (tw->tc->s_font, "null") != 0)
        strlcpy (tw->tc->s_font, tw->tc->s_font, sizeof (tw->tc->s_font));

    if (use_antialias)
        vte_terminal_set_font_from_string_full (VTE_TERMINAL(tt->vte_term), tw->tc->s_font, antialias);
    else
        vte_terminal_set_font_from_string (VTE_TERMINAL(tt->vte_term), tw->tc->s_font);

    if (!from_main)
    {
        gtk_widget_hide (tw->window);

        if (scroll)
            gtk_widget_show (tt->scrollbar);
        else
            gtk_widget_hide (tt->scrollbar);

        gtk_widget_show (tw->window);

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
    
    if (gtk_notebook_get_n_pages ((GtkNotebook *) tw->notebook) <= 1)   
        gtk_notebook_set_show_tabs ((GtkNotebook *) tw->notebook, FALSE);
    else {
        gtk_widget_hide (tw->notebook);
        gtk_notebook_set_show_tabs ((GtkNotebook *) tw->notebook, TRUE);
        gtk_widget_show (tw->notebook);
    }
        
    if (x_pos_arg != -1)
        x = x_pos_arg;
    else
        x = tw->tc->x_pos;
    if (y_pos_arg != -1)
        y = y_pos_arg;
    else
        y = tw->tc->y_pos;

    gtk_window_move ((GtkWindow *) tw->window, x, y);

    if (tw->tc->max_height != old_max_height || tw->tc->max_width != old_max_width)
    {
            gtk_window_resize ((GtkWindow *) tw->window, tw->tc->max_width, tw->tc->max_height);
    }

    return TRUE;
}
