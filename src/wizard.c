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
#include <translation.h>
#include <configsys.h>
#include <callback_func.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include <vte/vte.h> /* VTE_* constants, mostly */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

/* INT_MAX */
#include <limits.h>

/* For use in get_display_dimension() */
static enum dimensions { HEIGHT, WIDTH };

/* This will hold the libglade representation of the .glade file.
 * We keep this global so that we can look up any element from any routine.
 *
 * Note that for libglade to autoconnect signals, the functions that it hooks
 * to must NOT be marked static. I decided against the autoconnect just to
 * keep things "in the code" so that they can be grepped for easily. */
static GladeXML *xml = NULL;

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

#if ENABLE_NLS
    xml = glade_xml_new (glade_file, NULL, PACKAGE);
#else
    xml = glade_xml_new (glade_file, NULL, NULL);
#endif

    if (!xml) {
        g_warning ("problem while loading the tilda.glade file");
        return 2;
    }

    wizard_window = glade_xml_get_widget (xml, "wizard_window");

    /* We're up and running now, so now connect all of the signal handlers.
     * NOTE: I decided to do this manually, since it is safer that way. */
    //glade_xml_signal_autoconnect(xml);

    /* See the notes above, where the tw variable is declared.
     * I know how ugly this is ... */
    tw = ltw;

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

    window_title = g_strdup_printf ("Tilda %d Config", ltw->instance);
    gtk_window_set_title (GTK_WINDOW(wizard_window), window_title);
    gtk_window_set_keep_above (GTK_WINDOW(wizard_window), TRUE);
    gtk_widget_show_all (wizard_window);
    g_free (window_title);

    /* Block here until the wizard is closed successfully */
    gtk_main ();

    return 0;
}


/* Gets called just after the wizard is closed. This should clean up after
 * the wizard, and do anything that couldn't be done immediately during the
 * wizard's lifetime. */
static void wizard_closed ()
{
    DEBUG_FUNCTION ("wizard_closed");

    const GtkWidget *entry_keybinding = glade_xml_get_widget (xml, "entry_keybinding");
    const GtkWidget *entry_custom_command = glade_xml_get_widget (xml, "entry_custom_command");
    const GtkWidget *wizard_window = glade_xml_get_widget (xml, "wizard_window");

    gchar *key = gtk_entry_get_text (GTK_ENTRY(entry_keybinding));
    gchar *command = gtk_entry_get_text (GTK_ENTRY(entry_custom_command));
    gboolean key_is_valid = FALSE;

    /* Try to grab the key. This is a good way to validate it :) */
    key_is_valid = tilda_keygrabber_bind (key, tw);

    if (!key_is_valid)
    {
        /* Show the "invalid keybinding" dialog */
        show_invalid_keybinding_dialog (GTK_WINDOW(wizard_window));

        return;
    }

    config_setstr ("key", key);

    /* TODO: validate this?? */
    config_setstr ("command", command);

    /* Free the libglade data structure */
    g_object_unref (G_OBJECT(xml));
    xml = NULL;

    /* Remove the wizard */
    gtk_widget_destroy (wizard_window);

    /* Write the config, because it probably changed. This saves us in case
     * of an XKill (or crash) later ... */
    config_write (tw->config_file);

    /* This exits the wizard */
    gtk_main_quit ();
}

void show_invalid_keybinding_dialog (GtkWindow *parent_window)
{
    DEBUG_FUNCTION ("show_invalid_keybinding_dialog");

    GtkWidget *dialog = gtk_message_dialog_new (parent_window,
                              GTK_DIALOG_DESTROY_WITH_PARENT,
                              GTK_MESSAGE_ERROR,
                              GTK_BUTTONS_CLOSE,
                              _("The keybinding you chose is invalid. Please choose another."));

    gtk_window_set_keep_above (GTK_WINDOW(dialog), TRUE);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

/******************************************************************************/
/*               ALL static callback helpers are below                        */
/******************************************************************************/

/*
 * The functions wizard_key_grab() and setup_key_grab() are totally inspired by
 * the custom keybindings dialog in denemo. See http://denemo.sourceforge.net/
 *
 * Thanks for the help! :)
 */

/*
 * This function gets called once per key that is pressed. This means we have to
 * manually ignore all modifier keys, and only register "real" keys.
 *
 * We return when the first "real" key is pressed.
 */
static void wizard_key_grab (GtkWidget *wizard_window, GdkEventKey *event)
{
    DEBUG_FUNCTION ("wizard_key_grab");
    DEBUG_ASSERT (wizard_window != NULL);
    DEBUG_ASSERT (event != NULL);

    const GtkWidget *button_grab_keybinding = glade_xml_get_widget (xml, "button_grab_keybinding");
    const GtkWidget *wizard_notebook = glade_xml_get_widget (xml, "wizard_notebook");
    const GtkWidget *entry_keybinding = glade_xml_get_widget (xml, "entry_keybinding");
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
        /* Re-enable widgets */
        gtk_widget_set_sensitive (button_grab_keybinding, TRUE);
        gtk_widget_set_sensitive (wizard_notebook, TRUE);

        /* Disconnect the key grabber */
        g_signal_handlers_disconnect_by_func (GTK_OBJECT(wizard_window), GTK_SIGNAL_FUNC(wizard_key_grab), NULL);

        /* Copy the pressed key to the text entry */
        gtk_entry_set_text (GTK_ENTRY(entry_keybinding), key);

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
    g_signal_handlers_block_by_func (spin, GTK_SIGNAL_FUNC(*callback), NULL);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin), new_val);
    g_signal_handlers_unblock_by_func (spin, GTK_SIGNAL_FUNC(*callback), NULL);
}

/******************************************************************************/
/*                       ALL Callbacks are below                              */
/******************************************************************************/

static void button_wizard_close_clicked_cb (GtkWidget *w)
{
    const GtkWidget *wizard_window = glade_xml_get_widget (xml, "wizard_window");

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
        //gtk_widget_set_double_buffered (VTE_TERMINAL(tt->vte_term), status);
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
        vte_terminal_set_cursor_blinks (VTE_TERMINAL(tt->vte_term), status);
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
        vte_terminal_set_font_from_string_full (VTE_TERMINAL(tt->vte_term), config_getstr ("font"), status);
    }
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
        vte_terminal_set_font_from_string_full (VTE_TERMINAL(tt->vte_term), font, antialias);
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

    label_custom_command = glade_xml_get_widget (xml, "label_custom_command");
    entry_custom_command = glade_xml_get_widget (xml, "entry_custom_command");

    gtk_widget_set_sensitive (label_custom_command, status);
    gtk_widget_set_sensitive (entry_custom_command, status);
}

static void combo_command_exit_changed_cb (GtkWidget *w)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("command_exit", status);
}

static void entry_web_browser_changed (GtkWidget *w)
{
    const gchar *web_browser = gtk_entry_get_text (GTK_ENTRY(w));

    config_setstr ("web_browser", web_browser);
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
    const GtkWidget *spin_height_pixels = glade_xml_get_widget (xml, "spin_height_pixels");
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
    const GtkWidget *spin_height_percentage = glade_xml_get_widget (xml, "spin_height_percentage");
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
    const GtkWidget *spin_width_pixels = glade_xml_get_widget (xml, "spin_width_pixels");
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
    const GtkWidget *spin_width_percentage = glade_xml_get_widget (xml, "spin_width_percentage");
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
    const GtkWidget *label_x_position = glade_xml_get_widget (xml, "label_x_position");
    const GtkWidget *spin_x_position = glade_xml_get_widget (xml, "spin_x_position");

    config_setbool ("centered_horizontally", status);

    if (status)
        config_setint ("x_pos", find_centering_coordinate (gdk_screen_width(), config_getint ("max_width")));
    else
        config_setint ("x_pos", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_x_position)));

    gtk_widget_set_sensitive (label_x_position, !status);
    gtk_widget_set_sensitive (spin_x_position, !status);

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
    const GtkWidget *label_y_position = glade_xml_get_widget (xml, "label_y_position");
    const GtkWidget *spin_y_position = glade_xml_get_widget (xml, "spin_y_position");

    config_setbool ("centered_vertically", status);

    if (status)
        config_setint ("y_pos", find_centering_coordinate (gdk_screen_height(), config_getint ("max_height")));
    else
        config_setint ("y_pos", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_y_position)));

    gtk_widget_set_sensitive (label_y_position, !status);
    gtk_widget_set_sensitive (spin_y_position, !status);

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
    const GtkWidget *label_level_of_transparency = glade_xml_get_widget (xml, "label_level_of_transparency");
    const GtkWidget *spin_level_of_transparency = glade_xml_get_widget (xml, "spin_level_of_transparency");
    const gdouble transparency_level = (gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_level_of_transparency)) / 100.0);
    gint i;
    tilda_term *tt;

    config_setbool ("enable_transparency", status);

    gtk_widget_set_sensitive (label_level_of_transparency, status);
    gtk_widget_set_sensitive (spin_level_of_transparency, status);

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
    const GtkWidget *label_animation_delay = glade_xml_get_widget (xml, "label_animation_delay");
    const GtkWidget *spin_animation_delay = glade_xml_get_widget (xml, "spin_animation_delay");
    const GtkWidget *label_animation_orientation = glade_xml_get_widget (xml, "label_animation_orientation");
    const GtkWidget *combo_animation_orientation = glade_xml_get_widget (xml, "combo_animation_orientation");

    gtk_widget_set_sensitive (label_animation_delay, status);
    gtk_widget_set_sensitive (spin_animation_delay, status);
    gtk_widget_set_sensitive (label_animation_orientation, status);
    gtk_widget_set_sensitive (combo_animation_orientation, status);

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
    const GtkWidget *button_background_image = glade_xml_get_widget (xml, "button_background_image");
    const gchar *image = config_getstr ("image");
    gint i;
    tilda_term *tt;


    config_setbool ("use_image", status);

    gtk_widget_set_sensitive (button_background_image, status);

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
    const GtkWidget *colorbutton_text = glade_xml_get_widget (xml, "colorbutton_text");
    const GtkWidget *colorbutton_back = glade_xml_get_widget (xml, "colorbutton_back");
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
    const GtkWidget *combo_colorschemes = glade_xml_get_widget (xml, "combo_colorschemes");
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
    const GtkWidget *combo_colorschemes = glade_xml_get_widget (xml, "combo_colorschemes");
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
    const GtkWidget *combo_backspace_binding = glade_xml_get_widget (xml, "combo_backspace_binding");
    const GtkWidget *combo_delete_binding = glade_xml_get_widget (xml, "combo_delete_binding");

    config_setint ("backspace_key", 0);
    config_setint ("delete_key", 1);

    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_backspace_binding), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_delete_binding), 1);
}

static void button_grab_keybinding_clicked_cb (GtkWidget *w)
{
    const GtkWidget *wizard_notebook = glade_xml_get_widget (xml, "wizard_notebook");
    const GtkWidget *wizard_window = glade_xml_get_widget (xml, "wizard_window");

    /* Make the preferences window non-sensitive while we are grabbing keys */
    gtk_widget_set_sensitive (w, FALSE);
    gtk_widget_set_sensitive (wizard_notebook, FALSE);

    /* Connect the key grabber to the preferences window */
    g_signal_connect (GTK_OBJECT(wizard_window), "key_press_event", GTK_SIGNAL_FUNC(wizard_key_grab), NULL);
}

/******************************************************************************/
/*                       Wizard set-up functions                              */
/******************************************************************************/


/* Defines to make the process of setting all of the initial values easier */
#define WIDGET_SET_INSENSITIVE(GLADE_NAME) gtk_widget_set_sensitive (glade_xml_get_widget (xml, (GLADE_NAME)), FALSE)
#define CHECK_BUTTON(GLADE_NAME,CFG_BOOL) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, (GLADE_NAME))), config_getbool ((CFG_BOOL)))
#define COMBO_BOX(GLADE_NAME,CFG_INT) gtk_combo_box_set_active (GTK_COMBO_BOX(glade_xml_get_widget (xml, (GLADE_NAME))), config_getint ((CFG_INT)))
#define FONT_BUTTON(GLADE_NAME,CFG_STR) gtk_font_button_set_font_name (GTK_FONT_BUTTON(glade_xml_get_widget (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define TEXT_ENTRY(GLADE_NAME,CFG_STR) gtk_entry_set_text (GTK_ENTRY(glade_xml_get_widget (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define SPIN_BUTTON(GLADE_NAME,CFG_INT) gtk_spin_button_set_value (GTK_SPIN_BUTTON(glade_xml_get_widget (xml, (GLADE_NAME))), config_getint ((CFG_INT)))
#define SPIN_BUTTON_SET_RANGE(GLADE_NAME,LOWER,UPPER) gtk_spin_button_set_range (GTK_SPIN_BUTTON(glade_xml_get_widget (xml, (GLADE_NAME))), (LOWER), (UPPER))
#define SPIN_BUTTON_SET_VALUE(GLADE_NAME,VALUE) gtk_spin_button_set_value (GTK_SPIN_BUTTON(glade_xml_get_widget (xml, (GLADE_NAME))), (VALUE))
#define FILE_BUTTON(GLADE_NAME,CFG_STR) gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(glade_xml_get_widget (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define COLOR_BUTTON(GLADE_NAME,COLOR) gtk_color_button_set_color (GTK_COLOR_BUTTON(glade_xml_get_widget (xml, (GLADE_NAME))), (COLOR))
#define SET_SENSITIVE_BY_CONFIG_BOOL(GLADE_NAME,CFG_BOOL) gtk_widget_set_sensitive (glade_xml_get_widget (xml, (GLADE_NAME)), config_getbool ((CFG_BOOL)))
#define SET_SENSITIVE_BY_CONFIG_NBOOL(GLADE_NAME,CFG_BOOL) gtk_widget_set_sensitive (glade_xml_get_widget (xml, (GLADE_NAME)), !config_getbool ((CFG_BOOL)))

/* Read all state from the config system, and put it into
 * its visual representation in the wizard. */
static void set_wizard_state_from_config ()
{
    GdkColor text_color, back_color;

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

    /* Title and Command Tab */
    TEXT_ENTRY ("entry_title", "title");
    COMBO_BOX ("combo_dynamically_set_title", "d_set_title");

    CHECK_BUTTON ("check_run_custom_command", "run_command");
    TEXT_ENTRY ("entry_custom_command", "command");
    COMBO_BOX ("combo_command_exit", "command_exit");
    SET_SENSITIVE_BY_CONFIG_BOOL ("entry_custom_command","run_command");
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_custom_command", "run_command");

    TEXT_ENTRY ("entry_web_browser", "web_browser");

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
    FILE_BUTTON ("button_background_image", "image");
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
    TEXT_ENTRY ("entry_keybinding", "key");
}

#define CONNECT_SIGNAL(GLADE_WIDGET,SIGNAL_NAME,SIGNAL_HANDLER) g_signal_connect (glade_xml_get_widget (xml, (GLADE_WIDGET)), (SIGNAL_NAME), GTK_SIGNAL_FUNC((SIGNAL_HANDLER)), NULL)

/* Connect all signals in the wizard. This should be done after setting all
 * values, that way all of the signal handlers don't get called */
static void connect_wizard_signals ()
{
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

    /* Title and Command Tab */
    CONNECT_SIGNAL ("entry_title","changed",entry_title_changed_cb);
    CONNECT_SIGNAL ("combo_dynamically_set_title","changed",combo_dynamically_set_title_changed_cb);

    CONNECT_SIGNAL ("check_run_custom_command","toggled",check_run_custom_command_toggled_cb);
    CONNECT_SIGNAL ("combo_command_exit","changed",combo_command_exit_changed_cb);

    CONNECT_SIGNAL ("entry_web_browser","changed",entry_web_browser_changed);

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
    CONNECT_SIGNAL ("button_grab_keybinding","clicked",button_grab_keybinding_clicked_cb);

    /* Close Button */
    CONNECT_SIGNAL ("button_wizard_close","clicked",button_wizard_close_clicked_cb);
    CONNECT_SIGNAL ("wizard_window","delete_event",button_wizard_close_clicked_cb);
}

