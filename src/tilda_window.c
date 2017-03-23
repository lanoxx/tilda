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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <tilda-config.h>

#include "debug.h"
#include "tilda.h"
#include "callback_func.h"
#include "configsys.h"
#include "tilda_window.h"
#include "tilda_terminal.h"
#include "key_grabber.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <vte/vte.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

static tilda_term* tilda_window_get_current_terminal (tilda_window *tw);

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
    const GtkPositionType gtk_pos[] = {GTK_POS_TOP, GTK_POS_BOTTOM, GTK_POS_LEFT, GTK_POS_RIGHT };

    if ((pos < 0) || (pos > 4)) {
        g_printerr (_("You have a bad tab_pos in your configuration file\n"));
        pos = NB_TOP;
    }

    if(NB_HIDDEN == pos) {
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK(tw->notebook), FALSE);
    }
    else {
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), gtk_pos[pos]);
        if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) > 1)
            gtk_notebook_set_show_tabs (GTK_NOTEBOOK(tw->notebook), TRUE);
    }

    return 0;
}


void tilda_window_set_fullscreen(tilda_window *tw)
{
  DEBUG_FUNCTION ("tilda_window_set_fullscreen");
  DEBUG_ASSERT (tw != NULL);

  if (tw->fullscreen == TRUE) {
    gtk_window_fullscreen (GTK_WINDOW (tw->window));
  }
  else {
    gtk_window_unfullscreen (GTK_WINDOW (tw->window));
    // This appears to be necssary on (at least) xfwm4 if you tabbed out
    // while fullscreened.
    gtk_window_set_default_size (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    gtk_window_move(GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
  }
}

gint toggle_fullscreen_cb (tilda_window *tw)
{
    DEBUG_FUNCTION ("toggle_fullscreen_cb");
    DEBUG_ASSERT (tw != NULL);

    tw->fullscreen = !tw->fullscreen;

    tilda_window_set_fullscreen(tw);

    // It worked. Having this return GDK_EVENT_STOP makes the callback not carry the
    // keystroke into the vte terminal widget.
    return GDK_EVENT_STOP;
}

gint toggle_transparency_cb (tilda_window *tw)
{
    DEBUG_FUNCTION ("toggle_transparency");
    DEBUG_ASSERT (tw != NULL);
    tilda_window_toggle_transparency(tw);
    return GDK_EVENT_STOP;
}

void tilda_window_toggle_transparency (tilda_window *tw)
{
    gboolean status = !config_getbool ("enable_transparency");
    config_setbool ("enable_transparency", status);
    tilda_window_apply_transparency (tw, status);
}

void tilda_window_refresh_transparency (tilda_window *tw)
{
    gboolean status = config_getbool ("enable_transparency");
    tilda_window_apply_transparency (tw, status);
}

void tilda_window_apply_transparency (tilda_window *tw, gboolean status)
{
    tilda_term *tt;
    guint i;

#ifdef VTE_290
    gdouble transparency_level = 0.0;
    transparency_level = ((gdouble) config_getint ("transparency"))/100;

    if (status)
    {
        for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_background_saturation (VTE_TERMINAL(tt->vte_term), transparency_level);
            vte_terminal_set_background_transparent(VTE_TERMINAL(tt->vte_term), !tw->have_argb_visual);
            vte_terminal_set_opacity (VTE_TERMINAL(tt->vte_term), (1.0 - transparency_level) * 0xffff);
        }
    }
    else
    {
        for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_background_saturation (VTE_TERMINAL(tt->vte_term), 0);
            vte_terminal_set_background_transparent(VTE_TERMINAL(tt->vte_term), FALSE);
            vte_terminal_set_opacity (VTE_TERMINAL(tt->vte_term), 0xffff);
        }
    }
#else
    GdkRGBA bg;
    bg.red   =    GUINT16_TO_FLOAT(config_getint ("back_red"));
    bg.green =    GUINT16_TO_FLOAT(config_getint ("back_green"));
    bg.blue  =    GUINT16_TO_FLOAT(config_getint ("back_blue"));
    bg.alpha =    (status ? GUINT16_TO_FLOAT(config_getint ("back_alpha")) : 1.0);

    for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_color_background(VTE_TERMINAL(tt->vte_term), &bg);
        }
#endif
}

gint tilda_window_toggle_searchbar (tilda_window *tw)
{
    DEBUG_FUNCTION ("toggle_searbar");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tw->search != NULL);
    gboolean visible = !gtk_widget_get_visible(tw->search->search_box);
    if (visible) {
        gtk_widget_grab_focus (tw->search->entry_search);
    } else {
        gtk_widget_grab_focus (tilda_window_get_current_terminal (tw)->vte_term);
    }
    gtk_widget_set_visible(tw->search->search_box, visible);
    return GDK_EVENT_STOP;
}

/* Zoom helpers */
static const double zoom_factors[] = {
        TERMINAL_SCALE_MINIMUM,
        TERMINAL_SCALE_XXXXX_SMALL,
        TERMINAL_SCALE_XXXX_SMALL,
        TERMINAL_SCALE_XXX_SMALL,
        PANGO_SCALE_XX_SMALL,
        PANGO_SCALE_X_SMALL,
        PANGO_SCALE_SMALL,
        PANGO_SCALE_MEDIUM,
        PANGO_SCALE_LARGE,
        PANGO_SCALE_X_LARGE,
        PANGO_SCALE_XX_LARGE,
        TERMINAL_SCALE_XXX_LARGE,
        TERMINAL_SCALE_XXXX_LARGE,
        TERMINAL_SCALE_XXXXX_LARGE,
        TERMINAL_SCALE_MAXIMUM
};

static gboolean find_larger_zoom_factor (double  current, double *found) {
    guint i;

    for (i = 0; i < G_N_ELEMENTS (zoom_factors); ++i)
    {
        /* Find a font that's larger than this one */
        if ((zoom_factors[i] - current) > 1e-6)
        {
            *found = zoom_factors[i];
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean find_smaller_zoom_factor (double  current, double *found) {
    int i;

    i = (int) G_N_ELEMENTS (zoom_factors) - 1;
    while (i >= 0)
    {
        /* Find a font that's smaller than this one */
        if ((current - zoom_factors[i]) > 1e-6)
        {
            *found = zoom_factors[i];
            return TRUE;
        }

        --i;
    }

    return FALSE;
}

/* Increase and Decrease and reset affects all tabs at once */
static gboolean normalize_font_size(tilda_window *tw)
{
    tilda_term *tt;
    guint i;
    tw->current_scale_factor = PANGO_SCALE_MEDIUM;

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        tilda_term_adjust_font_scale(tt, tw->current_scale_factor);
    }
    return GDK_EVENT_STOP;
}

static gboolean increase_font_size (tilda_window *tw)
{
    tilda_term *tt;
    guint i;
    if(!find_larger_zoom_factor (tw->current_scale_factor, &tw->current_scale_factor)) {
        return GDK_EVENT_STOP;
    }

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        tilda_term_adjust_font_scale(tt, tw->current_scale_factor);
    }
    return GDK_EVENT_STOP;
}

static gboolean decrease_font_size (tilda_window *tw)
{
    tilda_term *tt;
    guint i;
    if(!find_smaller_zoom_factor (tw->current_scale_factor, &tw->current_scale_factor)) {
        return GDK_EVENT_STOP;
    }

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        tilda_term_adjust_font_scale(tt, tw->current_scale_factor);
    }
    return GDK_EVENT_STOP;
}

gint tilda_window_next_tab (tilda_window *tw)
{
    DEBUG_FUNCTION ("next_tab");
    DEBUG_ASSERT (tw != NULL);

    int num_pages;
    int current_page;
    GtkNotebook *notebook;

    notebook = GTK_NOTEBOOK (tw->notebook);

    /* If we are on the last page, go to first page */
    num_pages = gtk_notebook_get_n_pages (notebook);
    current_page = gtk_notebook_get_current_page (notebook);

    if ((num_pages - 1) == current_page)
      gtk_notebook_set_current_page (notebook, 0);
    else
      gtk_notebook_next_page (notebook);

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
    GtkNotebook *notebook;

    notebook = GTK_NOTEBOOK (tw->notebook);

    num_pages = gtk_notebook_get_n_pages (notebook);
    current_page = gtk_notebook_get_current_page (notebook);

    if (current_page == 0)
      gtk_notebook_set_current_page (notebook, (num_pages - 1));
    else
      gtk_notebook_prev_page (notebook);

    // It worked. Having this return GDK_EVENT_STOP makes the callback not carry the
    // keystroke into the vte terminal widget.
    return GDK_EVENT_STOP;
}

enum tab_direction { TAB_LEFT = -1, TAB_RIGHT = 1 };

static gint move_tab (tilda_window *tw, int direction)
{
    DEBUG_FUNCTION ("move_tab");
    DEBUG_ASSERT (tw != NULL);

    int num_pages;
    int current_page_index;
    int new_page_index;
    GtkWidget* current_page;
    GtkNotebook* notebook;

    notebook = GTK_NOTEBOOK (tw->notebook);

    num_pages = gtk_notebook_get_n_pages (notebook);

    if (num_pages > 1) {
        current_page_index = gtk_notebook_get_current_page (notebook);
        current_page = gtk_notebook_get_nth_page (notebook,
                                                  current_page_index);

        /* wrap over if new_page_index over-/underflows */
        new_page_index = (current_page_index + direction) % num_pages;

        gtk_notebook_reorder_child (notebook, current_page, new_page_index);
    }

    // It worked. Having this return GDK_EVENT_STOP makes the callback not carry the
    // keystroke into the vte terminal widget.
    return GDK_EVENT_STOP;

}
static gint move_tab_left (tilda_window *tw)
{
    DEBUG_FUNCTION ("move_tab_left");
    return move_tab(tw, TAB_LEFT);
}

static gint move_tab_right (tilda_window *tw)
{
    DEBUG_FUNCTION ("move_tab_right");
    return move_tab(tw, TAB_RIGHT);
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
    if ((tw->auto_hide_current_time >= tw->auto_hide_max_time) || tw->current_state == STATE_UP)
    {
        pull(tw, PULL_UP, TRUE);
        tw->auto_hide_tick_handler = 0;
        return FALSE;
    }

    return TRUE;
}

/* Start auto hide tick */
static void start_auto_hide_tick(tilda_window *tw)
{
    DEBUG_FUNCTION("start_auto_hide_tick");
    // If the delay is less or equal to 1000ms then the timer is not precise
    // enough, because it takes already about 200ms to register it, so we
    // rather sleep for the given amount of time.
    const guint32 MAX_SLEEP_TIME = 1000;

    if ((tw->auto_hide_tick_handler == 0) && (tw->disable_auto_hide == FALSE)) {
        /* If there is no timer registered yet, then the auto_hide_tick_handler
         * has a value of zero. Next we need to make sure that the auto hide
         * max time is greater then zero, or else we can pull up immediately,
         * without the trouble of registering a timer.
         */
        if (tw->auto_hide_max_time > MAX_SLEEP_TIME) {
            tw->auto_hide_current_time = 0;
            tw->auto_hide_tick_handler = g_timeout_add(tw->timer_resolution, auto_hide_tick, tw);
        } else if (tw->auto_hide_max_time > 0 && tw->auto_hide_max_time <= MAX_SLEEP_TIME) {
            // auto_hide_max_time is in milli seconds, so we need to convert to
            // microseconds by multiplying with 1000
            g_usleep (tw->auto_hide_max_time * 1000);
            pull(tw, PULL_UP, TRUE);
        } else {
            pull(tw, PULL_UP, TRUE);
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

    /**
    * When the tilda 'key' to pull down/up the tilda window is pressed, then tilda will inevitably loose focus. The
    * problem is that we cannot distinguish whether it was focused before the key press occurred. Checking the xevent
    * type here allows us to distinguish these two cases:
    *
    *  * We loose focus because of a KeyPress event
    *  * We loose focus because another window gained focus or some other reason.
    *
    *  Depending on one of the two cases we set the tw->focus_loss_on_keypress to true or false. We can then
    *  check this flag in the pull() function that shows or hides the tilda window. This helps us to decide if
    *  we just need to focus tilda or if we should hide it.
    */
    XEvent xevent;
    XPeekEvent(gdk_x11_display_get_xdisplay(gdk_window_get_display(gtk_widget_get_window(tw->window))), &xevent);

    if(xevent.type == KeyPress) {
        tw->focus_loss_on_keypress = TRUE;
    } else {
        tw->focus_loss_on_keypress = FALSE;
    }

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

    return GDK_EVENT_STOP;
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
    if GTK_IS_SCROLLBAR(list->data) {
        list = list->next;
    }
    vte_terminal_copy_clipboard (VTE_TERMINAL(list->data));

    return GDK_EVENT_STOP;
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
    if GTK_IS_SCROLLBAR(list->data) {
        list = list->next;
    }
    vte_terminal_paste_clipboard (VTE_TERMINAL(list->data));

    return GDK_EVENT_STOP;
}

/* Tie a single keyboard shortcut to a callback function */
static gint tilda_add_config_accelerator_by_path(const gchar* key, const gchar* path, GCallback callback_func, tilda_window *tw)
{
    guint accel_key;
    GdkModifierType accel_mods;
    GClosure *temp;

    gtk_accelerator_parse (config_getstr(key), &accel_key, &accel_mods);
    if (! ((accel_key == 0) && (accel_mods == 0)) )  // make sure it parsed properly
    {
        temp = g_cclosure_new_swap (callback_func, tw, NULL);
        gtk_accel_map_add_entry(path, accel_key, accel_mods);
        gtk_accel_group_connect_by_path(tw->accel_group, path, temp);
    }

    return 0;
}

gboolean tilda_window_update_keyboard_accelerators (const gchar* path, const gchar* value) {
    guint accel_key;
    GdkModifierType accel_mods;
    gtk_accelerator_parse (value, &accel_key, &accel_mods);

    return gtk_accel_map_change_entry(path, accel_key, accel_mods, FALSE);
}

/* This function does the setup of the keyboard acceleratos. It should only be called once when the tilda window is
 * initialized. Use tilda_window_update_keyboard_accelerators to update keybindings that have been changed by the user.
 */
static gint tilda_window_setup_keyboard_accelerators (tilda_window *tw)
{

    /* Create Accel Group to add key codes for quit, next, prev and new tabs */
    tw->accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (tw->window), tw->accel_group);

    /* Set up keyboard shortcuts for Exit, Next Tab, Previous Tab,
       Move Tab, Add Tab, Close Tab, Copy, and Paste using key
       combinations defined in the config. */
    tilda_add_config_accelerator_by_path("addtab_key",     "<tilda>/context/New Tab",           G_CALLBACK(tilda_window_add_tab),           tw);
    tilda_add_config_accelerator_by_path("closetab_key",   "<tilda>/context/Close Tab",         G_CALLBACK(tilda_window_close_current_tab), tw);
    tilda_add_config_accelerator_by_path("copy_key",       "<tilda>/context/Copy",              G_CALLBACK(ccopy),                          tw);
    tilda_add_config_accelerator_by_path("paste_key",      "<tilda>/context/Paste",             G_CALLBACK(cpaste),                         tw);
    tilda_add_config_accelerator_by_path("fullscreen_key", "<tilda>/context/Toggle Fullscreen", G_CALLBACK(toggle_fullscreen_cb),           tw);
    tilda_add_config_accelerator_by_path("quit_key",       "<tilda>/context/Quit",              G_CALLBACK(gtk_main_quit),                  tw);
    tilda_add_config_accelerator_by_path("toggle_transparency_key", "<tilda>/context/Toggle Transparency", G_CALLBACK(toggle_transparency_cb),      tw);
    tilda_add_config_accelerator_by_path("toggle_searchbar_key", "<tilda>/context/Toggle Searchbar", G_CALLBACK(tilda_window_toggle_searchbar),     tw);

    tilda_add_config_accelerator_by_path("nexttab_key",      "<tilda>/context/Next Tab",        G_CALLBACK(tilda_window_next_tab),          tw);
    tilda_add_config_accelerator_by_path("prevtab_key",      "<tilda>/context/Previous Tab",    G_CALLBACK(tilda_window_prev_tab),          tw);
    tilda_add_config_accelerator_by_path("movetableft_key",  "<tilda>/context/Move Tab Left",   G_CALLBACK(move_tab_left),                  tw);
    tilda_add_config_accelerator_by_path("movetabright_key", "<tilda>/context/Move Tab Right",  G_CALLBACK(move_tab_right),                 tw);

    tilda_add_config_accelerator_by_path("increase_font_size_key",  "<tilda>/context/Increase Font Size",  G_CALLBACK(increase_font_size), tw);
    tilda_add_config_accelerator_by_path("decrease_font_size_key",  "<tilda>/context/Decrease Font Size",  G_CALLBACK(decrease_font_size), tw);
    tilda_add_config_accelerator_by_path("normalize_font_size_key", "<tilda>/context/Normalize Font Size", G_CALLBACK(normalize_font_size), tw);

    /* Set up keyboard shortcuts for Goto Tab # using key combinations defined in the config*/
    /* Know a better way? Then you do. */
    tilda_add_config_accelerator_by_path("gototab_1_key",  "<tilda>/context/Goto Tab 1",  G_CALLBACK(goto_tab_1),  tw);
    tilda_add_config_accelerator_by_path("gototab_2_key",  "<tilda>/context/Goto Tab 2",  G_CALLBACK(goto_tab_2),  tw);
    tilda_add_config_accelerator_by_path("gototab_3_key",  "<tilda>/context/Goto Tab 3",  G_CALLBACK(goto_tab_3),  tw);
    tilda_add_config_accelerator_by_path("gototab_4_key",  "<tilda>/context/Goto Tab 4",  G_CALLBACK(goto_tab_4),  tw);
    tilda_add_config_accelerator_by_path("gototab_5_key",  "<tilda>/context/Goto Tab 5",  G_CALLBACK(goto_tab_5),  tw);
    tilda_add_config_accelerator_by_path("gototab_6_key",  "<tilda>/context/Goto Tab 6",  G_CALLBACK(goto_tab_6),  tw);
    tilda_add_config_accelerator_by_path("gototab_7_key",  "<tilda>/context/Goto Tab 7",  G_CALLBACK(goto_tab_7),  tw);
    tilda_add_config_accelerator_by_path("gototab_8_key",  "<tilda>/context/Goto Tab 8",  G_CALLBACK(goto_tab_8),  tw);
    tilda_add_config_accelerator_by_path("gototab_9_key",  "<tilda>/context/Goto Tab 9",  G_CALLBACK(goto_tab_9),  tw);
    tilda_add_config_accelerator_by_path("gototab_10_key", "<tilda>/context/Goto Tab 10", G_CALLBACK(goto_tab_10), tw);

    return 0;
}

static tilda_term* tilda_window_get_current_terminal (tilda_window *tw) {
    gint pos = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));
    if (pos >= 0) {
        GList *found = g_list_nth (tw->terms, (guint) pos);
        if (found) {
            return found->data;
        }
    }
    return NULL;
}

static void tilda_window_search (G_GNUC_UNUSED GtkWidget *widget, tilda_window *tw, gboolean terminal_search_backwards) {
    GRegexCompileFlags compile_flags = G_REGEX_OPTIMIZE;
    gboolean wrap_on_search = FALSE;
    tilda_search *search = tw->search;
    gboolean is_regex = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (search->check_regex));
    const gchar *text = gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (search->entry_search))));

    if (!search->is_search_result) {
        wrap_on_search = TRUE;
    }

    gchar *pattern;
    if (is_regex) {
        compile_flags |= G_REGEX_MULTILINE;
        pattern = (gchar *) text;
    }
    else {
        pattern = g_regex_escape_string (text, -1);
    }
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (search->check_match_case))) {
        compile_flags |= G_REGEX_CASELESS;
    }

    tilda_term* term = tilda_window_get_current_terminal (tw);
    GtkWidget *vteTerminal = term->vte_term;

    GError *error = NULL;
    GRegex *regex = g_regex_new (pattern, compile_flags, G_REGEX_MATCH_NEWLINE_ANY, &error);
#ifdef VTE_290
    vte_terminal_search_set_gregex (VTE_TERMINAL (vteTerminal), regex);
#else
    vte_terminal_search_set_gregex (VTE_TERMINAL (vteTerminal), regex, 0);
#endif
    vte_terminal_search_set_wrap_around (VTE_TERMINAL (vteTerminal), wrap_on_search);

    gboolean search_result;
    if (terminal_search_backwards) {
        search_result = vte_terminal_search_find_previous (VTE_TERMINAL (vteTerminal));
    }
    else {
        search_result = vte_terminal_search_find_next (VTE_TERMINAL (vteTerminal));
    }

    gtk_widget_set_visible (search->label_search_end, !search_result);
    search->is_search_result = search_result;

    /* If the text was not a regex, then we escaped the text with g_regex_escape_string and so we need to free it. */
    if (!is_regex) {
        g_free (pattern);
    }
    g_regex_unref(regex);
}

static void tilda_window_search_forward_cb (GtkWidget *button, tilda_window *tw) {
    /* The default is to search forward */
    tilda_window_search (button, tw, FALSE);
}

static void tilda_window_search_backward_cb (GtkWidget *button, tilda_window *tw) {
    tilda_window_search (button, tw, TRUE);
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

gboolean search_box_key_cb (GtkWidget *widget, GdkEvent  *event, tilda_window *tw) {
    GdkEventKey *event_key = (GdkEventKey*)event;
    if (event_key->keyval == GDK_KEY_Return) {
        tilda_window_search(widget, tw, FALSE);
        return GDK_EVENT_STOP;
    }

    /* If the search entry has focus the user can hide the search bar by pressing escape. */
    if (event_key->keyval == GDK_KEY_Escape) {
        if (gtk_widget_has_focus (tw->search->entry_search)) {
            gtk_widget_grab_focus (tilda_window_get_current_terminal (tw)->vte_term);
            gtk_widget_set_visible (tw->search->search_box, FALSE);
            return GDK_EVENT_STOP;
        }
    }

    return GDK_EVENT_PROPAGATE;
}

gboolean entry_search_text_changed_callback (G_GNUC_UNUSED GtkEditable *editable,
                                             tilda_window *tw) {
    gtk_widget_hide (tw->search->label_search_end);
    tw->search->is_search_result = TRUE;

    return GDK_EVENT_STOP;
}

static tilda_search *tilda_search_box_init(tilda_window *tw)
{
    DEBUG_FUNCTION ("tilda_search_box_init");
    GtkBuilder *gtk_builder = tw->gtk_builder;

    tilda_search *search = malloc(sizeof(tilda_search));

    search->search_box = GTK_WIDGET(gtk_builder_get_object (gtk_builder, "search_box"));
    gtk_widget_set_name (GTK_WIDGET (search->search_box), "search");

    DEBUG_ASSERT(search->search_box != NULL);
    search->button_next = GTK_WIDGET (gtk_builder_get_object (gtk_builder, "button_search_next"));
    search->button_prev = GTK_WIDGET (gtk_builder_get_object (gtk_builder, "button_search_prev"));
    search->entry_search = GTK_WIDGET (gtk_builder_get_object (gtk_builder, "entry_search"));
    search->check_match_case = GTK_WIDGET (gtk_builder_get_object (gtk_builder, "check_match_case"));
    search->check_regex = GTK_WIDGET (gtk_builder_get_object (gtk_builder, "check_regex"));
    search->label_search_end = GTK_WIDGET (gtk_builder_get_object (gtk_builder, "search_label"));
    /* Initialize to true to prevent search from wrapping around on first search. */
    search->is_search_result = TRUE;

    g_signal_connect (G_OBJECT (search->button_next), "clicked", G_CALLBACK (tilda_window_search_forward_cb), tw);
    g_signal_connect (G_OBJECT (search->button_prev), "clicked", G_CALLBACK (tilda_window_search_backward_cb), tw);
    g_signal_connect (G_OBJECT(search->entry_search), "key-press-event", G_CALLBACK(search_box_key_cb), tw);
    g_signal_connect (G_OBJECT (search->entry_search), "changed", G_CALLBACK (entry_search_text_changed_callback), tw);
    return search;
}

/* Detect changes in GtkNotebook tab order and update the tw->terms list to reflect such changes. */
static void page_reordered_cb (GtkNotebook  *notebook,
                        GtkWidget    *child,
                        guint         page_num,
                        tilda_window *tw) {
    DEBUG_FUNCTION ("page_reordered_cb");
    GList *terminals;
    tilda_term *tilda_term;
    guint i;

    terminals = tw->terms;

    for (i = 0; i < g_list_length (terminals); i++)
    {
        tilda_term = g_list_nth_data (terminals, i);
        if (tilda_term->hbox == child) {
            terminals = g_list_remove (terminals, tilda_term);
            tw->terms = g_list_insert (terminals, tilda_term, page_num);
            break;
        }
    }
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

    GError* error = NULL;
    GtkBuilder *gtk_builder = gtk_builder_new ();

#if ENABLE_NLS
    gtk_builder_set_translation_domain (gtk_builder, PACKAGE);
#endif

    if(!gtk_builder_add_from_resource (gtk_builder, "/org/tilda/tilda.ui", &error)) {
        fprintf (stderr, "Error: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }
    tw->gtk_builder = gtk_builder;

    /* The gdk_x11_get_server_time call will hang if GDK_PROPERTY_CHANGE_MASK is not set */
    gdk_window_set_events(gdk_screen_get_root_window (gtk_widget_get_screen (tw->window)), GDK_PROPERTY_CHANGE_MASK);

    /* Generic timer resolution */
    tw->timer_resolution = config_getint("timer_resolution");

    /* Auto hide support */
    tw->auto_hide_tick_handler = 0;
    tw->auto_hide_max_time = config_getint("auto_hide_time");
    tw->auto_hide_on_mouse_leave = config_getbool("auto_hide_on_mouse_leave");
    tw->auto_hide_on_focus_lost = config_getbool("auto_hide_on_focus_lost");
    tw->disable_auto_hide = FALSE;
    tw->focus_loss_on_keypress = FALSE;

    PangoFontDescription *description = pango_font_description_from_string(config_getstr("font"));
    gint size = pango_font_description_get_size(description);
    tw->unscaled_font_size = size;
    tw->current_scale_factor = PANGO_SCALE_MEDIUM;

    if(1 == config_getint("non_focus_pull_up_behaviour")) {
        tw->hide_non_focused = TRUE;
    }
    else {
        tw->hide_non_focused = FALSE;
    }

    tw->fullscreen = config_getbool("start_fullscreen");
    tilda_window_set_fullscreen(tw);

    /* Set up all window properties */
    if (config_getbool ("pinned"))
        gtk_window_stick (GTK_WINDOW(tw->window));

    if(config_getbool ("set_as_desktop"))
        gtk_window_set_type_hint(GTK_WINDOW(tw->window), GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tw->window), config_getbool ("notaskbar"));
    gtk_window_set_keep_above (GTK_WINDOW(tw->window), config_getbool ("above"));
    gtk_window_set_decorated (GTK_WINDOW(tw->window), FALSE);
    gtk_widget_set_size_request (GTK_WIDGET(tw->window), 0, 0);
    tilda_window_set_icon (tw, g_build_filename (DATADIR, "pixmaps", "tilda.png", NULL));
    tilda_window_setup_alpha_mode (tw);

    gtk_widget_set_app_paintable (GTK_WIDGET (tw->window), TRUE);

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

    /* We need this signal to detect changes in the order of tabs so that we can keep the order
     * of tilda_terms in the tw->terms structure in sync with the order of tabs. */
    g_signal_connect (G_OBJECT(tw->notebook), "page-reordered", G_CALLBACK (page_reordered_cb), tw);

    /* Setup the tilda window. The tilda window consists of a toplevel window that contains the following widgets:
     *   * The main_box holds a GtkNotebook with all the terminal tabs
     *   * The search_box holds the search widgets and a label to indicate when the search has reached the bottom
     */
    GtkWidget *main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    tw->search = tilda_search_box_init(tw);

    gtk_container_add (GTK_CONTAINER(tw->window), main_box);
    gtk_box_pack_start (GTK_BOX (main_box), tw->notebook, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (main_box), tw->search->search_box, FALSE, TRUE, 0);


    /* Show the widgets */
    gtk_widget_show_all (main_box);
    gtk_widget_set_visible(tw->search->search_box, FALSE);
    /* the tw->window widget will be shown later, by pull() */

    /* Position the window */
    tw->current_state = STATE_UP;
    gtk_window_set_default_size (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    /* Create GDK resources now, to prevent crashes later on */
    gtk_widget_realize (tw->window);
    generate_animation_positions (tw);

    /* Initialize wizard window reference to NULL */
    tw->wizard_window = NULL;

    return TRUE;
}

gint tilda_window_free (tilda_window *tw)
{

    /* Close each tab which still exists.
     * This will free their data structures automatically. */
    if (tw->notebook != NULL) {
        gint num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK(tw->notebook));
        while (num_pages > 0)
        {
            /* Close the 0th tab, which should always exist while we have
             * some pages left in the notebook. */
            tilda_window_close_tab (tw, 0, TRUE);

            num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK(tw->notebook));
        }
    }

    g_free (tw->config_file);
    g_free (tw->search);
    if (tw->gtk_builder != NULL) {
        g_object_unref (G_OBJECT(tw->gtk_builder));
    }

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
    label = gtk_label_new (config_getstr("title"));
    /* Strangely enough, prepend puts pages on the end */
    index = gtk_notebook_append_page (GTK_NOTEBOOK(tw->notebook), tt->hbox, label);
    gtk_notebook_set_current_page (GTK_NOTEBOOK(tw->notebook), index);
    gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK(tw->notebook), tt->hbox, TRUE);

    /* We should show the tabs if there are more than one tab in the notebook,
     * and tab position is not set to hidden */
    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) > 1 &&
            config_getint("tab_pos") != NB_HIDDEN)
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), TRUE);

    /* The new terminal should grab the focus automatically */
    gtk_widget_grab_focus (tt->vte_term);

    return GDK_EVENT_STOP; //index;
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
                    pull (tw, PULL_UP, TRUE);
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

    return GDK_EVENT_STOP;
}
