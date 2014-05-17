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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <vte/vte.h>

static void
tilda_window_setup_alpha_mode (tilda_window *tw)
{
    GdkScreen *screen;
    GdkVisual *visual;

    screen = gtk_widget_get_screen (GTK_WIDGET (tw->window));
    visual = gdk_screen_get_rgba_visual (screen);
    if (visual == NULL) {
        visual = gdk_screen_get_system_visual (screen);
    }
    if (visual != NULL && gdk_screen_is_composited (screen)) {
        /* Set RGBA colormap if possible so VTE can use real alpha
         * channels for transparency. */

        gtk_widget_set_visual (GTK_WIDGET (tw->window), visual);
        tw->have_argb_visual = TRUE;
    } else {
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
    tilda_window_close_tab (tw, pos, FALSE);
}


gint tilda_window_set_tab_position (tilda_window *tw, enum notebook_tab_positions pos)
{
    const gint gtk_pos[] = { GTK_POS_TOP, GTK_POS_BOTTOM, GTK_POS_LEFT, GTK_POS_RIGHT };

    if ((pos < 0) || (pos > 3)) {
        g_printerr (_("You have a bad tab_pos in your configuration file\n"));
        pos = NB_TOP;
    }

    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), gtk_pos[pos]);

    return 0;
}


gint toggle_fullscreen_cb (tilda_window *tw)
{
    DEBUG_FUNCTION ("toggle_fullscreen_cb");
    DEBUG_ASSERT (tw != NULL);

    if (tw->fullscreen != TRUE) {
        gtk_window_fullscreen (GTK_WINDOW (tw->window));
    }
    else {
        gtk_window_unfullscreen (GTK_WINDOW (tw->window));
        // This appears to be necssary on (at least) xfwm4 if you tabbed out
        // while fullscreened.
        gtk_window_set_default_size (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
        gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    }
    tw->fullscreen = !tw->fullscreen;

    // It worked. Having this return GDK_EVENT_STOP makes the callback not carry the
    // keystroke into the vte terminal widget.
    return GDK_EVENT_STOP;
}

gint tilda_window_next_tab (tilda_window *tw)
{
    DEBUG_FUNCTION ("next_tab");
    DEBUG_ASSERT (tw != NULL);

    int num_pages;
    int current_page;

    /* If we are on the last page, go to first page */
    num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook));
    current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));

    if ((num_pages - 1) == current_page)
      gtk_notebook_set_current_page (GTK_NOTEBOOK (tw->notebook), 0);
    else
      gtk_notebook_next_page (GTK_NOTEBOOK (tw->notebook));

    // It worked. Having this return GDK_EVENT_STOP makes the callback not carry the
    // keystroke into the vte terminal widget.
    return GDK_EVENT_STOP;
}

gint tilda_window_prev_tab (tilda_window *tw)
{
    DEBUG_FUNCTION ("prev_tab");
    DEBUG_ASSERT (tw != NULL);

    int num_pages;
    int current_page;

    num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook));
    current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));

    if (current_page == 0)
        gtk_notebook_set_current_page (GTK_NOTEBOOK (tw->notebook), (num_pages -1));
    else
      gtk_notebook_prev_page (GTK_NOTEBOOK (tw->notebook));

    // It worked. Having this return GDK_EVENT_STOP makes the callback not carry the
    // keystroke into the vte terminal widget.
    return GDK_EVENT_STOP;
}

enum tab_direction { TAB_LEFT = 1, TAB_RIGHT = -1 };

static gint move_tab (tilda_window *tw, int direction)
{
    DEBUG_FUNCTION ("move_tab");
    DEBUG_ASSERT (tw != NULL);

    int num_pages;
    int current_page_index;
    int new_page_index;
    GtkWidget* current_page;

    num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook));

    if (num_pages > 1) {
        current_page_index = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));
        current_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook),
                                                  current_page_index);

        /* wrap over if new_page_index over-/underflows */
        new_page_index = (current_page_index + direction) % num_pages;

        gtk_notebook_reorder_child (GTK_NOTEBOOK (tw->notebook), current_page,
                                    new_page_index);
    }

    // It worked. Having this return GDK_EVENT_STOP makes the callback not carry the
    // keystroke into the vte terminal widget.
    return GDK_EVENT_STOP;

}
static gint move_tab_left (tilda_window *tw)
{
    DEBUG_FUNCTION ("move_tab_left");
    return move_tab(tw, LEFT);
}

static gint move_tab_right (tilda_window *tw)
{
    DEBUG_FUNCTION ("move_tab_right");
    return move_tab(tw, RIGHT);
}

static gboolean focus_term (GtkWidget *widget, gpointer data)
{
    DEBUG_FUNCTION ("focus_term");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);

    GList *list;
    GtkWidget *box;
    tilda_window *tw = TILDA_WINDOW(data);
    GtkWidget *n = GTK_WIDGET (tw->notebook);

    box = gtk_notebook_get_nth_page (GTK_NOTEBOOK(n), gtk_notebook_get_current_page(GTK_NOTEBOOK(n)));
    if (box != NULL) {
        list = gtk_container_get_children (GTK_CONTAINER(box));
        gtk_widget_grab_focus (list->data);
    }

    // It worked. Having this return GDK_EVENT_STOP makes the callback not carry the
    // keystroke into the vte terminal widget.
    return GDK_EVENT_STOP;
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
    // If the delay is less or equal to 1000ms then the timer is not precise
    // enough, because it takes already about 200ms to register it, so we
    // rather sleep for the given amount of time.
    const guint32 MAX_SLEEP_TIME = 1000;

    if ((tw->auto_hide_tick_handler == 0) && (tw->disable_auto_hide == FALSE)) {
        /* If there is not timer registered yet, then the auto_hide_tick_handler
         * has a value of zero. Next we need to make sure that the auto hide
         * max time is greater then zero, or else we can pull up immediately,
         * without the trouble of registering a timer.
         */
        if (tw->auto_hide_max_time > MAX_SLEEP_TIME) {
            tw->auto_hide_current_time = 0;
            tw->auto_hide_tick_handler = g_timeout_add(tw->timer_resolution, auto_hide_tick, tw);
        } else if (tw->auto_hide_max_time <= MAX_SLEEP_TIME) {
            // auto_hide_max_time is in milli seconds, so we need to convert to
            // microseconds by multiplying with 1000
            g_usleep (tw->auto_hide_max_time * 1000);
            pull(tw, PULL_UP);
        } else {
            pull(tw, PULL_UP);
        }
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

static gboolean mouse_enter (GtkWidget *widget, G_GNUC_UNUSED GdkEvent *event, gpointer data)
{
    DEBUG_FUNCTION ("mouse_enter");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);

    tilda_window *tw = TILDA_WINDOW(data);
    stop_auto_hide_tick(tw);

    return GDK_EVENT_STOP;
}

static gboolean mouse_leave (GtkWidget *widget, G_GNUC_UNUSED GdkEvent *event, gpointer data)
{
    DEBUG_FUNCTION ("mouse_leave");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);

    GdkEventCrossing *ev = (GdkEventCrossing*)event;
    tilda_window *tw = TILDA_WINDOW(data);

    if ((ev->mode != GDK_CROSSING_NORMAL) || (tw->auto_hide_on_mouse_leave == FALSE))
        return GDK_EVENT_STOP;

    start_auto_hide_tick(tw);

    return GDK_EVENT_STOP;
}

static gboolean focus_out_event_cb (GtkWidget *widget, G_GNUC_UNUSED GdkEvent *event, gpointer data)
{
    DEBUG_FUNCTION ("focus_out_event_cb");
    DEBUG_ASSERT (data != NULL);
    DEBUG_ASSERT (widget != NULL);

    tilda_window *tw = TILDA_WINDOW(data);

    if (tw->auto_hide_on_focus_lost == FALSE)
        return GDK_EVENT_PROPAGATE;

    start_auto_hide_tick(tw);

    return GDK_EVENT_PROPAGATE;
}

static void goto_tab (tilda_window *tw, guint i)
{
    DEBUG_FUNCTION ("goto_tab");
    DEBUG_ASSERT (tw != NULL);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (tw->notebook), i);
}

static gboolean goto_tab_generic (tilda_window *tw, guint tab_number)
{
    DEBUG_FUNCTION ("goto_tab_generic");
    DEBUG_ASSERT (tw != NULL);

    if (g_list_length (tw->terms) > (tab_number-1))
    {
        goto_tab (tw, tab_number - 1);
    }

    return TRUE;
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
static gint tilda_add_config_accelerator(const gchar* key, GCallback callback_func, tilda_window *tw)
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

    /* Set up keyboard shortcuts for Exit, Next Tab, Previous Tab,
       Move Tab, Add Tab, Close Tab, Copy, and Paste using key
       combinations defined in the config. */
    tilda_add_config_accelerator("quit_key",         G_CALLBACK(gtk_main_quit),                  tw);
    tilda_add_config_accelerator("nexttab_key",      G_CALLBACK(tilda_window_next_tab),          tw);
    tilda_add_config_accelerator("prevtab_key",      G_CALLBACK(tilda_window_prev_tab),          tw);
    tilda_add_config_accelerator("movetableft_key",  G_CALLBACK(move_tab_left),                  tw);
    tilda_add_config_accelerator("movetabright_key", G_CALLBACK(move_tab_right),                 tw);
    tilda_add_config_accelerator("addtab_key",       G_CALLBACK(tilda_window_add_tab),           tw);
    tilda_add_config_accelerator("closetab_key",     G_CALLBACK(tilda_window_close_current_tab), tw);
    tilda_add_config_accelerator("copy_key",         G_CALLBACK(ccopy),                          tw);
    tilda_add_config_accelerator("paste_key",        G_CALLBACK(cpaste),                         tw);
    tilda_add_config_accelerator("fullscreen_key",   G_CALLBACK(toggle_fullscreen_cb),           tw);

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

static gboolean delete_event_callback (G_GNUC_UNUSED GtkWidget *widget,
                                G_GNUC_UNUSED GdkEvent  *event,
                                G_GNUC_UNUSED gpointer   user_data)
{
    gtk_main_quit ();
    return FALSE;
}

gboolean tilda_window_init (const gchar *config_file, const gint instance, tilda_window *tw)
{
    DEBUG_FUNCTION ("tilda_window_init");
    DEBUG_ASSERT (instance >= 0);

    GtkCssProvider *provider;
    GtkStyleContext *style_context;

    /* Set the instance number */
    tw->instance = instance;

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

    /* Here we setup the CSS settings for the GtkNotebook.
     * If the option "Show notebook border" in the preferences is not
     * checked. Then we disable the border. Otherwise nothing is changed.
     *
     * Depending on the theme it might be necessary to set different
     * CSS rules, so it is not possible to find a solution that fits
     * everyone. The following configuration works well with the GNOME
     * default theme Adwaita, but for example has problems under Ubuntu.
     * Note that for bigger modifications the user can create a style.css
     * file in tildas config directory, which will be parsed by
     * load_custom_css_file() in tilda.c on start up.
     */
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK(tw->notebook), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook),
        config_getbool("notebook_border"));
    gtk_notebook_set_scrollable (GTK_NOTEBOOK(tw->notebook), TRUE);
    tilda_window_set_tab_position (tw, config_getint ("tab_pos"));

    provider = gtk_css_provider_new ();
    style_context = gtk_widget_get_style_context(tw->notebook);
    gtk_style_context_add_provider (style_context,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    if(!config_getbool("notebook_border")) {
        /**
         * Calling gtk_notebook_set_show_border is not enough. We need to
         * disable the border explicitly by using CSS.
         */
        gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (provider),
                                " .notebook {\n"
                                "   border: none;\n"
                                "}\n", -1, NULL);
    }

    g_object_unref (provider);

    /* Create the linked list of terminals */
    tw->terms = NULL;

    /* Add the initial terminal */
    if (!tilda_window_add_tab (tw))
    {
        return FALSE;
    }

    /* This is required in key_grabber.c to get the x11 server time,
     * since the specification requires this flag to be set when
     * gdk_x11_get_server_time() is called.
     **/
    gtk_widget_add_events (tw->window, GDK_PROPERTY_CHANGE_MASK );

    /* Connect signal handlers */
    g_signal_connect (G_OBJECT(tw->window), "delete-event", G_CALLBACK (delete_event_callback), tw);
    g_signal_connect (G_OBJECT(tw->window), "show", G_CALLBACK (focus_term), tw);

    g_signal_connect (G_OBJECT(tw->window), "focus-out-event", G_CALLBACK (focus_out_event_cb), tw);
    g_signal_connect (G_OBJECT(tw->window), "enter-notify-event", G_CALLBACK (mouse_enter), tw);
    g_signal_connect (G_OBJECT(tw->window), "leave-notify-event", G_CALLBACK (mouse_leave), tw);

    /* Add the notebook to the window */
    gtk_container_add (GTK_CONTAINER(tw->window), tw->notebook);

    /* Show the widgets */
    gtk_widget_show (tw->notebook);
    /* the tw->window widget will be shown later, by pull() */

    /* Position the window */
    tw->current_state = UP;
    gtk_window_set_default_size (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    /* Create GDK resources now, to prevent crashes later on */
    gtk_widget_realize (tw->window);
    generate_animation_positions (tw);

    return TRUE;
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
        tilda_window_close_tab (tw, 0, TRUE);

        num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK(tw->notebook));
    }

    g_free (tw->config_file);

    return 0;
}

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
    index = gtk_notebook_append_page (GTK_NOTEBOOK(tw->notebook), tt->hbox, label);
    gtk_notebook_set_current_page (GTK_NOTEBOOK(tw->notebook), index);
    gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK(tw->notebook), tt->hbox, TRUE);

    /* We should show the tabs if there are more than one tab in the notebook */
    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) > 1)
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), TRUE);

    /* Add to GList list of tilda_term structures in tilda_window structure */
    tw->terms = g_list_append (tw->terms, tt);

    /* The new terminal should grab the focus automatically */
    gtk_widget_grab_focus (tt->vte_term);

    return TRUE; //index;
}

gint tilda_window_close_tab (tilda_window *tw, gint tab_index, gboolean force_exit)
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

    /* With no pages left, either leave the program or create a new
     * terminal */
    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) < 1) {
        if (force_exit == TRUE) {
            /**
             * It is necessary to check the main_level because if CTRL_C was used
             * or the "Quit" option from the context menu then gtk_main_quit has
             * already been called and cannot call it again.
             */
            if(gtk_main_level () > 0) {
                gtk_main_quit ();
            }
        } else {
            /* These can stay here. They don't need to go into a header
             * because they are only used at this point in the code. */
            enum on_last_terminal_exit { EXIT_TILDA,
                                         RESTART_TERMINAL,
                                         RESTART_TERMINAL_AND_HIDE };

            /* Check the user's preference for what to do when the last
             * terminal is closed. Take the appropriate action */
            switch (config_getint ("on_last_terminal_exit"))
            {
                case RESTART_TERMINAL:
                    tilda_window_add_tab (tw);
                    break;
                case RESTART_TERMINAL_AND_HIDE:
                    tilda_window_add_tab (tw);
                    pull (tw, PULL_UP);
                    break;
                case EXIT_TILDA:
                default:
                    /**
                     * It is necessary to check the main_level because if CTRL_C was used
                     * or the "Quit" option from the context menu then gtk_main_quit has
                     * already been called and cannot call it again.
                     */
                    if(gtk_main_level () > 0) {
                        gtk_main_quit ();
                    }
                    break;
            }
        }
    }

    /* Remove the tilda_term from the list of terminals */
    tw->terms = g_list_remove (tw->terms, tt);

    /* Free the terminal, we are done with it */
    tilda_term_free (tt);

    return 0;
}
