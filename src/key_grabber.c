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
 *
 */

/* Some stolen from yeahconsole -- loving that open source :) */

#include "config.h"

#include "debug.h"
#include "key_grabber.h"
#include "screen-size.h"
#include "tilda.h"
#include <glib.h>
#include <glib/gi18n.h>
#include "configsys.h"
#include "tomboykeybinder.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkx.h>

#define ANIMATION_UP 0
#define ANIMATION_DOWN 1

/* Define local variables here */
#define ANIMATION_Y 0
#define ANIMATION_X 1

/*
 *  animation_coordinates:
 *  [0: up, 1: down]
 *  [0: ypos, 1: xpos]
 *  [animation steps]
 */
static gint animation_coordinates[2][2][32];

static float animation_ease_function_down(gint i, gint n) {
    const float t = (float)i/n;
    const float ts = t*t;
    const float tc = ts*t;
    return 1*tc*ts + -5*ts*ts + 10*tc + -10*ts + 5*t;
}

static float animation_ease_function_up(gint i, gint n) {
    const float t = (float)i/n;
    const float ts = t*t;
    const float tc = ts*t;
    return 1 - (0*tc*ts + 0*ts*ts + 0*tc + 1*ts + 0*t);
}

static void pull_down (struct tilda_window_ *tw);

static void pull_up (struct tilda_window_ *tw);

void generate_animation_positions (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("generate_animation_positions");
    DEBUG_ASSERT (tw != NULL);

    gint i;
    gint last_pos_x = config_getint ("x_pos");
    gint last_pos_y = config_getint ("y_pos");

    GdkRectangle rectangle;
    config_get_configured_window_size (&rectangle);

    gint last_width = rectangle.width;
    gint last_height = rectangle.height;
    gint screen_width;
    gint screen_height;
    screen_size_get_dimensions (&screen_width, &screen_height);

    for (i=0; i<32; i++)
    {
        switch (config_getint ("animation_orientation"))
        {
        case 3: /* right->left RIGHT */
            animation_coordinates[ANIMATION_UP][ANIMATION_Y][i] = last_pos_y;
            animation_coordinates[ANIMATION_UP][ANIMATION_X][i] =
                    (gint)(screen_width + (last_pos_x - screen_width) * animation_ease_function_up(i, 32));
            animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i] = last_pos_y;
            animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i] =
                    (gint)(screen_width + (last_pos_x - screen_width) * animation_ease_function_down(i, 32));
            break;
        case 2: /* left->right LEFT */
            animation_coordinates[ANIMATION_UP][ANIMATION_Y][i] = last_pos_y;
            animation_coordinates[ANIMATION_UP][ANIMATION_X][i] =
                    (gint)(-last_width + (last_pos_x - -last_width) * animation_ease_function_up(i, 32));
            animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i] = last_pos_y;
            animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i] =
                    (gint)(-last_width + (last_pos_x - -last_width) * animation_ease_function_down(i, 32));
            break;
        case 1: /* bottom->top BOTTOM */
            animation_coordinates[ANIMATION_UP][ANIMATION_Y][i] =
                    (gint)(screen_height + (last_pos_y - screen_height) * animation_ease_function_up(i, 32));
            animation_coordinates[ANIMATION_UP][ANIMATION_X][i] = last_pos_x;
            animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i] =
                    (gint)(screen_height + (last_pos_y - screen_height) * animation_ease_function_down(i, 32));
            animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i] = last_pos_x;
            break;
        case 0: /* top->bottom TOP */
        default:
            animation_coordinates[ANIMATION_UP][ANIMATION_Y][i] =
                    (gint)(-last_height + (last_pos_y - -last_height) * animation_ease_function_up(i, 32));
            animation_coordinates[ANIMATION_UP][ANIMATION_X][i] = last_pos_x;
            animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i] =
                    (gint)(-last_height + (last_pos_y - -last_height) * animation_ease_function_down(i, 32));
            animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i] = last_pos_x;
            break;
        }
    }
}

/* Shamelessly adapted (read: ripped off) from gdk_window_focus() and
 * http://code.google.com/p/ttm/ trunk/src/window.c set_active()
 *
 * Also, more thanks to halfline and marnanel from irc.gnome.org #gnome
 * for their help in figuring this out.
 *
 * Thank you. And boo to metacity, because they keep breaking us.
 */

/* This function will make sure that tilda window becomes active (gains
 * the focus) when it is called.
 *
 * This has to be the worst possible way of making this work, but it was the
 * only way to get metacity to play nicely. All the other WM's are so nice,
 * why oh why does metacity hate us so?
 */
void tilda_window_set_active (tilda_window *tw)
{
    DEBUG_FUNCTION ("tilda_window_set_active");
    DEBUG_ASSERT (tw != NULL);

    GdkScreen *screen = gtk_widget_get_screen (tw->window);
    Display *x11_display = GDK_WINDOW_XDISPLAY (gdk_screen_get_root_window (screen));
    Window x11_window = GDK_WINDOW_XID (gtk_widget_get_window (tw->window) );
    Window x11_root_window = GDK_WINDOW_XID ( gdk_screen_get_root_window (screen) );

    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;
    gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    if (gdk_x11_screen_supports_net_wm_hint (screen,
                                             gdk_atom_intern_static_string ("_NET_ACTIVE_WINDOW")))
    {
        guint32 timestamp = gtk_get_current_event_time ();
        if (timestamp == 0) {
            timestamp = gdk_x11_get_server_time(gdk_screen_get_root_window (screen));
        }
        event.xclient.type = ClientMessage;
        event.xclient.serial = 0;
        event.xclient.send_event = True;
        event.xclient.display = x11_display;
        event.xclient.window = x11_window;
        event.xclient.message_type = gdk_x11_get_xatom_by_name ("_NET_ACTIVE_WINDOW");

        event.xclient.format = 32;
        event.xclient.data.l[0] = 2; /* pager */
        event.xclient.data.l[1] = timestamp; /* timestamp */
        event.xclient.data.l[2] = 0;
        event.xclient.data.l[3] = 0;
        event.xclient.data.l[4] = 0;

        gdk_x11_display_error_trap_push(gdk_display_get_default());
        XSendEvent (x11_display, x11_root_window, False, mask, &event);
        gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());
    }
    else
    {
        /* The WM doesn't support the EWMH standards. We'll print a warning and
         * try this, though it probably won't work... */
        g_printerr (_("WARNING: Window manager (%s) does not support EWMH hints\n"),
                    gdk_x11_screen_get_window_manager_name (screen));
        gdk_x11_display_error_trap_push(gdk_display_get_default());
        XRaiseWindow (x11_display, x11_window);
        gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());
    }
}

/* Process all pending GTK events, without returning to the GTK mainloop */
static void process_all_pending_gtk_events ()
{
    GdkDisplay *display = gdk_display_get_default ();

    while (gtk_events_pending ())
        gtk_main_iteration ();

    /* This is not strictly necessary, but I think it makes the animation
     * look a little smoother. However, it probably does increase the load
     * on the X server. */
    gdk_display_flush (display);
}

/**
* @force_hide: This option is used by the auto hide feature, so we can ignore the checks to focus tilda instead
* of pulling up.
*/
void pull (struct tilda_window_ *tw, enum pull_action action, gboolean force_hide)
{
    DEBUG_FUNCTION ("pull");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (action == PULL_UP || action == PULL_DOWN || action == PULL_TOGGLE);

    gboolean needsFocus = !tw->focus_loss_on_keypress
            && !gtk_window_is_active(GTK_WINDOW(tw->window))
            && !force_hide
            && !tw->hide_non_focused;

    if (g_get_monotonic_time() - tw->last_action_time < 150 * G_TIME_SPAN_MILLISECOND) {
        /* this is to prevent crazy toggling, with 150ms prevention time */
        return;
    }

    if (tw->current_state == STATE_DOWN && needsFocus) {
        /**
         * See tilda_window.c in focus_out_event_cb for an explanation about focus_loss_on_keypress
         * This conditional branch will only focus tilda but it does not actually pull the window up.
         */
        g_debug ("Tilda window not focused but visible");
        gdk_x11_window_set_user_time(gtk_widget_get_window(tw->window),
                tomboy_keybinder_get_current_event_time());
        tilda_window_set_active(tw);
        return;
    }

    if ((tw->current_state == STATE_UP) && action != PULL_UP) {
        pull_down (tw);
    } else if ((tw->current_state == STATE_DOWN) && action != PULL_DOWN) {
        pull_up (tw);
    }

    tw->last_action = action;
    tw->last_action_time = g_get_monotonic_time();
}

static void pull_up (struct tilda_window_ *tw) {
    tw->current_state = STATE_GOING_UP;

    gdk_x11_window_set_user_time (gtk_widget_get_window (tw->window),
                                  tomboy_keybinder_get_current_event_time());

    if (tw->fullscreen)
    {
        gtk_window_unfullscreen (GTK_WINDOW(tw->window));
    }

    if (config_getbool ("animation") && !tw->fullscreen) {
        GdkWindow *x11window;
        GdkDisplay *display;
        Atom atom;
        if (!config_getbool ("set_as_desktop")) {
            x11window = gtk_widget_get_window (tw->window);
            display = gdk_window_get_display (x11window);
            atom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE_DOCK");

            gdk_x11_display_error_trap_push(gdk_display_get_default());
            XChangeProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (x11window),
                             gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE"),
                             XA_ATOM, 32, PropModeReplace,
                             (guchar *) &atom, 1);
            gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());

            process_all_pending_gtk_events ();
        }
        gint slide_sleep_usec = config_getint ("slide_sleep_usec");
        for (guint i=0; i<32; i++) {
            gtk_window_move (GTK_WINDOW(tw->window),
                             animation_coordinates[ANIMATION_UP][ANIMATION_X][i],
                             animation_coordinates[ANIMATION_UP][ANIMATION_Y][i]);

            process_all_pending_gtk_events ();
            g_usleep (slide_sleep_usec);
        }

        if (!config_getbool ("set_as_desktop")) {
            x11window = gtk_widget_get_window (tw->window);
            display = gdk_window_get_display (x11window);
            if (config_getbool ("set_as_desktop")) {
                atom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE_DESKTOP");
            } else {
                atom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE_NORMAL");
            }

            gdk_x11_display_error_trap_push(gdk_display_get_default());
            XChangeProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (x11window),
                             gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE"),
                             XA_ATOM, 32, PropModeReplace,
                             (guchar *) &atom, 1);
            gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());
        }
    }

    /* All we have to do at this point is hide the window.
     * Case 1 - Animation on:  The window has moved outside the screen, just hide it
     * Case 2 - Animation off: Just hide the window */
    gtk_widget_hide (GTK_WIDGET(tw->window));

    g_debug ("pull_up(): MOVED UP");
    tw->current_state = STATE_UP;
}

static void pull_down (struct tilda_window_ *tw) {
    tw->current_state = STATE_GOING_DOWN;

    /* Keep things here just like they are. If you use gtk_window_present() here, you
     * will introduce some weird graphical glitches. Also, calling gtk_window_move()
     * before showing the window avoids yet more glitches. You should probably not use
     * gtk_window_show_all() here, as it takes a long time to execute.
     *
     * Overriding the user time here seems to work a lot better than calling
     * gtk_window_present_with_time() here, or at the end of the function. I have
     * no idea why, they should do the same thing. */
    gdk_x11_window_set_user_time (gtk_widget_get_window (tw->window),
                                  tomboy_keybinder_get_current_event_time());
    gtk_widget_show (GTK_WIDGET(tw->window));
#if GTK_MINOR_VERSION == 16
        /* Temporary fix for GTK breaking restore on Fullscreen, only needed for
         * GTK+ version 3.16, since it was fixed early in 3.17 and above. */
        tilda_window_set_fullscreen(tw);
#endif

    /* The window should maintain its properties when it is merely hidden, but it does
     * not. If you delete the following call, the window will not remain visible
     * on all workspaces after pull()ing it up and down a number of times.
     *
     * Note that the "Always on top" property doesn't seem to go away, only this
     * property (Show on all desktops) does... */
    if (config_getbool ("pinned"))
            gtk_window_stick (GTK_WINDOW (tw->window));

    if (config_getbool ("animation") && !tw->fullscreen) {
        GdkWindow *x11window;
        GdkDisplay *display;
        Atom atom;
        if (!config_getbool ("set_as_desktop")) {
            x11window = gtk_widget_get_window (tw->window);
            display = gdk_window_get_display (x11window);
            atom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE_DOCK");
            gdk_x11_display_error_trap_push(gdk_display_get_default());

            XChangeProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (x11window),
                             gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE"),
                             XA_ATOM, 32, PropModeReplace,
                             (guchar *) &atom, 1);
            process_all_pending_gtk_events ();
            gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());
        }
        gint slide_sleep_usec = config_getint ("slide_sleep_usec");
        for (guint i=0; i<32; i++) {
            gtk_window_move (GTK_WINDOW(tw->window),
                             animation_coordinates[ANIMATION_DOWN][ANIMATION_X][i],
                             animation_coordinates[ANIMATION_DOWN][ANIMATION_Y][i]);

            process_all_pending_gtk_events ();
            g_usleep (slide_sleep_usec);
        }

        if (!config_getbool ("set_as_desktop")) {
            x11window = gtk_widget_get_window (tw->window);
            display = gdk_window_get_display (x11window);
            if (config_getbool ("set_as_desktop")) {
                atom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE_DESKTOP");
            } else {
                atom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE_NORMAL");
            }

            gdk_x11_display_error_trap_push(gdk_display_get_default());
            XChangeProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (x11window),
                             gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_TYPE"),
                             XA_ATOM, 32, PropModeReplace,
                             (guchar *) &atom, 1);
            gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());
        }
    } else {
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Nasty code to make metacity behave. Starting at metacity-2.22 they "fixed" the
     * focus stealing prevention to make the old _NET_WM_USER_TIME hack
     * not work anymore. This is working for now... */
    tilda_window_set_active (tw);

    if (tw->fullscreen)
    {
        gtk_window_fullscreen (GTK_WINDOW(tw->window));
    }

    g_debug ("pull_down(): MOVED DOWN");
    tw->current_state = STATE_DOWN;
}

static void onKeybindingPull (G_GNUC_UNUSED const char *keystring, gpointer user_data)
{
    DEBUG_FUNCTION("onKeybindingPull");
    tilda_window *tw = TILDA_WINDOW(user_data);
    pull (tw, PULL_TOGGLE, FALSE);
}

gboolean tilda_keygrabber_bind (const gchar *keystr, tilda_window *tw)
{
    /* Empty strings are no good */
    if (keystr == NULL || strcmp ("", keystr) == 0)
        return FALSE;

    return tomboy_keybinder_bind (keystr, (TomboyBindkeyHandler)onKeybindingPull, tw);
}

void tilda_keygrabber_unbind (const gchar *keystr)
{
    tomboy_keybinder_unbind (keystr, (TomboyBindkeyHandler)onKeybindingPull);
}



/* vim: set ts=4 sts=4 sw=4 expandtab: */

