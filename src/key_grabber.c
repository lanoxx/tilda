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

 /*
 * Some stolen from yeahconsole -- loving that open source :)
 *
 */

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tilda.h"
#include "key_grabber.h"

Display *dpy;
Window root;
Window win;
Window termwin;
Window last_focused;
int screen;
KeySym key;

void pull (struct tilda_window_ *tw)
{
    gint x, y;
    gint w, h;
    //gint min_h, max_h;

    if (x_pos_arg != -1)
        x = x_pos_arg;
    else
        x = tw->tc->x_pos;
    if (y_pos_arg != -1)
        y = y_pos_arg;
    else
        y = tw->tc->y_pos;

    gtk_window_get_size ((GtkWindow *) tw->window, &w, &h);

    if (h == tw->tc->min_height)
    {
        gdk_threads_enter();

        if (gtk_window_is_active ((GtkWindow *) tw->window) == FALSE)
            gtk_window_present ((GtkWindow *) tw->window);
        else
            gtk_widget_show ((GtkWidget *) tw->window);

        if ((strcasecmp (tw->tc->s_pinned, "true")) == 0)
            gtk_window_stick (GTK_WINDOW (tw->window));

        if ((strcasecmp (tw->tc->s_above, "true")) == 0)
            gtk_window_set_keep_above (GTK_WINDOW (tw->window), TRUE);

        gtk_window_move ((GtkWindow *) tw->window, x, y);

        /*for (max_h=h;max_h<=max_height;max_h++)
        {
            gtk_window_resize ((GtkWindow *) window, max_width, max_h);
            sleep (.01);
        }*/
        gtk_window_resize ((GtkWindow *) tw->window, tw->tc->max_width, tw->tc->max_height);
        gdk_flush ();
        gdk_threads_leave();
    }
    else if (h == tw->tc->max_height)
    {
        gdk_threads_enter();

        /*for (min_h=h;min_h>1;min_h--)
        {
            gtk_window_resize ((GtkWindow *) window, min_width, min_h);
            sleep (.001);
        }*/

        gtk_window_resize ((GtkWindow *) tw->window, tw->tc->min_width, tw->tc->min_height);
        gtk_widget_hide ((GtkWidget *) tw->window);

        gdk_flush ();
        gdk_threads_leave();
    }
}

void key_grab (tilda_window *tw)
{
    XModifierKeymap *modmap;
    unsigned int numlockmask = 0;
    unsigned int modmask = 0;
    int i, j;

    /* Key grabbing stuff taken from yeahconsole who took it from evilwm */
    modmap = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < modmap->max_keypermod; j++) {
            if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dpy, XK_Num_Lock)) {
                numlockmask = (1 << i);
            }
        }
    }
    XFreeModifiermap(modmap);

    if (strstr(tw->tc->s_key, "Control"))
        modmask = modmask | ControlMask;

    if (strstr(tw->tc->s_key, "Alt"))
        modmask = modmask | Mod1Mask;

    if (strstr(tw->tc->s_key, "Win"))
        modmask = modmask | Mod4Mask;

    if (strstr(tw->tc->s_key, "None"))
        modmask = 0;

    if (strtok(tw->tc->s_key, "+"))
        key = XStringToKeysym(strtok(NULL, "+"));

    XGrabKey(dpy, XKeysymToKeycode(dpy, key), modmask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, key), LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);

    if (numlockmask)
    {
        XGrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | modmask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);
    }
}

void *wait_for_signal (struct tilda_window_ *tw)
{
    KeySym grabbed_key;
    XEvent event;

    if (!(dpy = XOpenDisplay(NULL)))
        fprintf(stderr, "Shit -- can't open Display %s", XDisplayName(NULL));


    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    key_grab (tw);

    if (QUICK_STRCMP (tw->tc->s_down, "TRUE") == 0)
        pull (tw);
    else
        gtk_widget_hide (tw->window);

    for (;;)
    {
        XNextEvent(dpy, &event);

        switch (event.type)
        {
            case KeyPress:
                grabbed_key = XKeycodeToKeysym(dpy, event.xkey.keycode, 0);

                if (key == grabbed_key)
                {
                    pull (tw);
                    break;
                }
            default:
                break;
        }
    }

    return 0;
}

