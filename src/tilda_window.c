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
#include "config.h"
#include "../tilda-config.h"
#include "tilda.h"
#include "callback_func.h"
#include "tilda_window.h"
#include "tilda_terminal.h"
#include "key_grabber.h"

/* CONFIGURATION OPTIONS */
static cfg_opt_t new_conf[] = {

    /* strings */
    CFG_STR("image", NULL, CFGF_NONE),
    CFG_STR("command", NULL, CFGF_NONE),
    CFG_STR("font", "Monospace 13", CFGF_NONE),
    CFG_STR("key", NULL, CFGF_NONE),
    CFG_STR("title", "Tilda", CFGF_NONE),
    CFG_STR("background_color", "white", CFGF_NONE),
    CFG_STR("working_dir", NULL, CFGF_NONE),

    /* ints */
    CFG_INT("lines", 100, CFGF_NONE),
    CFG_INT("max_width", 600, CFGF_NONE),
    CFG_INT("max_height", 150, CFGF_NONE),
    CFG_INT("min_width", 0, CFGF_NONE),
    CFG_INT("min_height", 0, CFGF_NONE),
    CFG_INT("transparency", 0, CFGF_NONE),
    CFG_INT("x_pos", 0, CFGF_NONE),
    CFG_INT("y_pos", 0, CFGF_NONE),
    CFG_INT("tab_pos", 0, CFGF_NONE),
    CFG_INT("backspace_key", 0, CFGF_NONE),
    CFG_INT("delete_key", 1, CFGF_NONE),
    CFG_INT("d_set_title", 3, CFGF_NONE),
    CFG_INT("command_exit", 0, CFGF_NONE),
    CFG_INT("scheme", 3, CFGF_NONE),

    /* guint16 */
    CFG_INT("scrollbar_pos", 1, CFGF_NONE),
    CFG_INT("back_red", 0x0000, CFGF_NONE),
    CFG_INT("back_green", 0x0000, CFGF_NONE),
    CFG_INT("back_blue", 0x0000, CFGF_NONE),
    CFG_INT("text_red", 0xffff, CFGF_NONE),
    CFG_INT("text_green", 0xffff, CFGF_NONE),
    CFG_INT("text_blue", 0xffff, CFGF_NONE),

    /* booleans */
    CFG_BOOL("scroll_background", TRUE, CFGF_NONE),
    CFG_BOOL("scroll_on_output", FALSE, CFGF_NONE),
    CFG_BOOL("notebook_border", FALSE, CFGF_NONE),
    CFG_BOOL("down", TRUE, CFGF_NONE),
    CFG_BOOL("antialias", TRUE, CFGF_NONE),
    CFG_BOOL("scrollbar", FALSE, CFGF_NONE),
    CFG_BOOL("use_image", FALSE, CFGF_NONE),
    CFG_BOOL("grab_focus", TRUE, CFGF_NONE),
    CFG_BOOL("above", TRUE, CFGF_NONE),
    CFG_BOOL("notaskbar", TRUE, CFGF_NONE),
    CFG_BOOL("bold", TRUE, CFGF_NONE),
    CFG_BOOL("blinks", TRUE, CFGF_NONE),
    CFG_BOOL("scroll_on_key", TRUE, CFGF_NONE),
    CFG_BOOL("bell", FALSE, CFGF_NONE),
    CFG_BOOL("run_command", FALSE, CFGF_NONE),
    CFG_BOOL("pinned", TRUE, CFGF_NONE),
    CFG_END()
};

void init_tilda_window_configs (tilda_window *tw)
{
#ifdef DEBUG
    puts("init_tilda_window_configs");
#endif

    /* Set up the default config */
    tw->tc = cfg_init (new_conf, 0);

    /* Parse the config file */
    cfg_parse (tw->tc, tw->config_file);

}

void add_tab (tilda_window *tw)
{
#ifdef DEBUG
    puts("add_tab");
#endif

    tilda_term *tt;

    tt = (tilda_term *) malloc (sizeof (tilda_term));

    init_tilda_terminal (tw, tt, FALSE);
}

void add_tab_menu_call (gpointer data, guint callback_action, GtkWidget *w)
{
#ifdef DEBUG
    puts("add_tab_menu_call");
#endif

    tilda_window *tw;
    tilda_collect *tc = (tilda_collect *) data;

    tw = tc->tw;

    add_tab (tw);
}

void close_tab (gpointer data, guint callback_action, GtkWidget *w)
{
#ifdef DEBUG
    puts("close_tab");
#endif

    gint pos;
    tilda_term *tt;
    tilda_window *tw;
    tilda_collect *tc = (tilda_collect *) data;

    tw = tc->tw;
    tt = tc->tt;

    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) < 2)
    {
        clean_up (tw);
    }
    else
    {
        pos = gtk_notebook_page_num (GTK_NOTEBOOK (tw->notebook), tt->hbox);
        gtk_notebook_remove_page (GTK_NOTEBOOK (tw->notebook), pos);

        if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) == 1)
            gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), FALSE);

        tw->terms = g_list_remove (tw->terms, tt);
        free (tt);
    }
    
    free (tc);
}

gboolean init_tilda_window (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("init_tilda_window");
#endif

    GtkAccelGroup *accel_group;
    GClosure *clean, *next, *prev, *add;
    GError *error;

    /* Create a window to hold the scrolling shell, and hook its
     * delete event to the quit function.. */
    tw->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_resize_mode (GTK_CONTAINER(tw->window), GTK_RESIZE_IMMEDIATE);
    g_signal_connect (G_OBJECT(tw->window), "delete_event", GTK_SIGNAL_FUNC(deleted_and_quit), tw->window);

    /* Create notebook to hold all terminal widgets */
    tw->notebook = gtk_notebook_new ();
    g_signal_connect (G_OBJECT(tw->window), "show", GTK_SIGNAL_FUNC(focus_term), tw->notebook);

    /* Init GList of all tilda_term structures */
    tw->terms = NULL;
    
    after_command = cfg_getint (tw->tc, "command_exit");
    
    switch (cfg_getint (tw->tc, "tab_pos"))
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
            printf ("Bad tab_pos, not changing anything...\n");
            break;
    }

    gtk_container_add (GTK_CONTAINER(tw->window), tw->notebook);
    gtk_widget_show (tw->notebook);
    
    gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook), cfg_getbool(tw->tc, "notebook_border"));
    
    init_tilda_terminal (tw, tt, TRUE);

    /* Create Accel Group to add key codes for quit, next, prev and new tabs */
    accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (tw->window), accel_group);
    
    /* Exit on Ctrl-Q */
    clean = g_cclosure_new_swap ((GCallback) clean_up, tw, NULL);
    gtk_accel_group_connect (accel_group, 'q', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, clean);

    /* Go to Next Tab */
    next = g_cclosure_new_swap ((GCallback) next_tab, tw, NULL);
    gtk_accel_group_connect (accel_group, 'n', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, next);

    /* Go to Next Tab */
    prev = g_cclosure_new_swap ((GCallback) prev_tab, tw, NULL);
    gtk_accel_group_connect (accel_group, 'p', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, prev);
    
    /* Go to New Tab */
    add = g_cclosure_new_swap ((GCallback) add_tab, tw, NULL);
    gtk_accel_group_connect (accel_group, 't', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, add);
    
    gtk_window_set_decorated ((GtkWindow *) tw->window, FALSE);

    gtk_widget_set_size_request ((GtkWidget *) tw->window, 0, 0);

    if (!g_thread_create ((GThreadFunc) wait_for_signal, tw, FALSE, &error))
       perror ("Fuck that thread!!!");

    return TRUE;
}

