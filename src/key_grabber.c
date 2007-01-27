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
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tilda.h"
#include "config.h"
#include "key_grabber.h"
#include "xerror.h"
#include "../tilda-config.h"

/* Define local variables here */
static Display *dpy;
static Window root;
static Window win;
static Window termwin;
static Window last_focused;
static int screen;
static KeySym key;

static unsigned int display_width, display_height;

/*
 * The slide positions are derived from FVMW sources, file fvmm/move_resize.c,
 * to there added by Greg J. Badros, gjb@cs.washington.edu
 */

static float posCFV[] = {.005,.01,.02,.03,.08,.18,.3,.45,.65,.80,.88,.93,.95,.97,.99,1.0};
/* 0 - ypos, 1 - height, 2 - xpos, 3 - width */
static gint posIV[4][16]; 

void generate_animation_positions (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("generate_animation_positions");
    DEBUG_ASSERT (tw != NULL);

    gint i;
    gint last_pos_x = cfg_getint (tw->tc, "x_pos");
    gint last_pos_y = cfg_getint (tw->tc, "y_pos");
    gint last_width = cfg_getint (tw->tc, "max_width");
    gint last_height = cfg_getint (tw->tc, "max_height");

    for (i=0; i<16; i++)
    {
        switch (cfg_getint (tw->tc, "animation_orientation"))
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

static void pull_state (struct tilda_window_ *tw, int state)
{
    DEBUG_FUNCTION ("pull_state");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (state == PULL_UP || state == PULL_DOWN || state == PULL_TOGGLE);

    gint i;
    gint w, h;
    static gint pos=0;

    if (pos == 0 && state != PULL_UP)
    {
        gdk_threads_enter();

        pos++;

        if (gtk_window_is_active ((GtkWindow *) tw->window) == FALSE)
            gtk_window_present ((GtkWindow *) tw->window);
        else
            gtk_widget_show ((GtkWidget *) tw->window);

        if (cfg_getbool (tw->tc, "pinned"))
            gtk_window_stick (GTK_WINDOW (tw->window));

        if (cfg_getbool (tw->tc, "above"))
            gtk_window_set_keep_above (GTK_WINDOW (tw->window), TRUE);

        if (cfg_getbool (tw->tc, ("animation")))
        {
            gdk_threads_leave();

            for (i=0; i<16; i++)
            {
                gdk_threads_enter();
                gtk_window_move ((GtkWindow *) tw->window, posIV[2][i], posIV[0][i]);
                gtk_window_resize ((GtkWindow *) tw->window, posIV[3][i], posIV[1][i]);

                gdk_flush();
                gdk_threads_leave();
                usleep (cfg_getint (tw->tc, "slide_sleep_usec"));
            }
        }
        else
        {
            //gtk_window_move ((GtkWindow *) tw->window, cfg_getint (tw->tc, "x_pos"), cfg_getint (tw->tc, "y_pos"));
            //gtk_window_resize ((GtkWindow *) tw->window, cfg_getint (tw->tc, "max_width"), cfg_getint (tw->tc, "max_height"));
            gdk_flush();
            gdk_threads_leave();
        }

        gdk_threads_enter();

        gdk_window_focus (tw->window->window, gtk_get_current_event_time ());

        gdk_flush ();
        gdk_threads_leave();
    }
    else if (state != PULL_DOWN)
    {
        gdk_threads_enter();

        pos--;

        if (cfg_getbool (tw->tc, ("animation")))
        {
            gdk_threads_leave();
            for (i=15; i>=0; i--)
            {
                gdk_threads_enter();
                gtk_window_move ((GtkWindow *) tw->window, posIV[2][i], posIV[0][i]);
                gtk_window_resize ((GtkWindow *) tw->window, posIV[3][i], posIV[1][i]);

                gdk_flush();
                gdk_threads_leave();
                usleep (cfg_getint (tw->tc, "slide_sleep_usec"));
            }
        }
        else
        {
            //gtk_window_resize ((GtkWindow *) tw->window, cfg_getint (tw->tc, "min_width"), cfg_getint (tw->tc, "min_height"));
            gdk_flush();
            gdk_threads_leave();
        }

        gdk_threads_enter();

        gtk_widget_hide ((GtkWidget *) tw->window);

        gdk_flush ();
        gdk_threads_leave();
    }
}

//  Wrapper function so pull() calls without a state toggle like they used to
void pull (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("pull");
    DEBUG_ASSERT (tw != NULL);

    pull_state (tw, PULL_TOGGLE);
}

static int parse_key (tilda_window *tw, gchar *key_str, KeySym *key_ret)
{
    DEBUG_FUNCTION ("parse_key");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (key_str != NULL);
    DEBUG_ASSERT (key_ret != NULL);

    gchar tmp_key[32];
    KeySym key = NoSymbol;
    gchar *key_ptr = key_str;

    /* Exceptions to "normal" keys go here, such as the "grave" key */
    if (g_strrstr (key_str, "`"))        { key_ptr = "grave"; }

    /* Now, try to have XStringToKeysym() parse it */
    key = XStringToKeysym (key_ptr);

    /* Check for failure */
    if (key == NoSymbol)
    {
        /* Use default */
        fprintf (stderr, "Bad key: \"%s\" not recognized. Using default of F%d "
                         " (leaving your modifiers as-is).\n", key_ptr, tw->instance+1);
        g_snprintf (tmp_key, sizeof(tmp_key), "F%d", tw->instance+1);

        /* Re-parse the default key */
        key = XStringToKeysym (tmp_key);

        if (key == NoSymbol)
        {
            fprintf (stderr, "Using the default failed, bailing out...\n");
            exit (EXIT_FAILURE);
        }
    }

    *key_ret = key;

    return 0;
}

static int parse_keybinding (tilda_window *tw, guint *modmask_ret, KeySym *key_ret)
{
    DEBUG_FUNCTION ("parse_keybinding");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (modmask_ret != NULL);
    DEBUG_ASSERT (key_ret != NULL);

    guint modmask = 0;
    KeySym key = NoSymbol;
    gchar *key_str;
    gchar **key_split;
    gint i;

    key_str = g_strdup (cfg_getstr (tw->tc, "key"));
    key_str = g_strstrip (key_str);
    key_split = g_strsplit (key_str, "+", 3);

    for (i=0; i<g_strv_length(key_split); i++)
    {
        g_strstrip (key_split[i]);

        /* Now each key_split[i] can be either a mask or a key */
        if      (g_strrstr (key_split[i], "None"))    { modmask = 0; }
        else if (g_strrstr (key_split[i], "Control")) { modmask |= ControlMask; }
        else if (g_strrstr (key_split[i], "Alt"))     { modmask |= Mod1Mask; }
        else if (g_strrstr (key_split[i], "Win"))     { modmask |= Mod4Mask; }
        else /* Must be a key */
        {
            parse_key (tw, key_split[i], &key);
        }
    }

    g_free (key_str);
    g_strfreev (key_split);

    *key_ret = key;
    *modmask_ret = modmask;

    return 0;
}

/*
 * This will grab the key from the X server for exclusive use by
 * our application.
 */
gint key_grab (tilda_window *tw)
{
    DEBUG_FUNCTION ("key_grab");
    DEBUG_ASSERT (tw != NULL);

    XModifierKeymap *modmap;
    guint numlockmask = 0;
    guint modmask = 0;
    gint i, j;

    /* Key grabbing stuff taken from yeahconsole who took it from evilwm */
    modmap = XGetModifierMapping(dpy);

    for (i = 0; i < 8; i++)
        for (j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dpy, XK_Num_Lock))
                numlockmask = (1 << i);

    XFreeModifiermap(modmap);

    /* Parse the keybinding from the config file.
     *
     * NOTE: the variable "key" is global, so it can be used here and
     * in the wait_for_signal() function below.
     */
    parse_keybinding (tw, &modmask, &key);

    /* Bind the key(s) appropriately.
     *
     * NOTE: If you're getting BadAccess errors from here in the code, then
     * something in your environment has already bound this key! Use a different
     * keybinding.
     *
     * Unfortunately, we can't report this to the user, since X kills the program
     * automatically when it recieves a BadAccess error. :(
     */

    /* We are ignoring errors from X here, since we can get BadAccess errors. These will be
     * reported, but we'll check them later. */
    xerror_set_ignore (dpy, TRUE);

    /* NOTE: This grabs the key defined for both the Caps-Lock on and Caps-Lock off case. */
    XGrabKey(dpy, XKeysymToKeycode(dpy, key), modmask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, key), LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);

    if (numlockmask)
    {
        /* NOTE: This grabs the key defined for both the Num-Lock on and Num-Lock off case. */
        XGrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | modmask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | LockMask | modmask, root, True, GrabModeAsync, GrabModeAsync);
    }

    xerror_set_ignore (dpy, FALSE);

    if (xerror_occurred)
    {
        /* Clear the error */
        xerror_occurred = FALSE;

        /* An error occurred, so we need to BE NOISY */
        fprintf (stderr, "Error: key grabbing failed!\n");
        key_ungrab (tw); /* Try to ungrab the keys */

        /* TODO: call the wizard ??? */
        return 1;
    }

    return 0;
}

/*
 * This will "ungrab" the key that is currently being used by the application
 * to toggle the window state.
 *
 * This should be used in the wizard just before the key grabber button is
 * pressed, so that the user can press the same key that they are using.
 */
gint key_ungrab (tilda_window *tw)
{
    DEBUG_FUNCTION ("key_ungrab");
    DEBUG_ASSERT (tw != NULL);

    XModifierKeymap *modmap;
    guint numlockmask = 0;
    guint modmask = 0;
    gint i, j;

    /* Key grabbing stuff taken from yeahconsole who took it from evilwm */
    modmap = XGetModifierMapping(dpy);

    for (i = 0; i < 8; i++)
        for (j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dpy, XK_Num_Lock))
                numlockmask = (1 << i);

    XFreeModifiermap(modmap);

    /* Parse the keybinding from the config file.
     *
     * NOTE: the variable "key" is global, so it can be used here and
     * in the wait_for_signal() function below.
     */
    parse_keybinding (tw, &modmask, &key);

    xerror_set_ignore (dpy, TRUE);

    /* Unbind the key(s) appropriately. */
    XUngrabKey(dpy, XKeysymToKeycode(dpy, key), modmask, root);
    XUngrabKey(dpy, XKeysymToKeycode(dpy, key), LockMask | modmask, root);

    if (numlockmask)
    {
        XUngrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | modmask, root);
        XUngrabKey(dpy, XKeysymToKeycode(dpy, key), numlockmask | LockMask | modmask, root);
    }

    xerror_set_ignore (dpy, FALSE);

    if (xerror_occurred)
    {
        /* Clear the error */
        xerror_occurred = FALSE;

        /* FIXME: Be NOISY */
        fprintf (stderr, "Error: key ungrabbing failed!\n");
        return 1;
    }

    return 0;
}

void *wait_for_signal (tilda_window *tw)
{
    DEBUG_FUNCTION ("wait_for_signal");
    DEBUG_ASSERT (tw != NULL);

    KeySym grabbed_key;
    XEvent event;

    if (!(dpy = XOpenDisplay(NULL)))
    {
        DEBUG_ERROR ("Cannot open display");
        fprintf (stderr, "Cannot open Display %s", XDisplayName(NULL));
        exit (EXIT_FAILURE);
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    display_width = DisplayWidth(dpy, screen);
    display_height = DisplayHeight(dpy, screen);

    key_grab (tw);

    if (cfg_getbool (tw->tc, "hidden"))
    {
        /*
         * Fixes bug that causes Tilda to eat up 100% of CPU
         * if set to stay hidden when started
         */
        pull_state(tw, PULL_DOWN);
        pull_state(tw, PULL_UP);
    }
    else
        pull_state(tw, PULL_DOWN);

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

