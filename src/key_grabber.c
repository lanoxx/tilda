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

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define local variables here */
static Display *dpy;
static Window root;
static int screen;
static KeySym key;


static gint posIV[4][16]; /* 0 - ypos, 1 - height, 2 - xpos, 3 - width */

void generate_animation_positions (struct tilda_window_ *tw)
{
    DEBUG_FUNCTION ("generate_animation_positions");
    DEBUG_ASSERT (tw != NULL);

    /*
     * The slide positions are derived from FVWM sources, file fvwm/move_resize.c,
     * to there added by Greg J. Badros, gjb@cs.washington.edu
     */
    const float posCFV[] = {.005,.01,.02,.03,.08,.18,.3,.45,.65,.80,.88,.93,.95,.97,.99,1.0};

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

void pull (struct tilda_window_ *tw, enum pull_state state)
{
    DEBUG_FUNCTION ("pull");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (state == PULL_UP || state == PULL_DOWN || state == PULL_TOGGLE);

    gint i;
    static gint pos=0;

    if (pos == 0 && state != PULL_UP)
    {
        pos++;

        if (gtk_window_is_active ((GtkWindow *) tw->window) == FALSE)
            gtk_window_present ((GtkWindow *) tw->window);
        else
            gtk_widget_show ((GtkWidget *) tw->window);

        if (config_getbool ("pinned"))
            gtk_window_stick (GTK_WINDOW (tw->window));

        if (config_getbool ("above"))
            gtk_window_set_keep_above (GTK_WINDOW (tw->window), TRUE);

        if (config_getbool ("animation"))
        {
            for (i=0; i<16; i++)
            {
                gtk_window_move ((GtkWindow *) tw->window, posIV[2][i], posIV[0][i]);
                gtk_window_resize ((GtkWindow *) tw->window, posIV[3][i], posIV[1][i]);

                gdk_flush();
                g_usleep (config_getint ("slide_sleep_usec"));
            }
        }
        else
        {
            gdk_flush();
        }

        gdk_window_focus (tw->window->window, gtk_get_current_event_time ());
        gdk_flush ();
    }
    else if (state != PULL_DOWN)
    {
        pos--;

        if (config_getbool ("animation"))
        {
            for (i=15; i>=0; i--)
            {
                gtk_window_move ((GtkWindow *) tw->window, posIV[2][i], posIV[0][i]);
                gtk_window_resize ((GtkWindow *) tw->window, posIV[3][i], posIV[1][i]);

                gdk_flush();
                g_usleep (config_getint ("slide_sleep_usec"));
            }
        }
        else
        {
            gdk_flush();
        }

        gtk_widget_hide ((GtkWidget *) tw->window);
        gdk_flush ();
    }
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
        const char *msg = _("Bad key: \"%s\" not recognized. Using default of F%d (leaving your modifiers unmodified)\n");
        fprintf (stderr, msg, key_ptr, tw->instance+1);
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

    key_str = g_strdup (config_getstr ("key"));
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
        fprintf (stderr, _("Error: key grabbing failed!\n"));
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
        fprintf (stderr, _("Error: key ungrabbing failed!\n"));
        return 1;
    }

    return 0;
}

static gboolean wfs_pull_toggle_cb (gpointer data)
{
    tilda_window *tw = (tilda_window *)data;

    /* Toggle the window state */
    pull (tw, PULL_TOGGLE);

    /* We only want to call this once, so we return false */
    return FALSE;
}


/**
 * This needs to be called from the main thread, before the
 * wait_for_signal() thread is started.
 */
void wait_for_signal_init (tilda_window *tw)
{
    key_grab (tw);

    if (config_getbool ("hidden"))
    {
        /*
         * Fixes bug that causes Tilda to eat up 100% of CPU
         * if set to stay hidden when started
         */
        pull(tw, PULL_DOWN);
        pull(tw, PULL_UP);
    }
    else
        pull(tw, PULL_DOWN);
}


/**
 * IMPORTANT: you are not allowed to call any GTK+ functions, or
 * any functions which will call GTK+ from this function, since it
 * will always be running in a seperate thread.
 *
 * If you need to call GTK+, create a new static function above,
 * and "call" it with g_idle_add() to have the Glib mainloop call it
 * asynchronously, from the main thread. This keeps the GUI processing
 * in a single thread, removing the need for gdk_threads_enter() and
 * gdk_threads_leave() to protect all GTK+ function calls.
 */
void *wait_for_signal (tilda_window *tw)
{
    DEBUG_FUNCTION ("wait_for_signal");
    DEBUG_ASSERT (tw != NULL);

    KeySym grabbed_key;
    XEvent event;

    for (;;)
    {
        XNextEvent(dpy, &event);

        switch (event.type)
        {
            case KeyPress:
                grabbed_key = XKeycodeToKeysym(dpy, event.xkey.keycode, 0);

                /* If this is the right key, then add the pull-handler to the Glib
                 * main loop to be called at a later time from the main thread. */
                if (key == grabbed_key)
                    g_idle_add (&wfs_pull_toggle_cb, tw);

                break;
            default:
                break;
        }
    }

    return 0;
}

/**
 * Initialize the global variables needed by all of the key grabber stuff.
 *
 * We have global variables so that we don't have to keep creating connections
 * to the X Server.
 */
gint init_key_grabber ()
{
    if (!(dpy = XOpenDisplay (NULL)))
    {
        DEBUG_ERROR ("Cannot open display");
        fprintf (stderr, _("Cannot open Display %s"), XDisplayName (NULL));
        exit (EXIT_FAILURE);
    }

    screen = DefaultScreen (dpy);
    root = RootWindow (dpy, screen);

    return 0; /* success */
}

/* vim: set ts=4 sts=4 sw=4 expandtab: */

