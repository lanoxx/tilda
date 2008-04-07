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
 *
 */

/* Some stolen from yeahconsole -- loving that open source :) */

#include <tilda-config.h>

#include <debug.h>
#include <key_grabber.h>
#include <tilda.h>
#include <xerror.h>
#include <translation.h>
#include <configsys.h>
#include <tomboykeybinder.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <gtk/gtkmain.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkx.h>

#if DEBUG
#define debug_printf(args...) do { g_print ("debug: " args); } while (0)
#else
#define debug_printf(args...) do { /* nothing */ } while (0)
#endif


/* Define local variables here */
static gint posIV[4][16]; /* 0 - ypos, 1 - height, 2 - xpos, 3 - width */

void generate_animation_positions (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("generate_animation_positions");
    DEBUG_ASSERT (tw != NULL);

    /*
     * The slide positions are derived from FVWM sources, file fvwm/move_resize.c,
     * to there added by Greg J. Badros, gjb@cs.washington.edu
     */
    const float posCFV[] = {.01, .01,.02,.03,.08,.18,.3,.45,.65,.80,.88,.93,.95,.97,.99,1.0};

    gint i;
    gint last_pos_x = config_getint ("x_pos");
    gint last_pos_y = config_getint ("y_pos");
    gint last_width = config_getint ("max_width");
    gint last_height = config_getint ("max_height");

    for (i=0; i<16; i++)
    {
        switch (config_getint ("animation_orientation"))
        {
        case 3: /* right->left RIGHT */
            /*posIV[3][i] = (gint)(posCFV[i]*last_width); */
            /*posIV[2][i] = (gint)(last_pos_x+last_width-posIV[3][i]); */
            posIV[3][i] = last_width;
            posIV[2][i] = (gint)(last_pos_x+last_width-posCFV[i]*last_width);
            posIV[1][i] = last_height;
            posIV[0][i] = last_pos_y;
            break;
        case 2: /* left->right LEFT */
            /*posIV[3][i] = (gint)(posCFV[i]*last_width); */
            /*posIV[2][i] = last_pos_x; */
            posIV[3][i] = last_width;
            posIV[2][i] = (gint)(last_pos_x-last_width+posCFV[i]*last_width);
            posIV[1][i] = last_height;
            posIV[0][i] = last_pos_y;
            break;
        case 1: /* bottom->top BOTTOM */
            posIV[3][i] = last_width;
            posIV[2][i] = last_pos_x;
            posIV[1][i] = (gint)(posCFV[i]*last_height);
            posIV[0][i] = (gint)(last_pos_y+last_height-posIV[1][i]);
            break;
        case 0: /* top->bottom TOP */
        default:
            posIV[3][i] = last_width;
            posIV[2][i] = last_pos_x;
            posIV[1][i] = (gint)(posCFV[i]*last_height);
            posIV[0][i] = last_pos_y;
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

    Display *x11_display = GDK_WINDOW_XDISPLAY( tw->window->window );
    Window *x11_window = GDK_WINDOW_XWINDOW( tw->window->window );
    Window *x11_root_window = GDK_WINDOW_XWINDOW ( gtk_widget_get_root_window (tw->window) );
    GdkScreen *screen = gtk_widget_get_screen (tw->window);

    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    if (gdk_x11_screen_supports_net_wm_hint (screen,
                                             gdk_atom_intern_static_string ("_NET_ACTIVE_WINDOW")))
    {
        event.xclient.type = ClientMessage;
        event.xclient.serial = 0;
        event.xclient.send_event = True;
        event.xclient.display = x11_display;
        event.xclient.window = x11_window;
        event.xclient.message_type = gdk_x11_get_xatom_by_name ("_NET_ACTIVE_WINDOW");

        event.xclient.format = 32;
        event.xclient.data.l[0] = 2; /* pager */
        event.xclient.data.l[1] = tomboy_keybinder_get_current_event_time(); /* timestamp */
        event.xclient.data.l[2] = 0;
        event.xclient.data.l[3] = 0;
        event.xclient.data.l[4] = 0;

        XSendEvent (x11_display, x11_root_window, False, mask, &event);
    }
    else
    {
        /* The WM doesn't support the EWMH standards. We'll print a warning and
         * try this, though it probably won't work... */
        g_printerr ("WARNING: Window manager (%s) does not support EWMH hints\n",
                    gdk_x11_screen_get_window_manager_name (screen));
        XRaiseWindow (x11_display, x11_window);
    }
}

/* Process all pending GTK events, without returning to the GTK mainloop */
static void process_all_pending_gtk_events ()
{
    while (gtk_events_pending ())
        gtk_main_iteration ();

    /* This is not strictly necessary, but I think it makes the animation
     * look a little smoother. However, it probably does increase the load
     * on the X server. */
    gdk_flush ();
}

void pull (struct tilda_window_ *tw, enum pull_state state)
{
    DEBUG_FUNCTION ("pull");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (state == PULL_UP || state == PULL_DOWN || state == PULL_TOGGLE);

    gint i;

    if (tw->current_state == UP && state != PULL_UP)
    {
        /* Keep things here just like they are. If you use gtk_window_present() here, you
         * will introduce some weird graphical glitches. Also, calling gtk_window_move()
         * before showing the window avoids yet more glitches. You should probably not use
         * gtk_window_show_all() here, as it takes a long time to execute.
         *
         * Overriding the user time here seems to work a lot better than calling
         * gtk_window_present_with_time() here, or at the end of the function. I have
         * no idea why, they should do the same thing. */
        gdk_x11_window_set_user_time (GTK_WIDGET(tw->window)->window,
                                      tomboy_keybinder_get_current_event_time());
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
        gtk_widget_show (GTK_WIDGET(tw->window));

        /* Nasty code to make metacity behave. Starting at metacity-2.22 they "fixed" the
         * focus stealing prevention to make the old _NET_WM_USER_TIME hack
         * not work anymore. This is working for now... */
        tilda_window_set_active (tw);

        /* The window should maintain its properties when it is merely hidden, but it does
         * not. If you delete the following call, the window will not remain visible
         * on all workspaces after pull()ing it up and down a number of times.
         *
         * Note that the "Always on top" property doesn't seem to go away, only this
         * property (Show on all desktops) does... */
        if (config_getbool ("pinned"))
            gtk_window_stick (GTK_WINDOW (tw->window));

        if (config_getbool ("animation"))
        {
            for (i=0; i<16; i++)
            {
                gtk_window_move (GTK_WINDOW(tw->window), posIV[2][i], posIV[0][i]);
                gtk_window_resize (GTK_WINDOW(tw->window), posIV[3][i], posIV[1][i]);

                process_all_pending_gtk_events ();
                g_usleep (config_getint ("slide_sleep_usec"));
            }
        }

        debug_printf ("pull(): MOVED DOWN\n");
        tw->current_state = DOWN;
    }
    else if (state != PULL_DOWN)
    {
        if (config_getbool ("animation"))
        {
            for (i=15; i>=0; i--)
            {
                gtk_window_move (GTK_WINDOW(tw->window), posIV[2][i], posIV[0][i]);
                gtk_window_resize (GTK_WINDOW(tw->window), posIV[3][i], posIV[1][i]);

                process_all_pending_gtk_events ();
                g_usleep (config_getint ("slide_sleep_usec"));
            }
        }

        /* All we have to do at this point is hide the window.
         * Case 1 - Animation on:  The window has shrunk, just hide it
         * Case 2 - Animation off: Just hide the window */
        gtk_widget_hide (GTK_WIDGET(tw->window));

        debug_printf ("pull(): MOVED UP\n");
        tw->current_state = UP;
    }
}

static void onKeybindingPull (const char *keystring, gpointer user_data)
{
	tilda_window *tw = TILDA_WINDOW(user_data);
	pull (tw, PULL_TOGGLE);
}

gboolean tilda_keygrabber_bind (const gchar *keystr, tilda_window *tw)
{
    /* Empty strings are no good */
    if (strcmp ("", keystr) == 0)
        return FALSE;

    return tomboy_keybinder_bind (keystr, onKeybindingPull, tw);
}

void tilda_keygrabber_unbind (const gchar *keystr)
{
    tomboy_keybinder_unbind (keystr, onKeybindingPull);
}



/* vim: set ts=4 sts=4 sw=4 expandtab: */

