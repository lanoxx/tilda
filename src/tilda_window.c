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
#include <callback_func.h>
#include <configsys.h>
#include <tilda_window.h>
#include <tilda_terminal.h>
#include <key_grabber.h>
#include <translation.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <vte/vte.h>

static void
tilda_window_setup_alpha_mode (tilda_window *tw)
{
    GdkScreen *screen;
    GdkColormap *colormap;

    screen = gtk_widget_get_screen (GTK_WIDGET (tw->window));
    colormap = gdk_screen_get_rgba_colormap (screen);
    if (colormap != NULL && gdk_screen_is_composited (screen))
    {
        /* Set RGBA colormap if possible so VTE can use real alpha
         * channels for transparency. */

        gtk_widget_set_colormap(GTK_WIDGET (tw->window), colormap);
        tw->have_argb_visual = TRUE;
    }
    else
    {
        tw->have_argb_visual = FALSE;
    }
}


static tilda_term* find_tt_in_g_list (tilda_window *tw, gint pos)
{
    DEBUG_FUNCTION ("find_tt_in_g_list");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tw->terms != NULL);

    GtkWidget *page, *current_page;
    GList *terms = g_list_first (tw->terms);
    tilda_term *result=NULL;

    current_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook), pos);

    do
    {
        page = TILDA_TERM(terms->data)->hbox;
        if (page == current_page)
        {
            result = terms->data;
            break;
        }
    } while ((terms = terms->next) != NULL);

    return result;
}

void tilda_window_close_current_tab (tilda_window *tw)
{
    DEBUG_FUNCTION ("close_current_tab");
    DEBUG_ASSERT (tw != NULL);

    gint pos = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));
    tilda_window_close_tab (tw, pos);
}


gint tilda_window_set_tab_position (tilda_window *tw, enum notebook_tab_positions pos)
{
    switch (pos)
    {
        default: /* default is top */
            g_printerr (_("You have a bad tab_pos in your configuration file\n"));
        case NB_TOP:
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_TOP);
            break;
        case NB_BOTTOM:
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_BOTTOM);
            break;
        case NB_LEFT:
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_LEFT);
            break;
        case NB_RIGHT:
            gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_RIGHT);
            break;
    }

    return 0;
}

static gint next_tab (tilda_window *tw)
{
    DEBUG_FUNCTION ("next_tab");
    DEBUG_ASSERT (tw != NULL);

    int num_pages;
    int current_page;

    num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook));
    current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));

    if (num_pages != (current_page + num_pages))
      gtk_notebook_next_page (GTK_NOTEBOOK (tw->notebook));
    else
      gtk_notebook_set_current_page (GTK_NOTEBOOK (tw->notebook), num_pages-1);

    // It worked. Having this return true makes the callback not carry the
    // keystroke into the vte terminal widget.
    return TRUE;
}

static gint prev_tab (tilda_window *tw)
{
    DEBUG_FUNCTION ("prev_tab");
    DEBUG_ASSERT (tw != NULL);

    int num_pages;
    int current_page;

    num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook));
    current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));

    if ((num_pages-1) != current_page)
      gtk_notebook_prev_page (GTK_NOTEBOOK (tw->notebook));
    else
      gtk_notebook_set_current_page (GTK_NOTEBOOK (tw->notebook), 0);

    // It worked. Having this return true makes the callback not carry the
    // keystroke into the vte terminal widget.
    return TRUE;
}

static void focus_term (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("focus_term");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);

    GList *list;
    GtkWidget *box;
    GtkWidget *n = GTK_WIDGET (data);

    box = gtk_notebook_get_nth_page (GTK_NOTEBOOK(n), gtk_notebook_get_current_page(GTK_NOTEBOOK(n)));
    list = gtk_container_get_children (GTK_CONTAINER(box));
    gtk_widget_grab_focus (list->data);
}

static void focus_out_event_cb (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("focus_out_event_cb");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);

    //printf ("hide()\n");
}

static void goto_tab (tilda_window *tw, guint i)
{
    DEBUG_FUNCTION ("goto_tab");
    DEBUG_ASSERT (tw != NULL);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (tw->notebook), i);
}

static gboolean goto_tab_generic (tilda_window *tw, gint tab_number)
{
    DEBUG_FUNCTION ("goto_tab_generic");
    DEBUG_ASSERT (tw != NULL);

    if (g_list_length (tw->terms) > (tab_number-1))
    {
        goto_tab (tw, g_list_length (tw->terms) - tab_number);
        return TRUE;
    }

    return FALSE;
}

/* These all just call the generic function since they're all basically the same
 * anyway. Unfortunately, they can't just be macros, since we need to be able to
 * create a pointer to them for callbacks. */
static gboolean goto_tab_1  (tilda_window *tw) { return goto_tab_generic (tw, 1);  }
static gboolean goto_tab_2  (tilda_window *tw) { return goto_tab_generic (tw, 2);  }
static gboolean goto_tab_3  (tilda_window *tw) { return goto_tab_generic (tw, 3);  }
static gboolean goto_tab_4  (tilda_window *tw) { return goto_tab_generic (tw, 4);  }
static gboolean goto_tab_5  (tilda_window *tw) { return goto_tab_generic (tw, 5);  }
static gboolean goto_tab_6  (tilda_window *tw) { return goto_tab_generic (tw, 6);  }
static gboolean goto_tab_7  (tilda_window *tw) { return goto_tab_generic (tw, 7);  }
static gboolean goto_tab_8  (tilda_window *tw) { return goto_tab_generic (tw, 8);  }
static gboolean goto_tab_9  (tilda_window *tw) { return goto_tab_generic (tw, 9);  }
static gboolean goto_tab_10 (tilda_window *tw) { return goto_tab_generic (tw, 10); }

static gint ccopy (tilda_window *tw)
{
    DEBUG_FUNCTION ("ccopy");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tw->notebook != NULL);

    GtkWidget *current_page;
    GList *list;

    gint pos = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));
    current_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook), pos);
    list = gtk_container_get_children (GTK_CONTAINER(current_page));
    vte_terminal_copy_clipboard (VTE_TERMINAL(list->data));

    /* Stop the event's propagation */
    return TRUE;
}

static gint cpaste (tilda_window *tw)
{
    DEBUG_FUNCTION ("cpaste");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tw->notebook != NULL);

    GtkWidget *current_page;
    GList *list;

    gint pos = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));
    current_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook), pos);
    list = gtk_container_get_children (GTK_CONTAINER (current_page));
    vte_terminal_paste_clipboard (VTE_TERMINAL(list->data));

    /* Stop the event's propagation */
    return TRUE;
}

static gint tilda_window_setup_keyboard_accelerators (tilda_window *tw)
{
    GtkAccelGroup *accel_group;
    GClosure *temp;

    /* Create Accel Group to add key codes for quit, next, prev and new tabs */
    accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (tw->window), accel_group);

    /* Exit on <Ctrl><Shift>q */
    temp = g_cclosure_new_swap (G_CALLBACK(gtk_main_quit), tw, NULL);
    gtk_accel_group_connect (accel_group, 'q', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, temp);

    /* Go to Next Tab on <Ctrl>Page_Down */
    temp = g_cclosure_new_swap (G_CALLBACK(next_tab), tw, NULL);
    gtk_accel_group_connect (accel_group, GDK_Page_Down, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, temp);

    /* Go to Prev Tab on <Ctrl>Page_Up */
    temp = g_cclosure_new_swap (G_CALLBACK(prev_tab), tw, NULL);
    gtk_accel_group_connect (accel_group, GDK_Page_Up, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, temp);

    /* Add New Tab on <Ctrl><Shift>t */
    temp = g_cclosure_new_swap (G_CALLBACK(tilda_window_add_tab), tw, NULL);
    gtk_accel_group_connect (accel_group, 't', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, temp);

    /* Close Current Tab on <Ctrl><Shift>w */
    temp = g_cclosure_new_swap (G_CALLBACK(tilda_window_close_current_tab), tw, NULL);
    gtk_accel_group_connect (accel_group, 'w', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, temp);

    /* Goto Tab # */
    /* Know a better way? Then you do. */
    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_1), tw, NULL);
    gtk_accel_group_connect (accel_group, '1', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_2), tw, NULL);
    gtk_accel_group_connect (accel_group, '2', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_3), tw, NULL);
    gtk_accel_group_connect (accel_group, '3', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_4), tw, NULL);
    gtk_accel_group_connect (accel_group, '4', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_5), tw, NULL);
    gtk_accel_group_connect (accel_group, '5', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_6), tw, NULL);
    gtk_accel_group_connect (accel_group, '6', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_7), tw, NULL);
    gtk_accel_group_connect (accel_group, '7', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_8), tw, NULL);
    gtk_accel_group_connect (accel_group, '8', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_9), tw, NULL);
    gtk_accel_group_connect (accel_group, '9', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(goto_tab_10), tw, NULL);
    gtk_accel_group_connect (accel_group, '0', GDK_MOD1_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(ccopy), tw, NULL);
    gtk_accel_group_connect (accel_group, 'c', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, temp);

    temp = g_cclosure_new_swap (G_CALLBACK(cpaste), tw, NULL);
    gtk_accel_group_connect (accel_group, 'v', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, temp);

    return 0;
}

static gint tilda_window_set_icon (tilda_window *tw, gchar *filename)
{
    GdkPixbuf *window_icon = gdk_pixbuf_new_from_file (filename, NULL);

    if (window_icon == NULL)
    {
        TILDA_PERROR ();
        DEBUG_ERROR ("Cannot open window icon");
        g_printerr (_("Unable to set tilda's icon: %s\n"), filename);
        return 1;
    }

    gtk_window_set_icon (GTK_WINDOW(tw->window), window_icon);
    g_object_unref (window_icon);

    return 0;
}

/**
 * tilda_window_init ()
 *
 * Create a new tilda_window * and return it. It will also initialize and set up
 * as much of the window as possible using the values in the configuation system.
 *
 * @param instance the instance number of this tilda_window
 *
 * Success: return a non-NULL tilda_window *
 * Failure: return NULL
 *
 * Notes: The configuration system must be up and running before calling this function.
 */
tilda_window *tilda_window_init (const gchar *config_file, const gint instance)
{
    DEBUG_FUNCTION ("tilda_window_init");
    DEBUG_ASSERT (instance >= 0);

    tilda_window *tw;

    tw = g_malloc (sizeof(tilda_window));

    if (tw == NULL)
        return NULL;

    /* Set the instance number */
    tw->instance = instance;

    /* Get the user's home directory */
    tw->home_dir = g_strdup (g_get_home_dir ());

    /* Set the config file */
    tw->config_file = g_strdup (config_file);

    /* Create the main window */
    tw->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* Set up all window properties */
    if (config_getbool ("pinned"))
        gtk_window_stick (GTK_WINDOW(tw->window));

    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tw->window), config_getbool ("notaskbar"));
    gtk_window_set_keep_above (GTK_WINDOW(tw->window), config_getbool ("above"));
    gtk_window_set_decorated (GTK_WINDOW(tw->window), FALSE);
    gtk_widget_set_size_request (GTK_WIDGET(tw->window), 0, 0);
    tilda_window_set_icon (tw, g_build_filename (DATADIR, "pixmaps", "tilda.png", NULL));
    tilda_window_setup_alpha_mode (tw);

    /* Add keyboard accelerators */
    tilda_window_setup_keyboard_accelerators (tw);

    /* Create the notebook */
    tw->notebook = gtk_notebook_new ();

    /* Set up all notebook properties */
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK(tw->notebook), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook), config_getbool("notebook_border"));
    tilda_window_set_tab_position (tw, config_getint ("tab_pos"));

    /* Create the linked list of terminals */
    tw->terms = NULL;

    /* Add the initial terminal */
    if (!tilda_window_add_tab (tw))
    {
        free (tw);
        return NULL;
    }

    /* Connect signal handlers */
    g_signal_connect (G_OBJECT(tw->window), "delete_event", GTK_SIGNAL_FUNC(gtk_main_quit), tw->window);
    g_signal_connect (G_OBJECT(tw->window), "show", GTK_SIGNAL_FUNC(focus_term), tw->notebook);

    g_signal_connect (G_OBJECT(tw->window), "focus-out-event", GTK_SIGNAL_FUNC(focus_out_event_cb), tw->window);

    /* Add the notebook to the window */
    gtk_container_add (GTK_CONTAINER(tw->window), tw->notebook);

    /* Show the widgets */
    gtk_widget_show (tw->notebook);
    /* the tw->window widget will be shown later, by pull() */

    /* Position the window */
    tw->current_state = UP;
    gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    gtk_window_set_default_size (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    generate_animation_positions (tw);

    return tw;
}

gint tilda_window_free (tilda_window *tw)
{
    gint num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK(tw->notebook));

    /* Close each tab which still exists.
     * This will free their data structures automatically. */
    while (num_pages > 0)
    {
        /* Close the 0th tab, which should always exist while we have
         * some pages left in the notebook. */
        tilda_window_close_tab (tw, 0);

        num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK(tw->notebook));
    }

    g_free (tw->config_file);
    g_free (tw->home_dir);

    g_free (tw);

    return 0;
}

/**
 * tilda_window_add_tab ()
 *
 * Create and add a new tab at the end of the notebook
 *
 * Success: the new tab's index (>=0)
 * Failure: -1
 */
gint tilda_window_add_tab (tilda_window *tw)
{
    DEBUG_FUNCTION ("tilda_window_add_tab");
    DEBUG_ASSERT (tw != NULL);

    tilda_term *tt;
    GtkWidget *label;
    gint index;

    tt = tilda_term_init (tw);

    if (tt == NULL)
    {
        TILDA_PERROR ();
        g_printerr (_("Out of memory, cannot create tab\n"));
        return FALSE;
    }

    /* Create page and append to notebook */
    label = gtk_label_new ("Tilda");
    /* Strangely enough, prepend puts pages on the end */
    index = gtk_notebook_prepend_page (GTK_NOTEBOOK(tw->notebook), tt->hbox, label);
    gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK(tw->notebook), tt->hbox, TRUE, TRUE, GTK_PACK_END);
    gtk_notebook_set_current_page (GTK_NOTEBOOK(tw->notebook), index);

    /* We should show the tabs if there are more than one tab in the notebook */
    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) > 1)
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), TRUE);

    /* Add to GList list of tilda_term structures in tilda_window structure */
    tw->terms = g_list_append (tw->terms, tt);

    /* The new terminal should grab the focus automatically */
    gtk_widget_grab_focus (tt->vte_term);

    return TRUE; //index;
}

/**
 * tilda_window_close_tab ()
 *
 * Closes the tab at the given tab index (starting from 0)
 *
 * Success: return 0
 * Failure: return non-zero
 */
gint tilda_window_close_tab (tilda_window *tw, gint tab_index)
{
    DEBUG_FUNCTION ("tilda_window_close_tab");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tab_index >= 0);

    tilda_term *tt;
    GtkWidget *child;

    child = gtk_notebook_get_nth_page (GTK_NOTEBOOK(tw->notebook), tab_index);

    if (child == NULL)
    {
        DEBUG_ERROR ("Bad tab_index specified");
        return -1;
    }

    tt = find_tt_in_g_list (tw, tab_index);

    gtk_notebook_remove_page (GTK_NOTEBOOK (tw->notebook), tab_index);

    /* We should hide the tabs if there is only one tab left */
    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) == 1)
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), FALSE);

    /* With no pages left, it's time to leave the program */
    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) < 1)
        gtk_main_quit ();

    /* Remove the tilda_term from the list of terminals */
    tw->terms = g_list_remove (tw->terms, tt);

    /* Free the terminal, we are done with it */
    tilda_term_free (tt);

    return 0;
}

