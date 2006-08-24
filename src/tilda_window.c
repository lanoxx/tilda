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
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
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
    CFG_STR("command", "", CFGF_NONE),
    CFG_STR("font", "Monospace 13", CFGF_NONE),
    CFG_STR("key", NULL, CFGF_NONE),
    CFG_STR("title", "Tilda", CFGF_NONE),
    CFG_STR("background_color", "white", CFGF_NONE),
    CFG_STR("working_dir", NULL, CFGF_NONE),

    /* ints */
    CFG_INT("lines", 100, CFGF_NONE),
    CFG_INT("max_width", 600, CFGF_NONE),
    CFG_INT("max_height", 150, CFGF_NONE),
    CFG_INT("min_width", 1, CFGF_NONE),
    CFG_INT("min_height", 1, CFGF_NONE),
    CFG_INT("transparency", 0, CFGF_NONE),
    CFG_INT("x_pos", 0, CFGF_NONE),
    CFG_INT("y_pos", 0, CFGF_NONE),
    CFG_INT("tab_pos", 0, CFGF_NONE),
    CFG_INT("backspace_key", 0, CFGF_NONE),
    CFG_INT("delete_key", 1, CFGF_NONE),
    CFG_INT("d_set_title", 3, CFGF_NONE),
    CFG_INT("command_exit", 0, CFGF_NONE),
    CFG_INT("scheme", 3, CFGF_NONE),
    CFG_INT("slide_sleep_usec", 15000, CFGF_NONE),
    CFG_INT("animation_orientation", 0, CFGF_NONE),

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
    CFG_BOOL("animation", TRUE, CFGF_NONE),
    CFG_BOOL("hidden", FALSE, CFGF_NONE),
    CFG_BOOL("centered_horizontally", FALSE, CFGF_NONE),
    CFG_BOOL("centered_vertically", FALSE, CFGF_NONE),
    CFG_END()
};

/**
 * Get a pointer to the config file name for this instance.
 *
 * NOTE: you MUST call free() on the returned value!!!
 *
 * @param tw the tilda_window structure corresponding to this instance
 * @return a pointer to a string representation of the config file's name
 */
static gchar* get_config_file_name (tilda_window *tw)
{
    gchar *config_file;
    gchar instance_str[6];
    gchar *config_prefix = "/.tilda/config_";
    gint config_file_size = 0;

    /* Get a string form of the instance */
    g_snprintf (instance_str, sizeof(instance_str), "%d", tw->instance);

    /* Calculate the config_file variable's size */
    config_file_size = strlen (tw->home_dir) + strlen (config_prefix) + strlen (instance_str) + 1;

    /* Allocate the config_file variable */
    if ((config_file = (gchar*) malloc (config_file_size * sizeof(gchar))) == NULL)
        print_and_exit ("Out of memory, exiting...", 1);

    /* Store the config file name in the allocated space */
    g_snprintf (config_file, config_file_size, "%s%s%s", tw->home_dir, config_prefix, instance_str);

    return config_file;
}

/**
 * Set up tw->tc to hold all of the values in tilda's config.
 * Gets the tw->instance number.
 * Sets tw->config_file.
 * Parses tw->config_file into tw->tc.
 *
 * @param tw the tilda_window in which to store the config
 */
void init_tilda_window_instance (tilda_window *tw)
{
#ifdef DEBUG
    puts("init_tilda_window_instance");
#endif

    /* Get the instance number for this tilda, and store it in tw->instance.
     * Also create the lock file for this instance. */
    getinstance (tw);

    /* Get and store the config file's name */
    tw->config_file = get_config_file_name (tw);

    /* Set up the default config dictionary */
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

    add_tab (((tilda_collect *) data)->tw);
}

static tilda_term* find_tt_in_g_list (tilda_window *tw, gint pos)
{
#ifdef DEBUG
    puts("find_tt_in_g_list");
#endif
    
    GtkWidget *page, *current_page;
    int i, list_length;
    GList *terms = g_list_first (tw->terms);
    tilda_term *result=NULL;
    
    current_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook), pos);
    
    do
    {
        page = ((tilda_term *) terms->data)->hbox;
        if (page == current_page)
        {
            result = terms->data;
            break;
        }
    } while ((terms = terms->next) != NULL);
    
    return result;
}

void close_current_tab (tilda_window *tw)
{
#ifdef DEBUG
    puts("close_current_tab");
#endif

    gint pos;
    tilda_term *tt;

    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) < 2)
    {
        clean_up (tw);
    }
    else
    {
        pos = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));
        if ((tt = find_tt_in_g_list (tw, pos)) != NULL)
        {
            gtk_notebook_remove_page (GTK_NOTEBOOK (tw->notebook), pos);

            if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) == 1)
                gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), FALSE);
        
            tw->terms = g_list_remove (tw->terms, tt);
            free (tt);
        } 
    }
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
    GClosure *clean, *close, *next, *prev, *add, *copy_closure, *paste_closure;
    GClosure *goto_tab_closure_1, *goto_tab_closure_2, *goto_tab_closure_3, *goto_tab_closure_4;
    GClosure *goto_tab_closure_5, *goto_tab_closure_6, *goto_tab_closure_7, *goto_tab_closure_8;
    GClosure *goto_tab_closure_9, *goto_tab_closure_10;
    GError *error;

    GdkPixbuf *window_icon;
    const gchar window_icon_file[] = "/usr/share/pixmaps/tilda.png"; // A better way to get this path?

    /* Create a window to hold the scrolling shell, and hook its
     * delete event to the quit function.. */
    tw->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_resize_mode (GTK_CONTAINER(tw->window), GTK_RESIZE_IMMEDIATE);
    g_signal_connect (G_OBJECT(tw->window), "delete_event", GTK_SIGNAL_FUNC(deleted_and_quit), tw->window);

    /* Create notebook to hold all terminal widgets */
    tw->notebook = gtk_notebook_new ();
    gtk_notebook_set_homogeneous_tabs (GTK_NOTEBOOK(tw->notebook), TRUE);
    g_signal_connect (G_OBJECT(tw->window), "show", GTK_SIGNAL_FUNC(focus_term), tw->notebook);

    /* Init GList of all tilda_term structures */
    tw->terms = NULL;

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
            fprintf (stderr, "Bad tab_pos, not changing anything...\n");
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
    gtk_accel_group_connect (accel_group, 'q', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, clean);

    /* Go to Next Tab */
    next = g_cclosure_new_swap ((GCallback) next_tab, tw, NULL);
    gtk_accel_group_connect (accel_group, GDK_Page_Up, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, next);

    /* Go to Prev Tab */
    prev = g_cclosure_new_swap ((GCallback) prev_tab, tw, NULL);
    gtk_accel_group_connect (accel_group, GDK_Page_Down, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, prev);

    /* Go to New Tab */
    add = g_cclosure_new_swap ((GCallback) add_tab, tw, NULL);
    gtk_accel_group_connect (accel_group, 't', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, add);

    /* Delete Current Tab */
    close = g_cclosure_new_swap ((GCallback) close_current_tab, tw, NULL);
    gtk_accel_group_connect (accel_group, 'w', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, close);    

    /* Goto Tab # */
    /* Know a better way? Then you do. */
    goto_tab_closure_1 = g_cclosure_new_swap ((GCallback) goto_tab_1, tw, NULL);
    gtk_accel_group_connect (accel_group, '1', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_1);            
    
    goto_tab_closure_2 = g_cclosure_new_swap ((GCallback) goto_tab_2, tw, NULL);
    gtk_accel_group_connect (accel_group, '2', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_2);            
    
    goto_tab_closure_3 = g_cclosure_new_swap ((GCallback) goto_tab_3, tw, NULL);
    gtk_accel_group_connect (accel_group, '3', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_3);            
    
    goto_tab_closure_4 = g_cclosure_new_swap ((GCallback) goto_tab_4, tw, NULL);
    gtk_accel_group_connect (accel_group, '4', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_4);            
    
    goto_tab_closure_5 = g_cclosure_new_swap ((GCallback) goto_tab_5, tw, NULL);
    gtk_accel_group_connect (accel_group, '5', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_5);            
    
    goto_tab_closure_6 = g_cclosure_new_swap ((GCallback) goto_tab_6, tw, NULL);
    gtk_accel_group_connect (accel_group, '6', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_6);            
    
    goto_tab_closure_7 = g_cclosure_new_swap ((GCallback) goto_tab_7, tw, NULL);
    gtk_accel_group_connect (accel_group, '7', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_7);            
    
    goto_tab_closure_8 = g_cclosure_new_swap ((GCallback) goto_tab_8, tw, NULL);
    gtk_accel_group_connect (accel_group, '8', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_8);            
    
    goto_tab_closure_9 = g_cclosure_new_swap ((GCallback) goto_tab_9, tw, NULL);
    gtk_accel_group_connect (accel_group, '9', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_9);            
    
    goto_tab_closure_10 = g_cclosure_new_swap ((GCallback) goto_tab_10, tw, NULL);
    gtk_accel_group_connect (accel_group, '0', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, goto_tab_closure_10);            
    
    copy_closure = g_cclosure_new_swap ((GCallback) ccopy, tw, NULL);
    gtk_accel_group_connect (accel_group, 'c', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, copy_closure);            
    
    paste_closure = g_cclosure_new_swap ((GCallback) cpaste, tw, NULL);
    gtk_accel_group_connect (accel_group, 'v', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, paste_closure);            
     
    gtk_window_set_decorated (GTK_WINDOW(tw->window), FALSE);

    /*  Set a window icon! */
    window_icon = gdk_pixbuf_new_from_file (window_icon_file, NULL);
    gtk_window_set_icon (GTK_WINDOW(tw->window), window_icon);
    g_object_unref (window_icon);

    gtk_widget_set_size_request (GTK_WIDGET(tw->window), 0, 0);

    if (!g_thread_create ((GThreadFunc) wait_for_signal, tw, FALSE, &error))
       perror ("Fuck that thread!!!");

    return TRUE;
}

