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
#include <gtk/gtk.h>
#include <X11/Xlib.h>

#include "eggaccelerators.h"
#include "tomboykeybinder.h"
#include "debug.h"

/**
 * This mask corresponds to the first 8 modifier masks of GdkModifierType
 * and is equal to a combination of all key masks as defined in the X
 * Windowing system. It can be used to remove virtual modifiers from a
 * list of virtual and real modifiers.
 */
#define REAL_MODIFIER_MASK (GDK_SHIFT_MASK \
                           | GDK_LOCK_MASK \
                           | GDK_CONTROL_MASK \
                           | GDK_MOD1_MASK \
                           | GDK_MOD2_MASK \
                           | GDK_MOD3_MASK \
                           | GDK_MOD4_MASK \
                           | GDK_MOD5_MASK)

typedef struct _Binding {
    TomboyBindkeyHandler handler;
    gpointer             user_data;
    char                *keystring;
    unsigned int         keycode;
    GdkModifierType      modifiers;
} Binding;

static GSList   *bindings        = NULL;
static Time  last_event_time  = 0;
static gboolean processing_event = FALSE;

/**
 * This is used to store the real modifier values for the virtual modifiers
 * num lock, caps lock and scroll lock.
 */
static GdkModifierType num_lock_mask, caps_lock_mask, scroll_lock_mask;

/**
 * Join virtual modifiers into a printable string separated by ' | '.
 *
 * @param virtual_modifiers
 * @return A string containing the string representation of all virtual modifiers separated by ' | '.
 */
static gchar *virtual_modifiers_as_string (GdkModifierType virtual_modifiers);

/**
 * Gdk does not allow us to determine the real modifiers for some
 * special virtual modifiers such as num lock and scroll look.
 * Therefore we will need to determine these our self.
 *
 * @param keymap
 */
static void
lookup_ignorable_modifiers (GdkKeymap *keymap)
{
    gchar *string;

    egg_keymap_resolve_virtual_modifiers (keymap,
                                          EGG_VIRTUAL_LOCK_MASK,
                                          &caps_lock_mask);

    string = virtual_modifiers_as_string (caps_lock_mask);
    g_debug ("Virtual modifier for 'caps_lock_mask' set to: %s", string);
    g_free (string);

    egg_keymap_resolve_virtual_modifiers (keymap,
                                          EGG_VIRTUAL_NUM_LOCK_MASK,
                                          &num_lock_mask);

    string = virtual_modifiers_as_string (num_lock_mask);
    g_debug ("Virtual modifier for 'num_lock_mask' set to: %s", string);
    g_free (string);

    egg_keymap_resolve_virtual_modifiers (keymap,
                                          EGG_VIRTUAL_SCROLL_LOCK_MASK,
                                          &scroll_lock_mask);

    string = virtual_modifiers_as_string (scroll_lock_mask);
    g_debug ("Virtual modifier for 'scroll_lock_mask' set to: %s", string);
    g_free (string);
}

static void
grab_ungrab_with_ignorable_modifiers (GdkWindow *rootwin,
                                      Binding *binding,
                                      gboolean grab)
{
    guint mod_masks[] = {
            0, /* modifier only */
            num_lock_mask,
            caps_lock_mask,
            scroll_lock_mask,
            num_lock_mask | caps_lock_mask,
            num_lock_mask | scroll_lock_mask,
            caps_lock_mask | scroll_lock_mask,
            num_lock_mask | caps_lock_mask | scroll_lock_mask,
    };
    guint i;

    for (i = 0; i < G_N_ELEMENTS (mod_masks); i++) {
        if (grab) {
            XGrabKey (GDK_WINDOW_XDISPLAY (rootwin),
                      binding->keycode,
                      binding->modifiers | mod_masks[i],
                      GDK_WINDOW_XID (rootwin),
                      False,
                      GrabModeAsync,
                      GrabModeAsync);
        } else {
            XUngrabKey (GDK_WINDOW_XDISPLAY (rootwin),
                        binding->keycode,
                        binding->modifiers | mod_masks[i],
                        GDK_WINDOW_XID (rootwin));
        }
    }
}

static gboolean
do_grab_key (Binding *binding)
{
    GdkDisplay *display = gdk_display_get_default ();
    GdkKeymap *keymap  = gdk_keymap_get_for_display (display);
    GdkWindow *rootwin = gdk_get_default_root_window ();

    GdkModifierType virtual_mods = (GdkModifierType) 0;
    guint           keysym       = 0;

    if (keymap == NULL || rootwin == NULL)
        return FALSE;

    gtk_accelerator_parse (binding->keystring,
                           &keysym,
                           &virtual_mods);

    if (keysym == 0 && virtual_mods == 0) {
        return FALSE;
    }

    binding->keycode = XKeysymToKeycode (GDK_WINDOW_XDISPLAY (rootwin),
                                         keysym);
    if (binding->keycode == 0)
        return FALSE;

    GdkModifierType mapped_modifiers = virtual_mods;

    gdk_keymap_map_virtual_modifiers (keymap,
                                      &mapped_modifiers);

    // mask out virtual modifiers so we get only the real modifiers
    binding->modifiers = mapped_modifiers & REAL_MODIFIER_MASK;

    gdk_x11_display_error_trap_push (display);

    grab_ungrab_with_ignorable_modifiers (rootwin,
                                          binding,
                                          TRUE /* grab */);

    gdk_display_flush (display);

    if (gdk_x11_display_error_trap_pop (display)) {
        g_warning ("Binding '%s' failed!\n", binding->keystring);
        return FALSE;
    }

    gchar *virtual_modifier_string = virtual_modifiers_as_string (virtual_mods);
    g_debug ("Resolved accelerator: %s. KeySymbol: %d, Virtual Modifiers: %s",
             binding->keystring,
             keysym,
             virtual_modifier_string);
    g_free (virtual_modifier_string);

    g_debug ("Binding to keycode %d", binding->keycode);

    gchar *real_modifiers_as_string = virtual_modifiers_as_string (
            (GdkModifierType) binding->modifiers);
    g_debug ("Binding to real modifier mask %d (%s)", binding->modifiers,
             real_modifiers_as_string);
    g_free (real_modifiers_as_string);

    return TRUE;
}

static gboolean
do_ungrab_key (Binding *binding)
{
    GdkWindow *rootwin = gdk_get_default_root_window ();

    g_debug ("Removing grab for '%s'", binding->keystring);

    grab_ungrab_with_ignorable_modifiers (rootwin,
                                          binding,
                                          FALSE /* ungrab */);

    return TRUE;
}

static GdkFilterReturn
filter_func (GdkXEvent *gdk_xevent, G_GNUC_UNUSED GdkEvent *event,
             G_GNUC_UNUSED gpointer data)
{
    XEvent *xevent = (XEvent *) gdk_xevent;
    guint  event_mods;
    GSList *iter;

    switch (xevent->type) {
        case KeyPress:
            g_debug ("Got KeyPress! keycode: %d, modifiers: %d",
                     xevent->xkey.keycode,
                     xevent->xkey.state);

            /*
             * Set the last event time for use when showing
             * windows to avoid anti-focus-stealing code.
             */
            processing_event = TRUE;
            last_event_time  = xevent->xkey.time;

            g_debug ("Current event time %ld", last_event_time);

            event_mods = xevent->xkey.state & ~(num_lock_mask |
                                                caps_lock_mask |
                                                scroll_lock_mask);

            for (iter = bindings; iter != NULL; iter = iter->next) {
                Binding *binding = (Binding *) iter->data;

                if (binding->keycode == xevent->xkey.keycode &&
                    binding->modifiers == event_mods) {

                    g_debug ("Calling handler for '%s'...",
                             binding->keystring);

                    (binding->handler) (binding->keystring,
                                        binding->user_data);
                }
            }

            processing_event = FALSE;
            break;
        case KeyRelease:
            g_debug ("Got KeyRelease!");
            break;
        default:
            break;
    }

    return GDK_FILTER_CONTINUE;
}

/**
 * This is invoked if the keymap changes, for example if some virtual
 * modifier are remapped to real modifiers with `xmodmap`.
 *
 * To how this function work one can set the virtual modifier 'Scroll_Lock'
 * to the real modifier 'mod3' and clear it again:
 *
 *     xmodmap -e "add mod3 = Scroll_Lock"
 *     xmodmap -e "clear mod3"
 *
 * This will cause this callback to be invoked.
 *
 * @param map Pointer to the current key map.
 */
static void
keymap_changed (GdkKeymap *map)
{
    GSList *iter;

    g_debug ("Keymap changed! Regrabbing keys...");

    for (iter = bindings; iter != NULL; iter = iter->next) {
        Binding *binding = (Binding *) iter->data;
        do_ungrab_key (binding);
    }

    lookup_ignorable_modifiers (map);

    for (iter = bindings; iter != NULL; iter = iter->next) {
        Binding *binding = (Binding *) iter->data;
        do_grab_key (binding);
    }
}

void
tomboy_keybinder_init (void)
{
    GdkDisplay *display = gdk_display_get_default ();
    GdkKeymap *keymap  = gdk_keymap_get_for_display (display);
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
tomboy_keybinder_bind (const char *keystring,
                       TomboyBindkeyHandler handler,
                       gpointer user_data)
{
    Binding  *binding;
    gboolean success;

    binding = g_new0 (Binding, 1);
    binding->keystring = g_strdup (keystring);
    binding->handler   = handler;
    binding->user_data = user_data;

    /* Sets the binding's keycode and modifiers */
    success = do_grab_key (binding);

    if (success) {

        g_debug("Added binding. Keystring: %s", binding->keystring);

        bindings = g_slist_prepend (bindings, binding);
    } else {
        g_free (binding->keystring);
        g_free (binding);
    }

    return success;
}

void
tomboy_keybinder_unbind (const char *keystring,
                         TomboyBindkeyHandler handler)
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

guint32
tomboy_keybinder_get_current_event_time (void)
{
    if (processing_event)
        return last_event_time;
    else
        return GDK_CURRENT_TIME;
}

/**
 * Join virtual modifiers into a printable string separated by ' | '.
 *
 * Glib has added a similar function called: `g_flags_to_string` in version 2.54.
 * At some point when it is reasonable to bump our glib dependency to 2.54 we
 * can replace this function with the glib version.
 *
 * @param virtual_modifiers
 * @return A string containing the string representation of all virtual modifiers separated by ' | '.
 */
static gchar *virtual_modifiers_as_string (GdkModifierType virtual_modifiers)
{
    int modifier = 1;
    int index    = 0;

    GFlagsClass *flags_class;
    GFlagsValue *enum_value;

    g_return_val_if_fail (G_TYPE_IS_FLAGS (GDK_TYPE_MODIFIER_TYPE), NULL);

    flags_class = g_type_class_ref (GDK_TYPE_MODIFIER_TYPE);

    const char **modifierStrings = g_malloc0_n (sizeof (char *),
                                                flags_class->n_values);

    while (modifier < GDK_RELEASE_MASK) {
        enum_value = g_flags_get_first_value (G_FLAGS_CLASS (flags_class),
                                              virtual_modifiers & modifier);

        if (enum_value) {
            modifierStrings[index] = enum_value->value_name;
        }

        if (modifierStrings[index]) {
            index++;
        }

        modifier = modifier << 1;
    }

    char *result = g_strjoinv (" | ", (gchar **) modifierStrings);

    g_free (modifierStrings);
    g_type_class_unref (flags_class);

    return result;
}
