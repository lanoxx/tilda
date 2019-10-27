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

#ifndef TILDA_WINDOW_H
#define TILDA_WINDOW_H

#include "tilda_terminal.h"

#include <glib.h>
#include <gtk/gtk.h>

#include "tilda-search-box.h"

G_BEGIN_DECLS

enum pull_action {
    PULL_UP,
    PULL_DOWN,
    PULL_TOGGLE
};

typedef struct tilda_window_ tilda_window;

enum tilda_animation_state {
    STATE_UP,
    STATE_DOWN,
    STATE_GOING_UP,
    STATE_GOING_DOWN
};

struct tilda_window_
{
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *search;

    GList *terms;
    GtkAccelGroup * accel_group;
    GtkBuilder *gtk_builder;
    GtkWidget *wizard_window; /* GtkDialog that contains the wizard */

    gchar *lock_file;
    gchar *config_file;
    gboolean config_writing_disabled;
    gint instance;
    gboolean have_argb_visual;

    /* Temporarily disable auto hiding */
    gboolean disable_auto_hide;
    /* Auto hide tick-function handler */
    guint auto_hide_tick_handler;
    /* Auto hide current time */
    guint32 auto_hide_current_time;
    /* Auto hide max time */
    guint32 auto_hide_max_time;
    /* Generic timer resolution */
    guint32 timer_resolution;
    /* Should Tilda hide itself on focus lost event? */
    gboolean auto_hide_on_focus_lost;
    /* Should Tilda hide itself when mouse leaves it? */
    gboolean auto_hide_on_mouse_leave;

    /* Should Tilda hide itself even if not focused */
    gboolean hide_non_focused;

	gboolean fullscreen;

    /* This field MUST be set before calling pull()! */
    enum tilda_animation_state current_state;

    gboolean focus_loss_on_keypress;

    gint unscaled_font_size;
    gdouble current_scale_factor;

    enum pull_action last_action;
    gint64 last_action_time;

    /**
     * This stores the ID of the event source which handles size updates.
     */
    guint size_update_event_source;
};

/* For use in get_display_dimension() */
enum dimensions { HEIGHT, WIDTH };

enum notebook_tab_positions { NB_TOP, NB_BOTTOM, NB_LEFT, NB_RIGHT, NB_HIDDEN };

/**
 * tilda_window_add_tab ()
 *
 * Create and add a new tab at the end of the notebook
 *
 * Success: the new tab's index (>=0)
 * Failure: -1
 */
gint tilda_window_add_tab (tilda_window *tw);

/**
 * tilda_window_close_tab ()
 *
 * Closes the tab at the given tab index (starting from 0)
 *
 * Success: return 0
 * Failure: return non-zero
 */
gint tilda_window_close_tab (tilda_window *tw, gint tab_position, gboolean force_exit);

/**
 * tilda_window_next_tab ()
 *
 * Switch to next tab
 *
 * Success: return 0
 * Failure: return non-zero
 */
gint tilda_window_next_tab (tilda_window *tw);

/**
 * tilda_window_prev_tab ()
 *
 * Switch to previous tab
 *
 * Success: return 0
 * Failure: return non-zero
 */
gint tilda_window_prev_tab (tilda_window *tw);

/**
 * tilda_window_init ()
 *
 * Initalizes an already allocated tilda_window *. It will also initialize and set up
 * as much of the window as possible using the values in the configuation system.
 *
 * @param instance the instance number of this tilda_window
 *
 * Notes: The configuration system must be up and running before calling this function.
 */
gboolean tilda_window_init (const gchar *config_file, const gint instance, tilda_window *tw);

/**
 * Releases resources that are being used by the tilda window, such as the tabs
 * or the config file.
 */
gint tilda_window_free (tilda_window *tw);

/**
 * Applies or reapplies the current fullscreen state of the tilda window.
 */
void tilda_window_set_fullscreen(tilda_window *tw);

/**
 * This toggles the fullscreen mode on or off. This is intended to be registered
 * as a GCallback in order to be invoked after some user action.
 */
gint toggle_fullscreen_cb (tilda_window *tw);

/**
 * This controls where the tabs are positions (e.g. top, left, bottom or
 * right).
 */
gint tilda_window_set_tab_position (tilda_window *tw, enum notebook_tab_positions pos);

/**
 * tilda_window_close_tab ()
 *
 * Closes the tab current tab
 */
void tilda_window_close_current_tab (tilda_window *tw);

/* This should be called by the wizard for each key that has changed. */
gboolean tilda_window_update_keyboard_accelerators (const gchar* path, const gchar* value);

/**
 * Toggles transparency on all terms
 */
void tilda_window_toggle_transparency(tilda_window *tw);

/**
 * Refreshes transparency
 */
void tilda_window_refresh_transparency(tilda_window *tw);

/**
 * Toggles the search bar of the tilda window.
 */
void tilda_window_toggle_searchbar (tilda_window *tw);

/**
 * Show confirm dialog before quitting (if enabled)
 */
gint tilda_window_confirm_quit (tilda_window *tw);

GdkMonitor* tilda_window_find_monitor_number(tilda_window *tw);

/**
 * Finds the coordinate that will center the tilda window in the screen.
 *
 * If you want to center the tilda window on the top or bottom of the screen,
 * pass the screen width into screen_dimension and the tilda window's width
 * into the tilda_dimension variable. The result will be the x coordinate that
 * should be used in order to have the tilda window centered on the screen.
 *
 * Centering based on y coordinate is similar, just use the screen height and
 * tilda window height.
 */
gint tilda_window_find_centering_coordinate (tilda_window *tw, enum dimensions dimension);

void tilda_window_update_window_position (tilda_window *tw);

#define TILDA_WINDOW(data) ((tilda_window *)(data))

/* Allow scales a bit smaller and a bit larger than the usual pango ranges */
#define TERMINAL_SCALE_XXX_SMALL   (PANGO_SCALE_XX_SMALL/1.2)
#define TERMINAL_SCALE_XXXX_SMALL  (TERMINAL_SCALE_XXX_SMALL/1.2)
#define TERMINAL_SCALE_XXXXX_SMALL (TERMINAL_SCALE_XXXX_SMALL/1.2)
#define TERMINAL_SCALE_XXX_LARGE   (PANGO_SCALE_XX_LARGE*1.2)
#define TERMINAL_SCALE_XXXX_LARGE  (TERMINAL_SCALE_XXX_LARGE*1.2)
#define TERMINAL_SCALE_XXXXX_LARGE (TERMINAL_SCALE_XXXX_LARGE*1.2)
#define TERMINAL_SCALE_MINIMUM     (TERMINAL_SCALE_XXXXX_SMALL/1.2)
#define TERMINAL_SCALE_MAXIMUM     (TERMINAL_SCALE_XXXXX_LARGE*1.2)

G_END_DECLS

#endif
