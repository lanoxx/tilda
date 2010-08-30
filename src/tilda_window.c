

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


gint toggle_fullscreen_cb (tilda_window *tw)
{
    DEBUG_FUNCTION ("toggle_fullscreen_cb");
    DEBUG_ASSERT (tw != NULL);
    
	if (tw->fullscreen != TRUE) {
		tw->fullscreen = TRUE;
		gtk_window_fullscreen (GTK_WINDOW (tw->window));
	}
	else {
		gtk_window_unfullscreen (GTK_WINDOW (tw->window));
		tw->fullscreen = FALSE;
	}
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
    tilda_window *tw = TILDA_WINDOW(data);
    GtkWidget *n = GTK_WIDGET (tw->notebook);

    box = gtk_notebook_get_nth_page (GTK_NOTEBOOK(n), gtk_notebook_get_current_page(GTK_NOTEBOOK(n)));
    list = gtk_container_get_children (GTK_CONTAINER(box));
    gtk_widget_grab_focus (list->data);
}

static gboolean auto_hide_tick(gpointer data)
{
    DEBUG_FUNCTION ("auto_hide_tick");
    DEBUG_ASSERT (data != NULL);
    
    tilda_window *tw = TILDA_WINDOW(data);
    tw->auto_hide_current_time += tw->timer_resolution;
    if ((tw->auto_hide_current_time >= tw->auto_hide_max_time) || tw->current_state == UP)
    {
        pull(tw, PULL_UP);
        tw->auto_hide_tick_handler = 0;
        return FALSE;
    }
    return TRUE;
}

/* Start auto hide tick */
static void start_auto_hide_tick(tilda_window *tw)
{
    if ((tw->auto_hide_tick_handler == 0) && (tw->disable_auto_hide == FALSE))
    {
        tw->auto_hide_current_time = 0;
        tw->auto_hide_tick_handler = g_timeout_add(tw->timer_resolution, auto_hide_tick, tw);
    }
}

/* Stop auto hide tick */
static void stop_auto_hide_tick(tilda_window *tw)
{
    if (tw->auto_hide_tick_handler != 0)
    {
        g_source_remove(tw->auto_hide_tick_handler);
        tw->auto_hide_tick_handler = 0;
    }
}

static void mouse_enter (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    DEBUG_FUNCTION ("mouse_enter");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);
    
    tilda_window *tw = TILDA_WINDOW(data);
    stop_auto_hide_tick(tw);
    if (tw->disable_auto_hide == FALSE)
        tilda_window_set_active(tw);
}
 
static void mouse_leave (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    DEBUG_FUNCTION ("mouse_leave");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);
    
    GdkEventCrossing *ev = (GdkEventCrossing*)event;
    tilda_window *tw = TILDA_WINDOW(data);
    
    if ((ev->mode != GDK_CROSSING_NORMAL) || (tw->auto_hide_on_mouse_leave == FALSE))
        return;
    
    start_auto_hide_tick(tw);
}

static void focus_out_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    DEBUG_FUNCTION ("focus_out_event_cb");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);
    
    tilda_window *tw = TILDA_WINDOW(data);
    
    if (tw->auto_hide_on_focus_lost == FALSE)
        return;
    
    start_auto_hide_tick(tw);
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

/* Tie a single keyboard shortcut to a callback function */
gint tilda_add_config_accelerator(const gchar* key, GCallback callback_func, tilda_window *tw)
{
    guint accel_key;
    GdkModifierType accel_mods;
    GClosure *temp;
 
    gtk_accelerator_parse (config_getstr(key), &accel_key, &accel_mods);
    if (! ((accel_key == 0) && (accel_mods == 0)) )  // make sure it parsed properly
    {
        temp = g_cclosure_new_swap (callback_func, tw, NULL);
        gtk_accel_group_connect (tw->accel_group, accel_key, accel_mods , GTK_ACCEL_VISIBLE, temp);
    }
 
    return 0;
}
 
gint tilda_window_setup_keyboard_accelerators (tilda_window *tw)
{
 
    /* If we already have an tw->accel_group (which would happen if we're redefining accelerators in the config window)
       we want to remove it before creating a new one. */
    if (tw->accel_group != NULL)
        gtk_window_remove_accel_group (GTK_WINDOW (tw->window), tw->accel_group);
 
    /* Create Accel Group to add key codes for quit, next, prev and new tabs */
    tw->accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (tw->window), tw->accel_group);
 
    /* Set up keyboard shortcuts for Exit, Next Tab, Previous Tab, Add Tab,
       Close Tab, Copy, and Paste using key combinations defined in the config. */
    tilda_add_config_accelerator("quit_key",     G_CALLBACK(gtk_main_quit),                  tw);
    tilda_add_config_accelerator("nexttab_key",  G_CALLBACK(next_tab),                       tw);
    tilda_add_config_accelerator("prevtab_key",  G_CALLBACK(prev_tab),                       tw);
    tilda_add_config_accelerator("addtab_key",   G_CALLBACK(tilda_window_add_tab),           tw);
    tilda_add_config_accelerator("closetab_key", G_CALLBACK(tilda_window_close_current_tab), tw);
    tilda_add_config_accelerator("copy_key",     G_CALLBACK(ccopy),                          tw);
    tilda_add_config_accelerator("paste_key",    G_CALLBACK(cpaste),                         tw);
    tilda_add_config_accelerator("fullscreen_key",    G_CALLBACK(toggle_fullscreen_cb),             tw);
 
    /* Set up keyboard shortcuts for Goto Tab # using key combinations defined in the config*/
    /* Know a better way? Then you do. */
    tilda_add_config_accelerator("gototab_1_key",  G_CALLBACK(goto_tab_1),  tw);
    tilda_add_config_accelerator("gototab_2_key",  G_CALLBACK(goto_tab_2),  tw);
    tilda_add_config_accelerator("gototab_3_key",  G_CALLBACK(goto_tab_3),  tw);
    tilda_add_config_accelerator("gototab_4_key",  G_CALLBACK(goto_tab_4),  tw);
    tilda_add_config_accelerator("gototab_5_key",  G_CALLBACK(goto_tab_5),  tw);
    tilda_add_config_accelerator("gototab_6_key",  G_CALLBACK(goto_tab_6),  tw);
    tilda_add_config_accelerator("gototab_7_key",  G_CALLBACK(goto_tab_7),  tw);
    tilda_add_config_accelerator("gototab_8_key",  G_CALLBACK(goto_tab_8),  tw);
    tilda_add_config_accelerator("gototab_9_key",  G_CALLBACK(goto_tab_9),  tw);
    tilda_add_config_accelerator("gototab_10_key", G_CALLBACK(goto_tab_10), tw);
    
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

    /* Get the user's config and cache directory 
     * Comply with FreeDesktop XDG Spec
     */
    tw->user_config_home = g_build_filename(g_get_user_config_dir (), "tilda", NULL);
    tw->user_cache_home = g_build_filename(g_get_user_cache_dir (), "tilda", NULL);

    /* Set the config file */
    tw->config_file = g_strdup (config_file);

    /* Create the main window */
    tw->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
    /* Generic timer resolution */
    tw->timer_resolution = config_getint("timer_resolution");
    
    /* Auto hide support */
    tw->auto_hide_tick_handler = 0;
    tw->auto_hide_max_time = config_getint("auto_hide_time");
    tw->auto_hide_on_mouse_leave = config_getbool("auto_hide_on_mouse_leave");
    tw->auto_hide_on_focus_lost = config_getbool("auto_hide_on_focus_lost");
    tw->disable_auto_hide = FALSE;

	tw->fullscreen = FALSE;

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
    tw->accel_group = NULL; /* We can redefine the accelerator group from the wizard; this shows that it's our first time defining it. */
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
    g_signal_connect (G_OBJECT(tw->window), "delete_event", GTK_SIGNAL_FUNC(gtk_main_quit), tw);
    g_signal_connect (G_OBJECT(tw->window), "show", GTK_SIGNAL_FUNC(focus_term), tw);
    g_signal_connect (G_OBJECT(tw->window), "focus-out-event", GTK_SIGNAL_FUNC(focus_out_event_cb), tw);
    g_signal_connect (G_OBJECT(tw->window), "enter-notify-event", GTK_SIGNAL_FUNC(mouse_enter), tw);
    g_signal_connect (G_OBJECT(tw->window), "leave-notify-event", GTK_SIGNAL_FUNC(mouse_leave), tw);

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

	/* Create GDK resources now, to prevent crashes later on */
    gtk_widget_realize (tw->window);
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
    g_free (tw->user_cache_home);
    g_free (tw->user_config_home);

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
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(tw->notebook), tt->hbox, TRUE);

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

