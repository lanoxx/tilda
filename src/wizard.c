/* vim: set ts=4 sts=4 sw=4 expandtab nowrap: */
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

#include <tilda-config.h>

#include <debug.h>
#include <tilda.h>
#include <wizard.h>
#include <key_grabber.h>
#include <configsys.h>
#include <callback_func.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <vte/vte.h> /* VTE_* constants, mostly */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

/* INT_MAX */
#include <limits.h>

const GdkColor
terminal_palette_tango[TERMINAL_PALETTE_SIZE] =
{
  { 0, 0x2e2e, 0x3434, 0x3636 },
  { 0, 0xcccc, 0x0000, 0x0000 },
  { 0, 0x4e4e, 0x9a9a, 0x0606 },
  { 0, 0xc4c4, 0xa0a0, 0x0000 },
  { 0, 0x3434, 0x6565, 0xa4a4 },
  { 0, 0x7575, 0x5050, 0x7b7b },
  { 0, 0x0606, 0x9820, 0x9a9a },
  { 0, 0xd3d3, 0xd7d7, 0xcfcf },
  { 0, 0x5555, 0x5757, 0x5353 },
  { 0, 0xefef, 0x2929, 0x2929 },
  { 0, 0x8a8a, 0xe2e2, 0x3434 },
  { 0, 0xfcfc, 0xe9e9, 0x4f4f },
  { 0, 0x7272, 0x9f9f, 0xcfcf },
  { 0, 0xadad, 0x7f7f, 0xa8a8 },
  { 0, 0x3434, 0xe2e2, 0xe2e2 },
  { 0, 0xeeee, 0xeeee, 0xecec }
};

const GdkColor
terminal_palette_zenburn[TERMINAL_PALETTE_SIZE] = {
    {0, 0x2222, 0x2222, 0x2222 }, //black
    {0, 0x8080, 0x3232, 0x3232 }, //darkred
    {0, 0x5b5b, 0x7676, 0x2f2f }, //darkgreen
    {0, 0xaaaa, 0x9999, 0x4343 }, //brown
    {0, 0x3232, 0x4c4c, 0x8080 }, //darkblue
    {0, 0x7070, 0x6c6c, 0x9a9a }, //darkmagenta
    {0, 0x9292, 0xb1b1, 0x9e9e }, //darkcyan
    {0, 0xffff, 0xffff, 0xffff }, //lightgrey
    {0, 0x2222, 0x2222, 0x2222 }, //darkgrey
    {0, 0x9898, 0x2b2b, 0x2b2b }, //red
    {0, 0x8989, 0xb8b8, 0x3f3f }, //green
    {0, 0xefef, 0xefef, 0x6060 }, //yellow
    {0, 0x2b2b, 0x4f4f, 0x9898 }, //blue
    {0, 0x8282, 0x6a6a, 0xb1b1 }, //magenta
    {0, 0xa1a1, 0xcdcd, 0xcdcd }, //cyan
    {0, 0xdede, 0xdede, 0xdede }, //white}
};

const GdkColor
terminal_palette_linux[TERMINAL_PALETTE_SIZE] =
{
  { 0, 0x0000, 0x0000, 0x0000 },
  { 0, 0xaaaa, 0x0000, 0x0000 },
  { 0, 0x0000, 0xaaaa, 0x0000 },
  { 0, 0xaaaa, 0x5555, 0x0000 },
  { 0, 0x0000, 0x0000, 0xaaaa },
  { 0, 0xaaaa, 0x0000, 0xaaaa },
  { 0, 0x0000, 0xaaaa, 0xaaaa },
  { 0, 0xaaaa, 0xaaaa, 0xaaaa },
  { 0, 0x5555, 0x5555, 0x5555 },
  { 0, 0xffff, 0x5555, 0x5555 },
  { 0, 0x5555, 0xffff, 0x5555 },
  { 0, 0xffff, 0xffff, 0x5555 },
  { 0, 0x5555, 0x5555, 0xffff },
  { 0, 0xffff, 0x5555, 0xffff },
  { 0, 0x5555, 0xffff, 0xffff },
  { 0, 0xffff, 0xffff, 0xffff }
};

const GdkColor
terminal_palette_xterm[TERMINAL_PALETTE_SIZE] =
{
    {0, 0x0000, 0x0000, 0x0000 },
    {0, 0xcdcb, 0x0000, 0x0000 },
    {0, 0x0000, 0xcdcb, 0x0000 },
    {0, 0xcdcb, 0xcdcb, 0x0000 },
    {0, 0x1e1a, 0x908f, 0xffff },
    {0, 0xcdcb, 0x0000, 0xcdcb },
    {0, 0x0000, 0xcdcb, 0xcdcb },
    {0, 0xe5e2, 0xe5e2, 0xe5e2 },
    {0, 0x4ccc, 0x4ccc, 0x4ccc },
    {0, 0xffff, 0x0000, 0x0000 },
    {0, 0x0000, 0xffff, 0x0000 },
    {0, 0xffff, 0xffff, 0x0000 },
    {0, 0x4645, 0x8281, 0xb4ae },
    {0, 0xffff, 0x0000, 0xffff },
    {0, 0x0000, 0xffff, 0xffff },
    {0, 0xffff, 0xffff, 0xffff }
};

const GdkColor
terminal_palette_rxvt[TERMINAL_PALETTE_SIZE] =
{
  { 0, 0x0000, 0x0000, 0x0000 },
  { 0, 0xcdcd, 0x0000, 0x0000 },
  { 0, 0x0000, 0xcdcd, 0x0000 },
  { 0, 0xcdcd, 0xcdcd, 0x0000 },
  { 0, 0x0000, 0x0000, 0xcdcd },
  { 0, 0xcdcd, 0x0000, 0xcdcd },
  { 0, 0x0000, 0xcdcd, 0xcdcd },
  { 0, 0xfafa, 0xebeb, 0xd7d7 },
  { 0, 0x4040, 0x4040, 0x4040 },
  { 0, 0xffff, 0x0000, 0x0000 },
  { 0, 0x0000, 0xffff, 0x0000 },
  { 0, 0xffff, 0xffff, 0x0000 },
  { 0, 0x0000, 0x0000, 0xffff },
  { 0, 0xffff, 0x0000, 0xffff },
  { 0, 0x0000, 0xffff, 0xffff },
  { 0, 0xffff, 0xffff, 0xffff }
};

typedef struct _TerminalPaletteScheme
{
  const char *name;
  const GdkColor *palette;
}TerminalPaletteScheme;

static TerminalPaletteScheme palette_schemes[] = {
    { N_("Tango"), terminal_palette_tango },
    { N_("Linux console"), terminal_palette_linux },
    { N_("XTerm"), terminal_palette_xterm },
    { N_("Rxvt"), terminal_palette_rxvt },
    { N_("Zenburn"), terminal_palette_zenburn }
};

static void init_palette_scheme_menu (void);
static void update_palette_color_button(gint idx);


/* For use in get_display_dimension() */
enum dimensions { HEIGHT, WIDTH };

/* This will hold the GtkBuilder representation of the .glade file.
 * We keep this global so that we can look up any element from any routine.
 *
 * Note that for GtkBuilder to autoconnect signals, the functions that it hooks
 * to must NOT be marked static. I decided against the autoconnect just to
 * keep things "in the code" so that they can be grepped for easily. */
static GtkBuilder *xml = NULL;

/* This is terrible. We're keeping a local copy of a variable that
 * should probably be project global, because we use it everywhere.
 *
 * I did this to avoid passing and typecasting it in every single
 * callback function. I will make the variable project-global later,
 * but that is another day. */
static tilda_window *tw = NULL;

/* Prototypes for use in the wizard() */
static void set_wizard_state_from_config ();
static void connect_wizard_signals ();

/* Show the wizard. This will show the wizard, then exit immediately. */
gint wizard (tilda_window *ltw)
{
    DEBUG_FUNCTION ("wizard");
    DEBUG_ASSERT (ltw != NULL);

    gchar *window_title;
    const gchar *glade_file = g_build_filename (DATADIR, "tilda.glade", NULL);
    GtkWidget *wizard_window;

    /* Make sure that there isn't already a wizard showing */
    if (xml) {
        DEBUG_ERROR ("wizard started while one already active");
        return 1;
    }

    GError* error = NULL;
    xml = gtk_builder_new ();

#if ENABLE_NLS
    gtk_builder_set_translation_domain (xml, PACKAGE);
#endif

    if (!gtk_builder_add_from_file (xml, glade_file, &error)) {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
        return EXIT_FAILURE;
    }

    if (!xml) {
        g_warning ("problem while loading the tilda.glade file");
        return 2;
    }

    wizard_window = GTK_WIDGET (
        gtk_builder_get_object (xml, "wizard_window")
    );

    /* We're up and running now, so now connect all of the signal handlers.
     * NOTE: I decided to do this manually, since it is safer that way. */
    //glade_xml_signal_autoconnect(xml);

    /* See the notes above, where the tw variable is declared.
     * I know how ugly this is ... */
    tw = ltw;

    init_palette_scheme_menu ();

    /* Copy the current program state into the wizard */
    set_wizard_state_from_config ();

    /* Connect all signal handlers. We do this after copying the state into
     * the wizard so that all of the handlers don't get called as we copy in
     * the values. */
    connect_wizard_signals ();

    /* Unbind the current keybinding. I'm aware that this opens up an opportunity to
     * let "someone else" grab the key, but it also saves us some trouble, and makes
     * validation easier. */
    tilda_keygrabber_unbind (config_getstr ("key"));

    /* Adding widget title for CSS selection */
    gtk_widget_set_name (GTK_WIDGET(wizard_window), "Wizard");

    window_title = g_strdup_printf (_("Tilda %d Config"), ltw->instance);
    gtk_window_set_title (GTK_WINDOW(wizard_window), window_title);
    gtk_window_set_keep_above (GTK_WINDOW(wizard_window), TRUE);

    gtk_widget_show_all (wizard_window);
    g_free (window_title);

    /* Disable auto hide */
    tw->disable_auto_hide = TRUE;

    return 0;
}

gboolean validate_pulldown_keybinding(const gchar* accel, const GtkWidget* wizard_window, const gchar* message)
{
    /* Try to grab the key. This is a good way to validate it :) */
    gboolean key_is_valid = tilda_keygrabber_bind (accel, tw);

    if (key_is_valid)
        return TRUE;
    else
    {
        /* Show the "invalid keybinding" dialog */
        show_invalid_keybinding_dialog (GTK_WINDOW(wizard_window), message);
        return FALSE;
    }
}

gboolean validate_keybinding(const gchar* accel, const GtkWidget* wizard_window, const gchar* message)
{
    guint accel_key;
    GdkModifierType accel_mods;

    /* Parse the accelerator string. If it parses improperly, both values will be 0.*/
    gtk_accelerator_parse (accel, &accel_key, &accel_mods);
    if (! ((accel_key == 0) && (accel_mods == 0)) ) {
        return TRUE;
    } else {
        show_invalid_keybinding_dialog (GTK_WINDOW(wizard_window), message);
        return FALSE;
    }
}

# define GET_BUTTON_LABEL(BUTTON) ( gtk_button_get_label( GTK_BUTTON( \
    gtk_builder_get_object (xml, BUTTON))))

/* Gets called just after the wizard is closed. This should clean up after
 * the wizard, and do anything that couldn't be done immediately during the
 * wizard's lifetime. */
static void wizard_closed ()
{
    DEBUG_FUNCTION ("wizard_closed");

    const gchar *key = GET_BUTTON_LABEL("button_keybinding_pulldown");
    const gchar *addtab_key = GET_BUTTON_LABEL("button_keybinding_addtab");
    const gchar *closetab_key = GET_BUTTON_LABEL("button_keybinding_closetab");
    const gchar *nexttab_key = GET_BUTTON_LABEL("button_keybinding_nexttab");
    const gchar *prevtab_key = GET_BUTTON_LABEL("button_keybinding_prevtab");
    const gchar *movetableft_key = GET_BUTTON_LABEL("button_keybinding_movetableft");
    const gchar *movetabright_key = GET_BUTTON_LABEL("button_keybinding_movetabright");
    const gchar *copy_key = GET_BUTTON_LABEL("button_keybinding_copy");
    const gchar *paste_key = GET_BUTTON_LABEL("button_keybinding_paste");
    const gchar *quit_key = GET_BUTTON_LABEL("button_keybinding_quit");
    const gchar *gototab_1_key = GET_BUTTON_LABEL("button_keybinding_gototab1");
    const gchar *gototab_2_key = GET_BUTTON_LABEL("button_keybinding_gototab2");
    const gchar *gototab_3_key = GET_BUTTON_LABEL("button_keybinding_gototab3");
    const gchar *gototab_4_key = GET_BUTTON_LABEL("button_keybinding_gototab4");
    const gchar *gototab_5_key = GET_BUTTON_LABEL("button_keybinding_gototab5");
    const gchar *gototab_6_key = GET_BUTTON_LABEL("button_keybinding_gototab6");
    const gchar *gototab_7_key = GET_BUTTON_LABEL("button_keybinding_gototab7");
    const gchar *gototab_8_key = GET_BUTTON_LABEL("button_keybinding_gototab8");
    const gchar *gototab_9_key = GET_BUTTON_LABEL("button_keybinding_gototab9");
    const gchar *gototab_10_key = GET_BUTTON_LABEL("button_keybinding_gototab10");

    const GtkWidget *entry_custom_command =
        GTK_WIDGET (gtk_builder_get_object(xml, "entry_custom_command"));
    const GtkWidget *wizard_window =
        GTK_WIDGET (gtk_builder_get_object (xml, "wizard_window"));
    const gchar *command = gtk_entry_get_text (GTK_ENTRY(entry_custom_command));

    /* Validate our new shortcuts */

     /* The pulldown key is validated differently (should it be?), so it gets its own function. */
    if (!validate_pulldown_keybinding(key, wizard_window, _("The keybinding you chose for \"Pull Down Terminal\" is invalid. Please choose another.")))
        return;

     /* Check the rest of them */
    if (!validate_keybinding(addtab_key, wizard_window, _("The keybinding you chose for \"Add Tab\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(closetab_key, wizard_window, _("The keybinding you chose for \"Close Tab\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(nexttab_key, wizard_window, _("The keybinding you chose for \"Next Tab\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(prevtab_key, wizard_window, _("The keybinding you chose for \"Previous Tab\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(movetableft_key, wizard_window, _("The keybinding you chose for \"Move Tab to Left\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(movetabright_key, wizard_window, _("The keybinding you chose for \"Move Tab to Right\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(copy_key, wizard_window, _("The keybinding you chose for \"Copy\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(paste_key, wizard_window, _("The keybinding you chose for \"Paste\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(quit_key, wizard_window, _("The keybinding you chose for \"Quit\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_1_key, wizard_window, _("The keybinding you chose for \"Go To Tab 1\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_2_key, wizard_window, _("The keybinding you chose for \"Go To Tab 2\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_3_key, wizard_window, _("The keybinding you chose for \"Go To Tab 3\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_4_key, wizard_window, _("The keybinding you chose for \"Go To Tab 4\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_5_key, wizard_window, _("The keybinding you chose for \"Go To Tab 5\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_6_key, wizard_window, _("The keybinding you chose for \"Go To Tab 6\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_7_key, wizard_window, _("The keybinding you chose for \"Go To Tab 7\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_8_key, wizard_window, _("The keybinding you chose for \"Go To Tab 8\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_9_key, wizard_window, _("The keybinding you chose for \"Go To Tab 9\" is invalid. Please choose another.")))
        return;
    if (!validate_keybinding(gototab_10_key, wizard_window, _("The keybinding you chose for \"Go To Tab 10\" is invalid. Please choose another.")))
        return;

    /* Now that our shortcuts are validated, store them back into the config. */
    config_setstr ("key", key);
    config_setstr ("addtab_key", addtab_key);
    config_setstr ("closetab_key", closetab_key);
    config_setstr ("nexttab_key", nexttab_key);
    config_setstr ("prevtab_key", prevtab_key);
    config_setstr ("movetableft_key", movetableft_key);
    config_setstr ("movetabright_key", movetabright_key);
    config_setstr ("copy_key", copy_key);
    config_setstr ("paste_key", paste_key);
    config_setstr ("quit_key", quit_key);
    config_setstr ("gototab_1_key",  gototab_1_key);
    config_setstr ("gototab_2_key",  gototab_2_key);
    config_setstr ("gototab_3_key",  gototab_3_key);
    config_setstr ("gototab_4_key",  gototab_4_key);
    config_setstr ("gototab_5_key",  gototab_5_key);
    config_setstr ("gototab_6_key",  gototab_6_key);
    config_setstr ("gototab_7_key",  gototab_7_key);
    config_setstr ("gototab_8_key",  gototab_8_key);
    config_setstr ("gototab_9_key",  gototab_9_key);
    config_setstr ("gototab_10_key", gototab_10_key);

    /* Now that they're in the config, reset the keybindings right now. */
    tilda_window_setup_keyboard_accelerators(tw);

    /* TODO: validate this?? */
    config_setstr ("command", command);

    /* Free the libglade data structure */
    g_object_unref (G_OBJECT(xml));
    xml = NULL;

    /* Remove the wizard */
    gtk_widget_destroy (GTK_WIDGET(wizard_window));

    /* Write the config, because it probably changed. This saves us in case
     * of an XKill (or crash) later ... */
    config_write (tw->config_file);

    /* Enables auto hide */
    tw->disable_auto_hide = FALSE;
}

void show_invalid_keybinding_dialog (GtkWindow *parent_window, const gchar* message)
{
    DEBUG_FUNCTION ("show_invalid_keybinding_dialog");

    GtkWidget *dialog = gtk_message_dialog_new (parent_window,
                              GTK_DIALOG_DESTROY_WITH_PARENT,
                              GTK_MESSAGE_ERROR,
                              GTK_BUTTONS_CLOSE,
                              "%s", message);

    gtk_window_set_keep_above (GTK_WINDOW(dialog), TRUE);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

/******************************************************************************/
/*               ALL static callback helpers are below                        */
/******************************************************************************/

/*
 * The functions wizard_dlg_key_grab() and setup_key_grab() are totally inspired by
 * the custom keybindings dialog in denemo. See http://denemo.sourceforge.net/
 *
 * Thanks for the help! :)
 */

/*
 * This function gets called once per key that is pressed. This means we have to
 * manually ignore all modifier keys, and only register "real" keys.
 *
 * We return when the first "real" key is pressed.
 *
 * Note that this is called for the key_press_event for the key-grab dialog, not for the wizard.
 */

static void wizard_dlg_key_grab (GtkWidget *dialog, GdkEventKey *event, GtkWidget* w, const GtkWidget* wizard_window)
{
    DEBUG_FUNCTION ("wizard_dlg_key_grab");
    DEBUG_ASSERT (wizard_window != NULL);
    DEBUG_ASSERT (event != NULL);

    gchar *key;

    if (gtk_accelerator_valid (event->keyval, event->state))
    {
        /* This lets us ignore all ignorable modifier keys, including
         * NumLock and many others. :)
         *
         * The logic is: keep only the important modifiers that were pressed
         * for this event. */
        event->state &= gtk_accelerator_get_default_mod_mask();

        /* Generate the correct name for this key */
        key = gtk_accelerator_name (event->keyval, event->state);

        #ifdef DEBUG
            g_printerr ("KEY GRABBED: %s\n", key);
        #endif

        /* Disconnect the key grabber */
        g_signal_handlers_disconnect_by_func (G_OBJECT(dialog), G_CALLBACK (wizard_dlg_key_grab), w);

        /* Destroy the dialog */
        gtk_widget_destroy (dialog);

        /* Copy the pressed key to the text entry */
        gtk_button_set_label (GTK_BUTTON(w), key);

        /* Free the string */
        g_free (key);
    }
}

static int percentage_dimension (int current_size, enum dimensions dimension)
{
    DEBUG_FUNCTION ("percentage_dimension");
    DEBUG_ASSERT (dimension == WIDTH || dimension == HEIGHT);

    if (dimension == HEIGHT)
        return (int) (((float) current_size) / ((float) gdk_screen_height ()) * 100.0);

    return (int) (((float) current_size) / ((float) gdk_screen_width ()) * 100.0);
}

#define percentage_height(current_height) percentage_dimension(current_height, HEIGHT)
#define percentage_width(current_width)   percentage_dimension(current_width, WIDTH)

#define pixels2percentage(PIXELS,DIMENSION) percentage_dimension ((PIXELS),(DIMENSION))
#define percentage2pixels(PERCENTAGE,DIMENSION) (((PERCENTAGE) / 100.0) * get_display_dimension ((DIMENSION)))

static gint get_display_dimension (const enum dimensions dimension)
{
    DEBUG_FUNCTION ("get_display_dimension");
    DEBUG_ASSERT (dimension == HEIGHT || dimension == WIDTH);

    if (dimension == HEIGHT)
        return gdk_screen_height ();

    if (dimension == WIDTH)
        return gdk_screen_width ();

    return -1;
}

static void window_title_change_all ()
{
    DEBUG_FUNCTION ("window_title_change_all");

    GtkWidget *page;
    GtkWidget *label;
    tilda_term *tt;
    gchar *title;
    gint i, size, list_count;

    size = gtk_notebook_get_n_pages (GTK_NOTEBOOK(tw->notebook));
    list_count = size-1;

    for (i=0;i<size;i++,list_count--)
    {
        tt = g_list_nth (tw->terms, list_count)->data;
        title = get_window_title (tt->vte_term);
        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook), i);
        label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (tw->notebook), page);
        gtk_label_set_label (GTK_LABEL(label), title);
        g_free (title);
    }
}

static void set_spin_value_while_blocking_callback (GtkSpinButton *spin, void (*callback)(GtkWidget *w), gint new_val)
{
    g_signal_handlers_block_by_func (spin, G_CALLBACK(*callback), NULL);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin), new_val);
    g_signal_handlers_unblock_by_func (spin, G_CALLBACK(*callback), NULL);
}

/******************************************************************************/
/*                       ALL Callbacks are below                              */
/******************************************************************************/

static void button_wizard_close_clicked_cb (GtkWidget *w)
{
    const GtkWidget *wizard_window =
        GTK_WIDGET (gtk_builder_get_object (xml, "wizard_window"));

    /* Call the clean-up function */
    wizard_closed ();
}

static void check_display_on_all_workspaces_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("pinned", status);

    if (status)
        gtk_window_stick (GTK_WINDOW (tw->window));
    else
        gtk_window_unstick (GTK_WINDOW (tw->window));
}

static void check_do_not_show_in_taskbar_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("notaskbar", status);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tw->window), status);
}

static void check_show_notebook_border_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("notebook_border", status);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook), status);
}

static void check_always_on_top_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("above", status);
    gtk_window_set_keep_above (GTK_WINDOW (tw->window), status);
}


static void check_start_tilda_hidden_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("hidden", status);
}

static void check_enable_double_buffering_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setbool ("double_buffer", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        gtk_widget_set_double_buffered (GTK_WIDGET(tt->vte_term), status);
    }
}

static void check_terminal_bell_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setbool ("bell", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_audible_bell (VTE_TERMINAL(tt->vte_term), status);
        vte_terminal_set_visible_bell (VTE_TERMINAL(tt->vte_term), !status);
    }
}

static void check_cursor_blinks_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setbool ("blinks", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_cursor_blink_mode (VTE_TERMINAL(tt->vte_term),
                (status)?VTE_CURSOR_BLINK_ON:VTE_CURSOR_BLINK_OFF);
    }
}

static void check_enable_antialiasing_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setbool ("antialias", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        PangoFontDescription *description =
            pango_font_description_from_string (config_getstr ("font"));
        vte_terminal_set_font (VTE_TERMINAL(tt->vte_term), description);
    }
}

static void spin_auto_hide_time_value_changed_cb (GtkWidget *w)
{
    const gint auto_hide_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));
    config_setint ("auto_hide_time", auto_hide_time);
    tw->auto_hide_max_time = auto_hide_time;
}

static void check_auto_hide_on_focus_lost_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("auto_hide_on_focus_lost", status);
    tw->auto_hide_on_focus_lost = status;
}

static void check_auto_hide_on_mouse_leave_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("auto_hide_on_mouse_leave", status);
    tw->auto_hide_on_mouse_leave = status;
}

static void check_allow_bold_text_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setbool ("bold", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_allow_bold (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void combo_tab_pos_changed_cb (GtkWidget *w)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    const gint positions[] = { GTK_POS_TOP,
                             GTK_POS_BOTTOM,
                             GTK_POS_LEFT,
                             GTK_POS_RIGHT };

    if (status < 0 || status > 4) {
        DEBUG_ERROR ("Notebook tab position invalid");
        g_printerr (_("Invalid tab position setting, ignoring\n"));
        return;
    }

    config_setint ("tab_pos", status);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK(tw->notebook), positions[status]);
}

static void button_font_font_set_cb (GtkWidget *w)
{
    const gchar *font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (w));
    const gboolean antialias = config_getbool ("antialias");
    gint i;
    tilda_term *tt;

    config_setstr ("font", font);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        PangoFontDescription *description =
            pango_font_description_from_string (font);
        vte_terminal_set_font (VTE_TERMINAL(tt->vte_term), description);
    }
}

static void entry_title_changed_cb (GtkWidget *w)
{
    const gchar *title = gtk_entry_get_text (GTK_ENTRY(w));

    config_setstr ("title", title);
    window_title_change_all ();
}

static void combo_dynamically_set_title_changed_cb (GtkWidget *w)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("d_set_title", status);
    window_title_change_all ();
}

static void check_run_custom_command_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    GtkWidget *label_custom_command;
    GtkWidget *entry_custom_command;

    config_setbool ("run_command", status);

    label_custom_command =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_custom_command"));
    entry_custom_command =
        GTK_WIDGET (gtk_builder_get_object (xml, "entry_custom_command"));

    gtk_widget_set_sensitive (GTK_WIDGET(label_custom_command), status);
    gtk_widget_set_sensitive (GTK_WIDGET(entry_custom_command), status);
}


/**
 * Check that the value entered into the GtkEntry is a valid command
 * which is accessible from the users PATH environment variable.
 * If the command is not found on the users PATH then a small error
 * icon is displayed on the end of the entry.
 * This function can only be registered to Widgets of (sub)type GtkEntry
 */
static void validate_executable_command_cb (GtkWidget *w, GdkEvent *event, gpointer userdata) {
    g_return_if_fail(w != NULL && GTK_IS_ENTRY(w));
    const char* command = gtk_entry_get_text (GTK_ENTRY(w));
    /* Check that the command exists */
    char* command_filename = g_find_program_in_path(command);
    if(command_filename == NULL) {
        //wrong command
        gtk_entry_set_icon_from_stock (GTK_ENTRY(w),
            GTK_ENTRY_ICON_SECONDARY,
            GTK_STOCK_DIALOG_ERROR);
        gtk_entry_set_icon_tooltip_text(GTK_ENTRY(w),
            GTK_ENTRY_ICON_SECONDARY,
            "The command you have entered is not a valid command.\n"
            "Make sure that the specified executable is in your PATH environment variable."
        );
    } else {
        gtk_entry_set_icon_from_stock (GTK_ENTRY(w),
            GTK_ENTRY_ICON_SECONDARY,
            NULL);
        free(command_filename);
    }
}

static void combo_command_exit_changed_cb (GtkWidget *w) {
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("command_exit", status);
}

static void combo_on_last_terminal_exit_changed_cb (GtkWidget *w)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("on_last_terminal_exit", status);
}

static void entry_web_browser_changed (GtkWidget *w) {
    const gchar *web_browser = gtk_entry_get_text (GTK_ENTRY(w));

    config_setstr ("web_browser", web_browser);
}

static void entry_word_chars_changed (GtkWidget *w)
{
    gint i;
    tilda_term *tt;
    const gchar *word_chars = gtk_entry_get_text (GTK_ENTRY(w));

    /* restore to default value if user clears this setting */
    if (NULL == word_chars || '\0' == word_chars[0])
        word_chars = DEFAULT_WORD_CHARS;

    config_setstr ("word_chars", word_chars);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_word_chars (VTE_TERMINAL(tt->vte_term), word_chars);
    }
}

/*
 * Prototypes for the next 4 functions. Since they depend on each other,
 * this is pretty much necessary.
 *
 * Sorry about the mess!
 */
static void spin_height_percentage_value_changed_cb (GtkWidget *w);
static void spin_height_pixels_value_changed_cb (GtkWidget *w);
static void spin_width_percentage_value_changed_cb (GtkWidget *w);
static void spin_width_pixels_value_changed_cb (GtkWidget *w);

static void spin_height_percentage_value_changed_cb (GtkWidget *w)
{
    const GtkWidget *spin_height_pixels =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_height_pixels"));

    const gint h_pct = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint h_pix = percentage2pixels (h_pct, HEIGHT);

    config_setint ("max_height", h_pix);
    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_height_pixels), &spin_height_pixels_value_changed_cb, h_pix);
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    if (config_getbool ("centered_vertically")) {
        config_setint ("y_pos", find_centering_coordinate (gdk_screen_height(), config_getint ("max_height")));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_height_pixels_value_changed_cb (GtkWidget *w)
{
    const GtkWidget *spin_height_percentage =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_height_percentage"));
    const gint h_pix = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint h_pct = pixels2percentage (h_pix, HEIGHT);

    config_setint ("max_height", h_pix);
    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_height_percentage), &spin_height_percentage_value_changed_cb, h_pct);
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    if (config_getbool ("centered_vertically")) {
        config_setint ("y_pos", find_centering_coordinate (gdk_screen_height(), config_getint ("max_height")));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_width_percentage_value_changed_cb (GtkWidget *w)
{
    const GtkWidget *spin_width_pixels =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_width_pixels"));

    const gint w_pct = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint w_pix = percentage2pixels (w_pct, WIDTH);

    config_setint ("max_width", w_pix);
    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_width_pixels), &spin_width_pixels_value_changed_cb, w_pix);
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    if (config_getbool ("centered_horizontally")) {
        config_setint ("x_pos", find_centering_coordinate (gdk_screen_width(), config_getint ("max_width")));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_width_pixels_value_changed_cb (GtkWidget *w)
{
    const GtkWidget *spin_width_percentage =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_width_percentage"));

    const gint w_pix = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint w_pct = pixels2percentage (w_pix, WIDTH);

    config_setint ("max_width", w_pix);
    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_width_percentage), &spin_width_percentage_value_changed_cb, w_pct);
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    if (config_getbool ("centered_horizontally")) {
        config_setint ("x_pos", find_centering_coordinate (gdk_screen_width(), config_getint ("max_width")));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void check_centered_horizontally_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_x_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_x_position"));
    const GtkWidget *spin_x_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_x_position"));

    config_setbool ("centered_horizontally", status);

    if (status)
        config_setint ("x_pos", find_centering_coordinate (gdk_screen_width(), config_getint ("max_width")));
    else
        config_setint ("x_pos", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_x_position)));

    gtk_widget_set_sensitive (GTK_WIDGET(label_x_position), !status);
    gtk_widget_set_sensitive (GTK_WIDGET(spin_x_position), !status);

    gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_x_position_value_changed_cb (GtkWidget *w)
{
    const gint x_pos = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint y_pos = config_getint ("y_pos");

    config_setint ("x_pos", x_pos);
    gtk_window_move (GTK_WINDOW(tw->window), x_pos, y_pos);

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void check_centered_vertically_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_y_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_y_position"));
    const GtkWidget *spin_y_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_y_position"));

    config_setbool ("centered_vertically", status);

    if (status)
        config_setint ("y_pos", find_centering_coordinate (gdk_screen_height(), config_getint ("max_height")));
    else
        config_setint ("y_pos", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_y_position)));

    gtk_widget_set_sensitive (GTK_WIDGET(label_y_position), !status);
    gtk_widget_set_sensitive (GTK_WIDGET(spin_y_position), !status);

    gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_y_position_value_changed_cb (GtkWidget *w)
{
    const gint x_pos = config_getint ("x_pos");
    const gint y_pos = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));

    config_setint ("y_pos", y_pos);
    gtk_window_move (GTK_WINDOW(tw->window), x_pos, y_pos);

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void check_enable_transparency_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_level_of_transparency =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_level_of_transparency"));
    const GtkWidget *spin_level_of_transparency =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_level_of_transparency"));

    const gdouble transparency_level = (gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_level_of_transparency)) / 100.0);
    gint i;
    tilda_term *tt;

    config_setbool ("enable_transparency", status);

    gtk_widget_set_sensitive (GTK_WIDGET(label_level_of_transparency), status);
    gtk_widget_set_sensitive (GTK_WIDGET(spin_level_of_transparency), status);

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
}

static void spin_level_of_transparency_value_changed_cb (GtkWidget *w)
{
    const gint status = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gdouble transparency_level = (status / 100.0);
    gint i;
    tilda_term *tt;

    config_setint ("transparency", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_background_saturation (VTE_TERMINAL(tt->vte_term), transparency_level);
        vte_terminal_set_opacity (VTE_TERMINAL(tt->vte_term), (1.0 - transparency_level) * 0xffff);
        vte_terminal_set_background_transparent(VTE_TERMINAL(tt->vte_term), !tw->have_argb_visual);
    }
}

static void spin_animation_delay_value_changed_cb (GtkWidget *w)
{
    const gint status = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));

    config_setint ("slide_sleep_usec", status);
}

static void combo_animation_orientation_changed_cb (GtkWidget *w)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("animation_orientation", status);
    generate_animation_positions (tw);
}

static void check_animated_pulldown_toggled_cb (GtkWidget *w)
{
    const gint status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_animation_delay =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_animation_delay"));
    const GtkWidget *spin_animation_delay =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_animation_delay"));
    const GtkWidget *label_animation_orientation =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_animation_orientation"));
    const GtkWidget *combo_animation_orientation =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_animation_orientation"));

    gtk_widget_set_sensitive (GTK_WIDGET(label_animation_delay), status);
    gtk_widget_set_sensitive (GTK_WIDGET(spin_animation_delay), status);
    gtk_widget_set_sensitive (GTK_WIDGET(label_animation_orientation), status);
    gtk_widget_set_sensitive (GTK_WIDGET(combo_animation_orientation), status);

    config_setbool ("animation", status);

    /* If we just disabled animation, we have to reset the window size to the normal
     * size, since the animations change the size of the window, and pull() does nothing more
     * than show and place the window. */
    if (!status)
    {
        gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Avoids a nasty looking glitch if you switch on animation while the window is
     * hidden. It will briefly show at full size, then shrink to the first animation
     * position. From there it works fine. */
    if (status && tw->current_state == UP)
    {
        /* I don't know why, but width=0, height=0 doesn't work. Width=1, height=1 works
         * exactly as expected, so I'm leaving it that way. */
        gtk_window_resize (GTK_WINDOW(tw->window), 1, 1);
    }
}

static void check_use_image_for_background_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *button_background_image =
        GTK_WIDGET (gtk_builder_get_object (xml, "button_background_image"));

    const gchar *image = config_getstr ("image");
    gint i;
    tilda_term *tt;


    config_setbool ("use_image", status);

    gtk_widget_set_sensitive (GTK_WIDGET(button_background_image), status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);

        if (status)
            vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), image);
        else
            vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), NULL);
    }
}

static void button_background_image_selection_changed_cb (GtkWidget *w)
{
    const gchar *image = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(w));
    gint i;
    tilda_term *tt;

    config_setstr ("image", image);

    if (config_getbool ("use_image"))
    {
        for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), image);
        }
    }
}

static void combo_colorschemes_changed_cb (GtkWidget *w)
{
    const gint scheme = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    const GtkWidget *colorbutton_text =
        GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_text"));
    const GtkWidget *colorbutton_back =
        GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_back"));
    GdkColor gdk_text, gdk_back;
    tilda_term *tt;
    gint i;
    gboolean nochange = FALSE;

    config_setint ("scheme", scheme);

    switch (scheme)
    {
        case 1:
            gdk_text.red = gdk_text.blue = 0x0000;
            gdk_text.green = 0xffff;
            gdk_back.red = gdk_back.green = gdk_back.blue = 0x0000;
            break;
        case 2:
            gdk_text.red = gdk_text.green = gdk_text.blue = 0x0000;
            gdk_back.red = gdk_back.green = gdk_back.blue = 0xffff;
            break;
        case 3:
            gdk_text.red = gdk_text.green = gdk_text.blue = 0xffff;
            gdk_back.red = gdk_back.green = gdk_back.blue = 0x0000;
            break;
        default:
            nochange = TRUE;
            break;
    }

    /* If we switched to "Custom", then don't do anything. Let the user continue
     * from the current color choice. */
    if (!nochange) {
        config_setint ("back_red", gdk_back.red);
        config_setint ("back_green", gdk_back.green);
        config_setint ("back_blue", gdk_back.blue);
        config_setint ("text_red", gdk_text.red);
        config_setint ("text_green", gdk_text.green);
        config_setint ("text_blue", gdk_text.blue);

        gtk_color_button_set_color (GTK_COLOR_BUTTON(colorbutton_text), &gdk_text);
        gtk_color_button_set_color (GTK_COLOR_BUTTON(colorbutton_back), &gdk_back);

        for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_color_foreground (VTE_TERMINAL(tt->vte_term), &gdk_text);
            vte_terminal_set_color_background (VTE_TERMINAL(tt->vte_term), &gdk_back);
        }
    }
}

static void colorbutton_text_color_set_cb (GtkWidget *w)
{
    const GtkWidget *combo_colorschemes =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_colorschemes"));

    gint i;
    tilda_term *tt;
    GdkColor gdk_text_color;

    /* The user just changed colors manually, so set the scheme to "Custom" */
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_colorschemes), 0);
    config_setint ("scheme", 0);

    /* Now get the color that was set, save it, then set it */
    gtk_color_button_get_color (GTK_COLOR_BUTTON(w), &gdk_text_color);
    config_setint ("text_red", gdk_text_color.red);
    config_setint ("text_green", gdk_text_color.green);
    config_setint ("text_blue", gdk_text_color.blue);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_color_foreground (VTE_TERMINAL(tt->vte_term), &gdk_text_color);
    }
}

static void colorbutton_back_color_set_cb (GtkWidget *w)
{
    const GtkWidget *combo_colorschemes =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_colorschemes"));
    gint i;
    tilda_term *tt;
    GdkColor gdk_back_color;

    /* The user just changed colors manually, so set the scheme to "Custom" */
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_colorschemes), 0);
    config_setint ("scheme", 0);

    /* Now get the color that was set, save it, then set it */
    gtk_color_button_get_color (GTK_COLOR_BUTTON(w), &gdk_back_color);
    config_setint ("back_red", gdk_back_color.red);
    config_setint ("back_green", gdk_back_color.green);
    config_setint ("back_blue", gdk_back_color.blue);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_color_background (VTE_TERMINAL(tt->vte_term), &gdk_back_color);
    }
}



static void combo_palette_scheme_changed_cb (GtkWidget *w)
{
    gint i, j;
    tilda_term *tt;
    GdkColor fg, bg;
    GtkWidget *color_button;

    i = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    if (i < G_N_ELEMENTS (palette_schemes))
    {
        color_button =
            GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_text"));
        gtk_color_button_get_color (GTK_COLOR_BUTTON(color_button), &fg);
        color_button =
            GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_back"));
        gtk_color_button_get_color (GTK_COLOR_BUTTON(color_button), &bg);

        memcpy(current_palette, palette_schemes[i].palette, sizeof(current_palette));

        /* Set terminal palette. */
        for (j=0; j<g_list_length (tw->terms); j++)
        {
            tt = g_list_nth_data (tw->terms, j);
            vte_terminal_set_colors (VTE_TERMINAL(tt->vte_term), &fg, &bg, current_palette, TERMINAL_PALETTE_SIZE);
        }

        for (j=0; j<TERMINAL_PALETTE_SIZE; j++)
        {
            update_palette_color_button(j);

            /* Set palette in the config. */
            config_setnint ("palette", current_palette[j].red,   j*3);
            config_setnint ("palette", current_palette[j].green, j*3+1);
            config_setnint ("palette", current_palette[j].blue,  j*3+2);
        }
    }
    else
    {
        /* "Custom" selected, do nothing. */
    }

    /* Set palette scheme in the config*/
    config_setint ("palette_scheme", i);
}

static void colorbutton_palette_n_set_cb (GtkWidget *w)
{
    const GtkWidget *combo_palette_scheme =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_palette_scheme"));
    const gchar* name = gtk_widget_get_name(w);
    gint i;
    tilda_term *tt;
    GtkWidget *color_button;
    GdkColor fg, bg;

    /* The user just changed palette manually, so set the palette scheme to "Custom" */
    i = G_N_ELEMENTS (palette_schemes);
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_palette_scheme), i);
    config_setint ("palette_scheme", i);

    /* Get the index number of color button which was set */
    i = atoi(&name[sizeof("colorbutton_palette_")-1]);

    /* Now get the color that was set, save it. */
    gtk_color_button_get_color (GTK_COLOR_BUTTON(w), &current_palette[i]);

    /* Why saving the whole palette, not the single color that was set,
     * is if the config file doesn't exist, and we save the single color,
     * then exit, the color always becomes the first color in the config file,
     * no matter which one it actually is, and leaves other colors unset.
     * Obviously this is not what we want.
     * However, maybe there is a better solution for this issue.
     */
    for (i=0; i<TERMINAL_PALETTE_SIZE; i++)
    {
        config_setnint ("palette", current_palette[i].red,   i*3);
        config_setnint ("palette", current_palette[i].green, i*3+1);
        config_setnint ("palette", current_palette[i].blue,  i*3+2);
    }

    /* Set terminal palette. */
    color_button =
        GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_text"));
    gtk_color_button_get_color (GTK_COLOR_BUTTON(color_button), &fg);
    color_button =
        GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_back"));
    gtk_color_button_get_color (GTK_COLOR_BUTTON(color_button), &bg);

    for (i=0; i<g_list_length (tw->terms); i++)
    {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_colors (VTE_TERMINAL(tt->vte_term), &fg, &bg, current_palette, TERMINAL_PALETTE_SIZE);
    }
}

static void combo_scrollbar_position_changed_cb (GtkWidget *w)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    gint i;
    tilda_term *tt;

    config_setint ("scrollbar_pos", status);

    for (i=0; i<g_list_length (tw->terms); i++)
    {
        tt = g_list_nth_data (tw->terms, i);
        tilda_term_set_scrollbar_position (tt, status);
    }
}

static void spin_scrollback_amount_value_changed_cb (GtkWidget *w)
{
    const gint status = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setint ("lines", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scrollback_lines (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void check_scroll_on_output_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setbool ("scroll_on_output", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scroll_on_output (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void check_scroll_on_keystroke_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setbool ("scroll_on_key", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void check_scroll_background_toggled_cb (GtkWidget *w)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    gint i;
    tilda_term *tt;

    config_setbool ("scroll_background", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scroll_background (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void combo_backspace_binding_changed_cb (GtkWidget *w)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    const gint keys[] = { VTE_ERASE_ASCII_DELETE,
                          VTE_ERASE_DELETE_SEQUENCE,
                          VTE_ERASE_ASCII_BACKSPACE,
                          VTE_ERASE_AUTO };
    gint i;
    tilda_term *tt;

    config_setint ("backspace_key", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), keys[status]);
    }
}

static void combo_delete_binding_changed_cb (GtkWidget *w)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    const gint keys[] = { VTE_ERASE_ASCII_DELETE,
                          VTE_ERASE_DELETE_SEQUENCE,
                          VTE_ERASE_ASCII_BACKSPACE,
                          VTE_ERASE_AUTO };
    gint i;
    tilda_term *tt;

    config_setint ("delete_key", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), keys[status]);
    }
}

static void button_reset_compatibility_options_clicked_cb (GtkWidget *w)
{
    const GtkWidget *combo_backspace_binding =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_backspace_binding"));
    const GtkWidget *combo_delete_binding =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_delete_binding"));

    config_setint ("backspace_key", 0);
    config_setint ("delete_key", 1);

    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_backspace_binding), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_delete_binding), 1);
}

static void button_keybinding_clicked_cb (GtkWidget *w)
{
    const GtkWidget *wizard_notebook =
        GTK_WIDGET (gtk_builder_get_object (xml, "wizard_notebook"));
    const GtkWidget *wizard_window =
        GTK_WIDGET (gtk_builder_get_object (xml, "wizard_window"));

    /* Get all my keybinding buttons. */
    const GtkWidget *button_keybinding_pulldown =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_pulldown"));
    const GtkWidget *button_keybinding_addtab =       GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_addtab"));
    const GtkWidget *button_keybinding_closetab =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_closetab"));
    const GtkWidget *button_keybinding_nexttab =      GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_nexttab"));
    const GtkWidget *button_keybinding_prevtab =      GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_prevtab"));
    const GtkWidget *button_keybinding_movetableft =  GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_movetableft"));
    const GtkWidget *button_keybinding_movetabright = GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_movetabright"));
    const GtkWidget *button_keybinding_copy =         GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_copy"));
    const GtkWidget *button_keybinding_paste =        GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_paste"));
    const GtkWidget *button_keybinding_quit =         GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_quit"));
    const GtkWidget *button_keybinding_gototab1 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab1"));
    const GtkWidget *button_keybinding_gototab2 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab2"));
    const GtkWidget *button_keybinding_gototab3 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab3"));
    const GtkWidget *button_keybinding_gototab4 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab4"));
    const GtkWidget *button_keybinding_gototab5 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab5"));
    const GtkWidget *button_keybinding_gototab6 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab6"));
    const GtkWidget *button_keybinding_gototab7 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab7"));
    const GtkWidget *button_keybinding_gototab8 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab8"));
    const GtkWidget *button_keybinding_gototab9 =     GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab9"));
    const GtkWidget *button_keybinding_gototab10 =    GTK_WIDGET (gtk_builder_get_object (xml, "button_keybinding_gototab10"));

    /* Make the preferences window and buttons non-sensitive while we are grabbing keys. */
    gtk_widget_set_sensitive (GTK_WIDGET(wizard_notebook), FALSE);

    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_pulldown), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_addtab), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_closetab), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_nexttab), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_prevtab), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_movetableft), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_movetabright), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_copy), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_paste), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_quit), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab1), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab2), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab3), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab4), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab5), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab6), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab7), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab8), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab9), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab10), FALSE);

    /* Bring up the dialog that will accept the new keybinding */
    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(wizard_window),
                              GTK_DIALOG_DESTROY_WITH_PARENT,
                              GTK_MESSAGE_QUESTION,
                              GTK_BUTTONS_CANCEL,
                              "Enter keyboard shortcut");

    /* Connect the key grabber to the dialog */
    g_signal_connect (G_OBJECT(dialog), "key_press_event", G_CALLBACK(wizard_dlg_key_grab), w);

    gtk_window_set_keep_above (GTK_WINDOW(dialog), TRUE);
    gint response = gtk_dialog_run (GTK_DIALOG (dialog));


    /* Re-enable widgets. Doing it here instead of wizard_dlg_key_grab because it is possible to close
       the dialog without grabbing a key, by clicking. */
    gtk_widget_set_sensitive (GTK_WIDGET(wizard_notebook), TRUE);

    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_pulldown), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_pulldown), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_addtab), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_closetab), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_nexttab), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_prevtab), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_movetableft), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_movetabright), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_copy), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_paste), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_quit), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab1), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab2), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab3), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab4), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab5), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab6), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab7), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab8), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab9), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET(button_keybinding_gototab10), TRUE);

    /* If the dialog was "programmatically destroyed" (we got a key), we don't want to destroy it again.
       Otherwise, we do want to destroy it, otherwise it would stick around even after hitting Cancel. */
    if (response != -1)
    {
        g_signal_handlers_disconnect_by_func (G_OBJECT(dialog), G_CALLBACK(wizard_dlg_key_grab), w);
        gtk_widget_destroy (dialog);
    }

}

/******************************************************************************/
/*                       Wizard set-up functions                              */
/******************************************************************************/


/* Defines to make the process of setting all of the initial values easier */
#define WIDGET_SET_INSENSITIVE(GLADE_NAME) gtk_widget_set_sensitive ( \
    GTK_WIDGET (gtk_builder_get_object (xml, (GLADE_NAME)), FALSE))
#define CHECK_BUTTON(GLADE_NAME,CFG_BOOL) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getbool ((CFG_BOOL)))
#define COMBO_BOX(GLADE_NAME,CFG_INT) gtk_combo_box_set_active (GTK_COMBO_BOX( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getint ((CFG_INT)))
#define FONT_BUTTON(GLADE_NAME,CFG_STR) gtk_font_button_set_font_name (GTK_FONT_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define TEXT_ENTRY(GLADE_NAME,CFG_STR) gtk_entry_set_text (GTK_ENTRY( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define BUTTON_LABEL_FROM_CFG(GLADE_NAME,CFG_STR) gtk_button_set_label (GTK_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define SPIN_BUTTON(GLADE_NAME,CFG_INT) gtk_spin_button_set_value (GTK_SPIN_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getint ((CFG_INT)))
#define SPIN_BUTTON_SET_RANGE(GLADE_NAME,LOWER,UPPER) gtk_spin_button_set_range (GTK_SPIN_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), (LOWER), (UPPER))
#define SPIN_BUTTON_SET_VALUE(GLADE_NAME,VALUE) gtk_spin_button_set_value (GTK_SPIN_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), (VALUE))
#define FILE_BUTTON(GLADE_NAME, FILENAME) gtk_file_chooser_set_filename (GTK_FILE_CHOOSER( \
    gtk_builder_get_object (xml, (GLADE_NAME))), FILENAME)
#define COLOR_BUTTON(GLADE_NAME,COLOR) gtk_color_button_set_color (GTK_COLOR_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), (COLOR))
#define SET_SENSITIVE_BY_CONFIG_BOOL(GLADE_NAME,CFG_BOOL) gtk_widget_set_sensitive ( \
    GTK_WIDGET (gtk_builder_get_object (xml, (GLADE_NAME))), config_getbool ((CFG_BOOL)))
#define SET_SENSITIVE_BY_CONFIG_NBOOL(GLADE_NAME,CFG_BOOL) gtk_widget_set_sensitive ( \
    GTK_WIDGET (gtk_builder_get_object (xml, (GLADE_NAME))), !config_getbool ((CFG_BOOL)))

/* Read all state from the config system, and put it into
 * its visual representation in the wizard. */
static void set_wizard_state_from_config ()
{
    GdkColor text_color, back_color;
    gint i;

    /* General Tab */
    CHECK_BUTTON ("check_display_on_all_workspaces", "pinned");
    CHECK_BUTTON ("check_always_on_top", "above");
    CHECK_BUTTON ("check_do_not_show_in_taskbar", "notaskbar");
    CHECK_BUTTON ("check_start_tilda_hidden", "hidden");
    CHECK_BUTTON ("check_show_notebook_border", "notebook_border");
    CHECK_BUTTON ("check_enable_double_buffering", "double_buffer");

    CHECK_BUTTON ("check_terminal_bell", "bell");
    CHECK_BUTTON ("check_cursor_blinks", "blinks");

    CHECK_BUTTON ("check_enable_antialiasing", "antialias");
    CHECK_BUTTON ("check_allow_bold_text", "bold");
    COMBO_BOX ("combo_tab_pos", "tab_pos");
    FONT_BUTTON ("button_font", "font");

    SPIN_BUTTON_SET_RANGE ("spin_auto_hide_time", 0, 99999);
    SPIN_BUTTON_SET_VALUE ("spin_auto_hide_time", config_getint ("auto_hide_time"));
    CHECK_BUTTON ("check_auto_hide_on_focus_lost", "auto_hide_on_focus_lost");
    CHECK_BUTTON ("check_auto_hide_on_mouse_leave", "auto_hide_on_mouse_leave");

    /* Title and Command Tab */
    TEXT_ENTRY ("entry_title", "title");
    COMBO_BOX ("combo_dynamically_set_title", "d_set_title");

    CHECK_BUTTON ("check_run_custom_command", "run_command");
    TEXT_ENTRY ("entry_custom_command", "command");
    COMBO_BOX ("combo_command_exit", "command_exit");
    COMBO_BOX ("combo_on_last_terminal_exit", "on_last_terminal_exit");
    SET_SENSITIVE_BY_CONFIG_BOOL ("entry_custom_command","run_command");
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_custom_command", "run_command");

    TEXT_ENTRY ("entry_web_browser", "web_browser");
    TEXT_ENTRY ("entry_word_chars", "word_chars");

    /* Appearance Tab */
    SPIN_BUTTON_SET_RANGE ("spin_height_percentage", 0, 100);
    SPIN_BUTTON_SET_VALUE ("spin_height_percentage", percentage_height (config_getint ("max_height")));
    SPIN_BUTTON_SET_RANGE ("spin_height_pixels", 0, gdk_screen_height());
    SPIN_BUTTON ("spin_height_pixels", "max_height");

    SPIN_BUTTON_SET_RANGE ("spin_width_percentage", 0, 100);
    SPIN_BUTTON_SET_VALUE ("spin_width_percentage", percentage_width (config_getint ("max_width")));
    SPIN_BUTTON_SET_RANGE ("spin_width_pixels", 0, gdk_screen_width());
    SPIN_BUTTON ("spin_width_pixels", "max_width");

    CHECK_BUTTON ("check_centered_horizontally", "centered_horizontally");
    CHECK_BUTTON ("check_centered_vertically", "centered_vertically");
    SPIN_BUTTON_SET_RANGE ("spin_x_position", 0, gdk_screen_width());
    SPIN_BUTTON_SET_RANGE ("spin_y_position", 0, gdk_screen_height());
    SPIN_BUTTON ("spin_x_position", "x_pos");
    SPIN_BUTTON ("spin_y_position", "y_pos");
    SET_SENSITIVE_BY_CONFIG_NBOOL ("spin_x_position","centered_horizontally");
    SET_SENSITIVE_BY_CONFIG_NBOOL ("label_x_position","centered_horizontally");
    SET_SENSITIVE_BY_CONFIG_NBOOL ("spin_y_position","centered_vertically");
    SET_SENSITIVE_BY_CONFIG_NBOOL ("label_y_position","centered_vertically");

    CHECK_BUTTON ("check_enable_transparency", "enable_transparency");
    SPIN_BUTTON ("spin_level_of_transparency", "transparency");
    CHECK_BUTTON ("check_animated_pulldown", "animation");
    SPIN_BUTTON ("spin_animation_delay", "slide_sleep_usec");
    COMBO_BOX ("combo_animation_orientation", "animation_orientation");
    CHECK_BUTTON ("check_use_image_for_background", "use_image");

    char* filename = config_getstr ("image");
    if(filename != NULL) {
        FILE_BUTTON ("button_background_image", filename);
    }
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_level_of_transparency","enable_transparency");
    SET_SENSITIVE_BY_CONFIG_BOOL ("spin_level_of_transparency","enable_transparency");
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_animation_delay","animation");
    SET_SENSITIVE_BY_CONFIG_BOOL ("spin_animation_delay","animation");
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_animation_orientation","animation");
    SET_SENSITIVE_BY_CONFIG_BOOL ("combo_animation_orientation","animation");
    SET_SENSITIVE_BY_CONFIG_BOOL ("button_background_image","use_image");

    /* Colors Tab */
    COMBO_BOX ("combo_colorschemes", "scheme");
    text_color.red = config_getint ("text_red");
    text_color.green = config_getint ("text_green");
    text_color.blue = config_getint ("text_blue");
    COLOR_BUTTON ("colorbutton_text", &text_color);
    back_color.red = config_getint ("back_red");
    back_color.green = config_getint ("back_green");
    back_color.blue = config_getint ("back_blue");
    COLOR_BUTTON ("colorbutton_back", &back_color);

    COMBO_BOX ("combo_palette_scheme", "palette_scheme");

    for(i = 0;i < TERMINAL_PALETTE_SIZE; i++)
    {
        current_palette[i].pixel = 0;
        current_palette[i].red   = config_getnint ("palette", i*3);
        current_palette[i].green = config_getnint ("palette", i*3+1);
        current_palette[i].blue  = config_getnint ("palette", i*3+2);

        update_palette_color_button(i);
    }

    /* Scrolling Tab */
    COMBO_BOX ("combo_scrollbar_position", "scrollbar_pos");
    SPIN_BUTTON ("spin_scrollback_amount", "lines");
    CHECK_BUTTON ("check_scroll_on_output", "scroll_on_output");
    CHECK_BUTTON ("check_scroll_on_keystroke", "scroll_on_key");
    CHECK_BUTTON ("check_scroll_background", "scroll_background");

    /* Compatibility Tab */
    COMBO_BOX ("combo_backspace_binding", "backspace_key");
    COMBO_BOX ("combo_delete_binding", "delete_key");

    /* Keybinding Tab */
    BUTTON_LABEL_FROM_CFG ("button_keybinding_pulldown", "key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_addtab", "addtab_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_closetab", "closetab_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_nexttab", "nexttab_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_prevtab", "prevtab_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_movetableft", "movetableft_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_movetabright", "movetabright_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_copy", "copy_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_paste", "paste_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_quit", "quit_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab1", "gototab_1_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab2", "gototab_2_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab3", "gototab_3_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab4", "gototab_4_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab5", "gototab_5_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab6", "gototab_6_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab7", "gototab_7_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab8", "gototab_8_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab9", "gototab_9_key");
    BUTTON_LABEL_FROM_CFG ("button_keybinding_gototab10", "gototab_10_key");
}

#define CONNECT_SIGNAL(GLADE_WIDGET,SIGNAL_NAME,SIGNAL_HANDLER) g_signal_connect ( \
    gtk_builder_get_object (xml, (GLADE_WIDGET)), (SIGNAL_NAME), G_CALLBACK((SIGNAL_HANDLER)), NULL)

/* Connect all signals in the wizard. This should be done after setting all
 * values, that way all of the signal handlers don't get called */
static void connect_wizard_signals ()
{
    gint i;

    /* General Tab */
    CONNECT_SIGNAL ("check_display_on_all_workspaces","toggled",check_display_on_all_workspaces_toggled_cb);
    CONNECT_SIGNAL ("check_do_not_show_in_taskbar","toggled",check_do_not_show_in_taskbar_toggled_cb);
    CONNECT_SIGNAL ("check_show_notebook_border","toggled",check_show_notebook_border_toggled_cb);
    CONNECT_SIGNAL ("check_always_on_top","toggled",check_always_on_top_toggled_cb);
    CONNECT_SIGNAL ("check_start_tilda_hidden","toggled",check_start_tilda_hidden_toggled_cb);
    CONNECT_SIGNAL ("check_enable_double_buffering","toggled",check_enable_double_buffering_toggled_cb);

    CONNECT_SIGNAL ("check_terminal_bell","toggled",check_terminal_bell_toggled_cb);
    CONNECT_SIGNAL ("check_cursor_blinks","toggled",check_cursor_blinks_toggled_cb);

    CONNECT_SIGNAL ("check_enable_antialiasing","toggled",check_enable_antialiasing_toggled_cb);
    CONNECT_SIGNAL ("check_allow_bold_text","toggled",check_allow_bold_text_toggled_cb);
    CONNECT_SIGNAL ("combo_tab_pos","changed",combo_tab_pos_changed_cb);
    CONNECT_SIGNAL ("button_font","font-set",button_font_font_set_cb);

    CONNECT_SIGNAL ("spin_auto_hide_time","value-changed",spin_auto_hide_time_value_changed_cb);
    CONNECT_SIGNAL ("check_auto_hide_on_focus_lost","toggled",check_auto_hide_on_focus_lost_toggled_cb);
    CONNECT_SIGNAL ("check_auto_hide_on_mouse_leave","toggled",check_auto_hide_on_mouse_leave_toggled_cb);

    CONNECT_SIGNAL ("combo_on_last_terminal_exit","changed",combo_on_last_terminal_exit_changed_cb);

    /* Title and Command Tab */
    CONNECT_SIGNAL ("entry_title","changed",entry_title_changed_cb);
    CONNECT_SIGNAL ("combo_dynamically_set_title","changed",combo_dynamically_set_title_changed_cb);

    CONNECT_SIGNAL ("check_run_custom_command","toggled",check_run_custom_command_toggled_cb);
    CONNECT_SIGNAL ("combo_command_exit","changed",combo_command_exit_changed_cb);

    CONNECT_SIGNAL ("entry_web_browser","changed",entry_web_browser_changed);
    CONNECT_SIGNAL ("entry_web_browser","focus-out-event", validate_executable_command_cb);
    CONNECT_SIGNAL ("entry_word_chars","changed",entry_word_chars_changed);

    /* Appearance Tab */
    CONNECT_SIGNAL ("spin_height_percentage","value-changed",spin_height_percentage_value_changed_cb);
    CONNECT_SIGNAL ("spin_height_pixels","value-changed",spin_height_pixels_value_changed_cb);
    CONNECT_SIGNAL ("spin_width_percentage","value-changed",spin_width_percentage_value_changed_cb);
    CONNECT_SIGNAL ("spin_width_pixels","value-changed",spin_width_pixels_value_changed_cb);

    CONNECT_SIGNAL ("check_centered_horizontally","toggled",check_centered_horizontally_toggled_cb);
    CONNECT_SIGNAL ("check_centered_vertically","toggled",check_centered_vertically_toggled_cb);
    CONNECT_SIGNAL ("spin_x_position","value-changed",spin_x_position_value_changed_cb);
    CONNECT_SIGNAL ("spin_y_position","value-changed",spin_y_position_value_changed_cb);

    CONNECT_SIGNAL ("check_enable_transparency","toggled",check_enable_transparency_toggled_cb);
    CONNECT_SIGNAL ("check_animated_pulldown","toggled",check_animated_pulldown_toggled_cb);
    CONNECT_SIGNAL ("check_use_image_for_background","toggled",check_use_image_for_background_toggled_cb);
    CONNECT_SIGNAL ("spin_level_of_transparency","value-changed",spin_level_of_transparency_value_changed_cb);
    CONNECT_SIGNAL ("spin_animation_delay","value-changed",spin_animation_delay_value_changed_cb);
    CONNECT_SIGNAL ("combo_animation_orientation","changed",combo_animation_orientation_changed_cb);
    CONNECT_SIGNAL ("button_background_image","selection-changed",button_background_image_selection_changed_cb);

    /* Colors Tab */
    CONNECT_SIGNAL ("combo_colorschemes","changed",combo_colorschemes_changed_cb);
    CONNECT_SIGNAL ("colorbutton_text","color-set",colorbutton_text_color_set_cb);
    CONNECT_SIGNAL ("colorbutton_back","color-set",colorbutton_back_color_set_cb);
    CONNECT_SIGNAL ("combo_palette_scheme","changed",combo_palette_scheme_changed_cb);
    for(i = 0; i < TERMINAL_PALETTE_SIZE; i++)
    {
        char *s = g_strdup_printf ("colorbutton_palette_%d", i);
        CONNECT_SIGNAL (s,"color-set",colorbutton_palette_n_set_cb);
        g_free (s);
    }

    /* Scrolling Tab */
    CONNECT_SIGNAL ("combo_scrollbar_position","changed",combo_scrollbar_position_changed_cb);
    CONNECT_SIGNAL ("spin_scrollback_amount","value-changed",spin_scrollback_amount_value_changed_cb);
    CONNECT_SIGNAL ("check_scroll_on_output","toggled",check_scroll_on_output_toggled_cb);
    CONNECT_SIGNAL ("check_scroll_on_keystroke","toggled",check_scroll_on_keystroke_toggled_cb);
    CONNECT_SIGNAL ("check_scroll_background","toggled",check_scroll_background_toggled_cb);

    /* Compatibility Tab */
    CONNECT_SIGNAL ("combo_backspace_binding","changed",combo_backspace_binding_changed_cb);
    CONNECT_SIGNAL ("combo_delete_binding","changed",combo_delete_binding_changed_cb);
    CONNECT_SIGNAL ("button_reset_compatibility_options","clicked",button_reset_compatibility_options_clicked_cb);

    /* Keybinding Tab */
    CONNECT_SIGNAL ("button_keybinding_pulldown","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_quit","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_addtab","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_closetab","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_nexttab","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_prevtab","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_movetableft","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_movetabright","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_copy","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_paste","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab1","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab2","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab3","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab4","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab5","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab6","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab7","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab8","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab9","clicked",button_keybinding_clicked_cb);
    CONNECT_SIGNAL ("button_keybinding_gototab10","clicked",button_keybinding_clicked_cb);

    /* Close Button */
    CONNECT_SIGNAL ("button_wizard_close","clicked",button_wizard_close_clicked_cb);
    CONNECT_SIGNAL ("wizard_window","delete_event",button_wizard_close_clicked_cb);
}

/* Initialize the palette scheme menu.
 * Add the predefined schemes to the combo box.*/
static void init_palette_scheme_menu (void)
{
    gint i;
    GtkWidget *combo_palette =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_palette_scheme"));

    i = G_N_ELEMENTS (palette_schemes);
    while (i > 0) {
        gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (combo_palette), _(palette_schemes[--i].name));
    }
}

static void update_palette_color_button(gint idx)
{
    char *s = g_strdup_printf ("colorbutton_palette_%d", idx);
    GtkWidget *color_button =
        GTK_WIDGET (gtk_builder_get_object (xml, s));

    g_free (s);

    gtk_color_button_set_color (GTK_COLOR_BUTTON (color_button), &current_palette[idx]);
}

