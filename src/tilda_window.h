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

#ifndef TILDA_WINDOW_H
#define TILDA_WINDOW_H

#include <tilda_window.h>
#include <tilda_terminal.h>

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct tilda_window_ tilda_window;

struct tilda_window_
{
    GtkWidget *window;
    GtkWidget *notebook;
    GList *terms;
    GtkAccelGroup * accel_group;

    gchar *lock_file;
    gchar *config_file;
    gboolean config_writing_disabled;
    gint instance;
    gboolean have_argb_visual;
    
    /* Temporarily disable auto hiding */
    gboolean disable_auto_hide;
    /* Auto hide tick-function handler */
    gint auto_hide_tick_handler;
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

	gboolean fullscreen;

    /* This field MUST be set before calling pull()! */
    enum tilda_positions { UP, DOWN } current_state;
};

enum notebook_tab_positions { NB_TOP, NB_BOTTOM, NB_LEFT, NB_RIGHT };

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
tilda_window *tilda_window_init (const gchar *config_file, const gint instance);

gint tilda_window_free (tilda_window *tw);

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

/**
 * This registers the keyboard shortcuts to the values which the user has
 * configured. It is automatically called when the window is initialized,
 * but it must be called by the wizard after the user has changed one
 * of the shortcuts, in order to registered the new shortcut.
 */
gint tilda_window_setup_keyboard_accelerators (tilda_window *tw);

#define TILDA_WINDOW(data) ((tilda_window *)(data))

G_END_DECLS

#endif
