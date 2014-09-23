/* tomboykeybinder.c
* Copyright (C) 2008 Alex Graveley
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <string.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "eggaccelerators.h"
#include "tomboykeybinder.h"
#include "debug.h"

typedef struct _Binding {
	TomboyBindkeyHandler  handler;
	gpointer              user_data;
	char                 *keystring;
	unsigned int          keycode;
	unsigned int          modifiers;
} Binding;

static GSList *bindings = NULL;
static guint32 last_event_time = 0;
static gboolean processing_event = FALSE;

static guint num_lock_mask, caps_lock_mask, scroll_lock_mask;

static void
lookup_ignorable_modifiers (GdkKeymap *keymap)
{
	egg_keymap_resolve_virtual_modifiers (keymap,
					      EGG_VIRTUAL_LOCK_MASK,
					      &caps_lock_mask);

	egg_keymap_resolve_virtual_modifiers (keymap,
					      EGG_VIRTUAL_NUM_LOCK_MASK,
					      &num_lock_mask);

	egg_keymap_resolve_virtual_modifiers (keymap,
					      EGG_VIRTUAL_SCROLL_LOCK_MASK,
					      &scroll_lock_mask);
}

static void
grab_ungrab_with_ignorable_modifiers (GdkWindow *rootwin,
				      Binding   *binding,
				      gboolean   grab)
{
	guint mod_masks [] = {
		0, /* modifier only */
		num_lock_mask,
		caps_lock_mask,
		scroll_lock_mask,
		num_lock_mask  | caps_lock_mask,
		num_lock_mask  | scroll_lock_mask,
		caps_lock_mask | scroll_lock_mask,
		num_lock_mask  | caps_lock_mask | scroll_lock_mask,
	};
	guint i;

	for (i = 0; i < G_N_ELEMENTS (mod_masks); i++) {
		if (grab) {
			XGrabKey (GDK_WINDOW_XDISPLAY (rootwin),
				  binding->keycode,
				  binding->modifiers | mod_masks [i],
				  GDK_WINDOW_XID (rootwin),
				  False,
				  GrabModeAsync,
				  GrabModeAsync);
		} else {
			XUngrabKey (GDK_WINDOW_XDISPLAY (rootwin),
				    binding->keycode,
				    binding->modifiers | mod_masks [i],
				    GDK_WINDOW_XID (rootwin));
		}
	}
}

static gboolean
do_grab_key (Binding *binding)
{
	GdkKeymap *keymap = gdk_keymap_get_default ();
	GdkWindow *rootwin = gdk_get_default_root_window ();

	EggVirtualModifierType virtual_mods = 0;
	guint keysym = 0;

	if (keymap == NULL || rootwin == NULL)
		return FALSE;

	if (!egg_accelerator_parse_virtual (binding->keystring,
					    &keysym,
					    &virtual_mods))
		return FALSE;

	TRACE (g_print ("Got accel %d, %d\n", keysym, virtual_mods));

	binding->keycode = XKeysymToKeycode (GDK_WINDOW_XDISPLAY (rootwin),
					     keysym);
	if (binding->keycode == 0)
		return FALSE;

	TRACE (g_print ("Got keycode %d\n", binding->keycode));

	egg_keymap_resolve_virtual_modifiers (keymap,
					      virtual_mods,
					      &binding->modifiers);

	TRACE (g_print ("Got modmask %d\n", binding->modifiers));

	gdk_error_trap_push ();

	grab_ungrab_with_ignorable_modifiers (rootwin,
					      binding,
					      TRUE /* grab */);

	gdk_flush ();

	if (gdk_error_trap_pop ()) {
	   g_warning ("Binding '%s' failed!\n", binding->keystring);
	   return FALSE;
	}

	return TRUE;
}

static gboolean
do_ungrab_key (Binding *binding)
{
	GdkWindow *rootwin = gdk_get_default_root_window ();

	TRACE (g_print ("Removing grab for '%s'\n", binding->keystring));

	grab_ungrab_with_ignorable_modifiers (rootwin,
					      binding,
					      FALSE /* ungrab */);

	return TRUE;
}

static GdkFilterReturn
filter_func (GdkXEvent *gdk_xevent, G_GNUC_UNUSED GdkEvent *event, G_GNUC_UNUSED gpointer data) {
    XEvent *xevent = (XEvent *) gdk_xevent;
    guint event_mods;
    GSList *iter;

    switch (xevent->type) {
        case KeyPress:
            TRACE (g_print ("Got KeyPress! keycode: %d, modifiers: %d\n",
                    xevent->xkey.keycode,
                    xevent->xkey.state));

            /*
             * Set the last event time for use when showing
             * windows to avoid anti-focus-stealing code.
             */
            processing_event = TRUE;
            last_event_time = xevent->xkey.time;
            TRACE (g_print ("Current event time %d\n", last_event_time));

            event_mods = xevent->xkey.state & ~(num_lock_mask  |
                                caps_lock_mask |
                                scroll_lock_mask);

            for (iter = bindings; iter != NULL; iter = iter->next) {
                Binding *binding = (Binding *) iter->data;

                if (binding->keycode == xevent->xkey.keycode &&
                    binding->modifiers == event_mods) {

                    TRACE (g_print ("Calling handler for '%s'...\n",
                            binding->keystring));

                    (binding->handler) (binding->keystring,
                                binding->user_data);
                }
            }

            processing_event = FALSE;
            break;
        case KeyRelease:
            TRACE (g_print ("Got KeyRelease! \n"));
            break;
        default:
            break;
	}

	return GDK_FILTER_CONTINUE;
}

static void
keymap_changed (G_GNUC_UNUSED GdkKeymap *map)
{
	GdkKeymap *keymap = gdk_keymap_get_default ();
	GSList *iter;

	TRACE (g_print ("Keymap changed! Regrabbing keys..."));

	for (iter = bindings; iter != NULL; iter = iter->next) {
		Binding *binding = (Binding *) iter->data;
		do_ungrab_key (binding);
	}

	lookup_ignorable_modifiers (keymap);

	for (iter = bindings; iter != NULL; iter = iter->next) {
		Binding *binding = (Binding *) iter->data;
		do_grab_key (binding);
	}
}

void
tomboy_keybinder_init (void)
{
	GdkKeymap *keymap = gdk_keymap_get_default ();
	GdkWindow *rootwin = gdk_get_default_root_window ();

	lookup_ignorable_modifiers (keymap);

	gdk_window_add_filter (rootwin,
			       filter_func,
			       NULL);

	g_signal_connect (keymap,
			  "keys_changed",
			  G_CALLBACK (keymap_changed),
			  NULL);
}

gboolean
tomboy_keybinder_bind (const char           *keystring,
		       TomboyBindkeyHandler  handler,
		       gpointer              user_data)
{
	Binding *binding;
	gboolean success;

	binding = g_new0 (Binding, 1);
	binding->keystring = g_strdup (keystring);
	binding->handler = handler;
	binding->user_data = user_data;

	/* Sets the binding's keycode and modifiers */
	success = do_grab_key (binding);

	if (success) {
		bindings = g_slist_prepend (bindings, binding);
	} else {
		g_free (binding->keystring);
		g_free (binding);
	}

	return success;
}

void
tomboy_keybinder_unbind (const char           *keystring,
			 TomboyBindkeyHandler  handler)
{
	GSList *iter;

	for (iter = bindings; iter != NULL; iter = iter->next) {
		Binding *binding = (Binding *) iter->data;

		if (strcmp (keystring, binding->keystring) != 0 ||
		    handler != binding->handler)
			continue;

		do_ungrab_key (binding);

		bindings = g_slist_remove (bindings, binding);

		g_free (binding->keystring);
		g_free (binding);
		break;
	}
}

/*
 * From eggcellrenderkeys.c.
 */
gboolean
tomboy_keybinder_is_modifier (guint keycode)
{
	gint i;
	gint map_size;
	XModifierKeymap *mod_keymap;
	gboolean retval = FALSE;

	mod_keymap = XGetModifierMapping (gdk_x11_get_default_xdisplay ());

	map_size = 8 * mod_keymap->max_keypermod;

	i = 0;
	while (i < map_size) {
		if (keycode == mod_keymap->modifiermap[i]) {
			retval = TRUE;
			break;
		}
		++i;
	}

	XFreeModifiermap (mod_keymap);

	return retval;
}

guint32
tomboy_keybinder_get_current_event_time (void)
{
	if (processing_event)
		return last_event_time;
	else
		return GDK_CURRENT_TIME;
}
