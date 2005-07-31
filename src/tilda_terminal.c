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

#include <stdio.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <vte/vte.h>
#include <string.h>
#include "config.h"
#include "tilda.h"
#include "../tilda-config.h"
#include "tilda_window.h"
#include "tilda_terminal.h"
#include "callback_func.h"

#define DINGUS1 "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?"
#define DINGUS2 "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?/[-A-Za-z0-9_\\$\\.\\+\\!\\*\\(\\),;:@&=\\?/~\\#\\%]*[^]'\\.}>\\) ,\\\"]"

gboolean init_tilda_terminal (tilda_window *tw, tilda_term *tt)
{
    gchar env_var[14];
    gint  env_add2_size;
    gchar *env_add[] = {"FOO=BAR", "BOO=BIZ", NULL, NULL};
    gboolean audible = TRUE, blink = TRUE, dingus = FALSE,
        dbuffer = TRUE, console = FALSE, scroll = FALSE,
        icon_title = FALSE, shell = TRUE;
    gint i;
    tilda_collect *t_collect;

    sprintf (env_var, "TILDA_NUM=%d", tw->instance);

    /* Create a box to hold everything. */
    tt->hbox = gtk_hbox_new (0, FALSE);

    t_collect = (tilda_collect *) malloc (sizeof (tilda_collect));
    t_collect->tw = tw;
    t_collect->tt = tt;

    /* Create the terminal widget and add it to the scrolling shell. */
    tt->vte_term = vte_terminal_new ();
    tt->scrollbar = gtk_vscrollbar_new ((VTE_TERMINAL(tt->vte_term))->adjustment);

    vte_terminal_set_backspace_binding (VTE_TERMINAL (tt->vte_term), VTE_ERASE_ASCII_BACKSPACE);

    if (!dbuffer)
        gtk_widget_set_double_buffered (tt->vte_term, dbuffer);

    if (SCROLLBAR_LEFT)
    {
        gtk_box_pack_start (GTK_BOX(tt->hbox), tt->scrollbar, FALSE, FALSE, 0); /* add scrollbar */
        gtk_box_pack_start (GTK_BOX(tt->hbox), tt->vte_term, TRUE, TRUE, 0); /* add term */
    }
    else /* scrollbar on the right */
    {
        gtk_box_pack_start (GTK_BOX(tt->hbox), tt->vte_term, TRUE, TRUE, 0); /* add term */
        gtk_box_pack_start (GTK_BOX(tt->hbox), tt->scrollbar, FALSE, FALSE, 0); /* add scrollbar */
    }

    /* Connect to the "window_title_changed" signal to set the main
     * window's title. */
    g_signal_connect (G_OBJECT(tt->vte_term), "window-title-changed",
                      G_CALLBACK(window_title_changed), tw->window);
    if (icon_title)
    {
        g_signal_connect (G_OBJECT(tt->vte_term), "icon-title-changed",
                          G_CALLBACK(icon_title_changed), tw->window);
    }

    /* Connect to the "eof" signal to quit when the session ends. */
    g_signal_connect (G_OBJECT(tt->vte_term), "eof",
                      G_CALLBACK(destroy_and_quit_eof), t_collect);
    g_signal_connect (G_OBJECT(tt->vte_term), "child-exited",
                      G_CALLBACK(close_tab_on_exit), t_collect);

    /* Connect to the "status-line-changed" signal. */
    g_signal_connect (G_OBJECT(tt->vte_term), "status-line-changed",
                      G_CALLBACK(status_line_changed), tt->vte_term);

    /* Connect to the "button-press" event. */
    g_signal_connect (G_OBJECT(tt->vte_term), "button-press-event",
                      G_CALLBACK(button_pressed), t_collect);

    /* Connect to application request signals. */
    g_signal_connect (G_OBJECT(tt->vte_term), "iconify-window",
                      G_CALLBACK(iconify_window), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "deiconify-window",
                      G_CALLBACK(deiconify_window), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "raise-window",
                      G_CALLBACK(raise_window), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "lower-window",
                      G_CALLBACK(lower_window), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "maximize-window",
                      G_CALLBACK(maximize_window), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "restore-window",
                      G_CALLBACK(restore_window), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "refresh-window",
                      G_CALLBACK(refresh_window), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "resize-window",
                      G_CALLBACK(resize_window), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "move-window",
                      G_CALLBACK(move_window), tw->window);

    /* Connect to font tweakage. */
    g_signal_connect (G_OBJECT(tt->vte_term), "increase-font-size",
                      G_CALLBACK(increase_font_size), tw->window);
    g_signal_connect (G_OBJECT(tt->vte_term), "decrease-font-size",
                      G_CALLBACK(decrease_font_size), tw->window);

    /* Match "abcdefg". */
    vte_terminal_match_add (VTE_TERMINAL(tt->vte_term), "abcdefg");

    env_add2_size = (sizeof(char) * strlen (env_var)) + 1;
    env_add[2] = (char *) malloc (env_add2_size);
    g_strlcpy (env_add[2], env_var, env_add2_size);

    if (dingus)
    {
        i = vte_terminal_match_add (VTE_TERMINAL(tt->vte_term), DINGUS1);
        vte_terminal_match_set_cursor_type (VTE_TERMINAL(tt->vte_term), i, GDK_GUMBY);
        i = vte_terminal_match_add(VTE_TERMINAL (tt->vte_term), DINGUS2);
        vte_terminal_match_set_cursor_type (VTE_TERMINAL(tt->vte_term), i, GDK_HAND1);
    }

    if (!console)
    {
        if (shell)
        {
            /* Launch a shell. */
            if (command == NULL)
            {
                #ifdef DEBUG
                perror ("had to fix 'command'");
                #endif

                command = getenv ("SHELL");
            }

            vte_terminal_fork_command (VTE_TERMINAL(tt->vte_term),
                command, NULL, env_add,
                working_directory,
                TRUE, TRUE, TRUE);
        }
    }

    /* Set some defaults. */
    vte_terminal_set_audible_bell (VTE_TERMINAL(tt->vte_term), audible);
    vte_terminal_set_visible_bell (VTE_TERMINAL(tt->vte_term), !audible);
    vte_terminal_set_cursor_blinks (VTE_TERMINAL(tt->vte_term), blink);
    vte_terminal_set_scroll_background (VTE_TERMINAL(tt->vte_term), scroll);
    vte_terminal_set_scroll_on_output (VTE_TERMINAL(tt->vte_term), FALSE);
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(tt->vte_term), TRUE);
    vte_terminal_set_scrollback_lines (VTE_TERMINAL(tt->vte_term), tw->tc->lines);
    vte_terminal_set_mouse_autohide (VTE_TERMINAL(tt->vte_term), TRUE);

    gtk_widget_show (tt->vte_term);
    gtk_widget_show (tt->hbox);

    /* Create page to append to notebook */
    gtk_notebook_prepend_page ((GtkNotebook *) tw->notebook, tt->hbox, gtk_label_new (TILDA_VERSION));
    gtk_notebook_set_tab_label_packing ((GtkNotebook *) tw->notebook, tt->hbox, TRUE, TRUE, GTK_PACK_END);
    gtk_notebook_set_current_page ((GtkNotebook *) tw->notebook, 0);

    /* Set everything up and display the widgets.
     *  Sending TRUE to let it know we are in main()
     */
    update_tilda (tw, tt, TRUE);

    return TRUE;
}

