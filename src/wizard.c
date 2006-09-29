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

#include <gtk/gtk.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

/* INT_MAX */
#include <limits.h>

#include "tilda.h"
#include "../tilda-config.h"
#include "config.h"
#include "wizard.h"
#include "load_tilda.h"


/* This struct will hold all of the configuration items that
 * are needed throughout this file */
static struct wizard_items items;

gboolean in_main = FALSE;
static gint exit_status = 0;

static void close_dialog (GtkWidget *widget, gpointer data)
{
#ifdef DEBUG
    puts("close_dialog");
#endif

    gtk_grab_remove (GTK_WIDGET (widget));
    gtk_widget_destroy (GTK_WIDGET (data));
}

static void image_select (GtkWidget *widget, GtkWidget *label_image)
{
#ifdef DEBUG
    puts("image_select");
#endif

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
        gtk_widget_set_sensitive (label_image, TRUE);
        gtk_widget_set_sensitive (items.button_background_image, TRUE);
    }
    else
    {
        gtk_widget_set_sensitive (label_image, FALSE);
        gtk_widget_set_sensitive (items.button_background_image, FALSE);
    }
}

static void toggle_check_enable_transparency (GtkWidget *widget, GtkWidget *label_level_of_transparency)
{
#ifdef DEBUG
    puts ("toggle_check_enable_transparency");
#endif

    const int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    gtk_widget_set_sensitive (label_level_of_transparency, active);
    gtk_widget_set_sensitive (items.spin_level_of_transparency, active);
}

static void toggle_check_animated_pulldown1 (GtkWidget *widget, GtkWidget *label_animation)
{
#ifdef DEBUG
    puts ("toggle_check_animated_pulldown1");
#endif

    const int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    gtk_widget_set_sensitive (label_animation, active);
    gtk_widget_set_sensitive (items.spin_animation_delay, active);
}

static void toggle_check_animated_pulldown2 (GtkWidget *widget, GtkWidget *label_orientation)
{
#ifdef DEBUG
    puts ("toggle_check_animated_pulldown2");
#endif

    const int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    gtk_widget_set_sensitive (label_orientation, active);
    gtk_widget_set_sensitive (items.combo_orientation, active);
}

static void toggle_centered_horizontally (GtkWidget *widget, GtkWidget *label_position)
{
    const int active = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    gtk_widget_set_sensitive (label_position, active);
    gtk_widget_set_sensitive (items.spin_x_position, active);
}

static void toggle_centered_vertically (GtkWidget *widget, GtkWidget *label_position)
{
    const int active = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    gtk_widget_set_sensitive (label_position, active);
    gtk_widget_set_sensitive (items.spin_y_position, active);
}

static void toggle_check_run_custom_command (GtkWidget *widget, GtkWidget *label_custom_command)
{
    const int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));

    gtk_widget_set_sensitive (label_custom_command, active);
    gtk_widget_set_sensitive (items.entry_custom_command, active);
}

static int percentage_dimension (int current_size, int dimension)
{
    if (dimension == HEIGHT)
        return (int) (((float) current_size) / ((float) get_physical_height_pixels ()) * 100.0);

    return (int) (((float) current_size) / ((float) get_physical_width_pixels ()) * 100.0);
}

#define percentage_height(current_height) percentage_dimension(current_height, HEIGHT)
#define percentage_width(current_width)   percentage_dimension(current_width, WIDTH)

static void generic_spin_percentage_changed (GtkWidget *event_spinner, GtkWidget *change_spinner, gulong callback_id, int dimension)
{
    /*
     * 1) Calculate new values
     * 2) Disable signal handler on target object
     * 3) Set new values
     * 4) Re-enable signal handler on target object
     */

    const int percentage = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (event_spinner));
    const int in_pixels = (percentage / 100.0) * get_display_dimension (dimension);

    g_signal_handler_block (change_spinner, callback_id);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON(change_spinner), in_pixels);

    g_signal_handler_unblock (change_spinner, callback_id);
}

static void spin_height_percentage_changed (GtkWidget *event_spinner, GtkWidget *change_spinner)
{
    generic_spin_percentage_changed (event_spinner, change_spinner, items.cid_height_pixels, HEIGHT);
}

static void spin_width_percentage_changed (GtkWidget *event_spinner, GtkWidget *change_spinner)
{
    generic_spin_percentage_changed (event_spinner, change_spinner, items.cid_width_pixels, WIDTH);
}

static void generic_spin_pixels_changed (GtkWidget *event_spinner, GtkWidget *changed_spinner, gulong callback_id, int dimension)
{
    /*
     * 1) Calculate new values
     * 2) Disable signal handler on target object
     * 3) Set new values
     * 4) Re-enable signal handler on target object
     */

    const int pixels = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(event_spinner));
    const int percentage = percentage_dimension (pixels, dimension);

    g_signal_handler_block (changed_spinner, callback_id);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON(changed_spinner), percentage);

    g_signal_handler_unblock (changed_spinner, callback_id);
}

static void spin_height_pixels_changed (GtkWidget *event_spinner, GtkWidget *change_spinner)
{
    generic_spin_pixels_changed (event_spinner, change_spinner, items.cid_height_percentage, HEIGHT);
}

static void spin_width_pixels_changed (GtkWidget *event_spinner, GtkWidget *change_spinner)
{
    generic_spin_pixels_changed (event_spinner, change_spinner, items.cid_width_percentage, WIDTH);
}

static GtkWidget* general (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("general");
#endif

    GtkWidget *vtable;
    GtkWidget *frame_wdisplay;
    GtkWidget *frame_tdisplay;
    GtkWidget *frame_font;

    GtkWidget *table_wdisplay;
    GtkWidget *table_tdisplay;
    GtkWidget *table_font;

    GtkWidget *label_tab_pos;

    /* Create the table that will hold the 3 frames */
    vtable = gtk_table_new (3, 1, FALSE);
    frame_wdisplay = gtk_frame_new ("Window Display");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_wdisplay))), "<b>Window Display</b>");
    frame_tdisplay = gtk_frame_new ("Terminal Display");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_tdisplay))), "<b>Terminal Display</b>");
    frame_font = gtk_frame_new ("Font");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_font))), "<b>Font</b>");

    /* Attach frames to the table */
    gtk_table_attach (GTK_TABLE(vtable), frame_wdisplay, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(vtable), frame_tdisplay, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(vtable), frame_font , 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);

    /* =========== WINDOW DISPLAY FRAME ============= */

    /* Create everything to fill the Window Display frame */
    table_wdisplay = gtk_table_new (3, 2, FALSE);
    items.check_display_on_all_workspaces = gtk_check_button_new_with_label ("Display on all workspaces");
    items.check_always_on_top = gtk_check_button_new_with_label ("Always on top");
    items.check_do_not_show_in_taskbar = gtk_check_button_new_with_label ("Do not show in taskbar");
    items.check_start_tilda_hidden = gtk_check_button_new_with_label ("Start Tilda hidden");
    items.check_show_notebook_border = gtk_check_button_new_with_label ("Show Notebook Border");

    /* Get the current values */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_display_on_all_workspaces), cfg_getbool (tw->tc, "pinned"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_always_on_top), cfg_getbool (tw->tc, "above"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_do_not_show_in_taskbar), cfg_getbool (tw->tc, "notaskbar"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_start_tilda_hidden), cfg_getbool (tw->tc, "hidden"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_show_notebook_border), cfg_getbool (tw->tc, "notebook_border"));

    /* Attach everything inside the frame */
    gtk_table_attach (GTK_TABLE(table_wdisplay), items.check_display_on_all_workspaces, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_wdisplay), items.check_always_on_top, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_wdisplay), items.check_do_not_show_in_taskbar, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_wdisplay), items.check_start_tilda_hidden, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_wdisplay), items.check_show_notebook_border, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);

    gtk_container_add (GTK_CONTAINER(frame_wdisplay), table_wdisplay);

    /* ============ TERMINAL DISPLAY FRAME ============= */

    /* Create everything to fill the Terminal Display frame */
    table_tdisplay = gtk_table_new (2, 1, FALSE);
    items.check_terminal_bell = gtk_check_button_new_with_label ("Terminal Bell");
    items.check_cursor_blinks = gtk_check_button_new_with_label ("Cursor blinks");

    /* Get the current values */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_terminal_bell), cfg_getbool (tw->tc, "bell"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_cursor_blinks), cfg_getbool (tw->tc, "blinks"));

    /* Attach everything inside the frame */
    gtk_table_attach (GTK_TABLE(table_tdisplay), items.check_terminal_bell, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_tdisplay), items.check_cursor_blinks, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);

    gtk_container_add (GTK_CONTAINER(frame_tdisplay), table_tdisplay);
    

    /* ============== FONT FRAME =============== */

    /* Create everything to fill the Font frame */
    table_font = gtk_table_new (3, 2, FALSE);
    label_tab_pos = gtk_label_new ("Position of Tabs: ");
    items.check_enable_antialias = gtk_check_button_new_with_label ("Enable anti-aliasing");
    items.check_allow_bold_text = gtk_check_button_new_with_label ("Allow bold text");
    items.combo_tab_position = gtk_combo_box_new_text    ();
    items.button_font = gtk_font_button_new_with_font (cfg_getstr(tw->tc, "font"));

    /* Get the current values */
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_tab_position), "RIGHT");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_tab_position), "LEFT");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_tab_position), "BOTTOM");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_tab_position), "TOP");
    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_tab_position), cfg_getint (tw->tc, "tab_pos"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_enable_antialias), cfg_getbool (tw->tc, "antialias"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_allow_bold_text), cfg_getbool (tw->tc, "bold"));

    /* Attach everything to the table */
    gtk_table_attach (GTK_TABLE(table_font), items.check_enable_antialias, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_font), items.check_allow_bold_text, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_font), label_tab_pos, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_font), items.combo_tab_position, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_font), items.button_font, 0, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);

    gtk_container_add (GTK_CONTAINER(frame_font), table_font);

    gtk_widget_show_all (vtable);
    return vtable;
}

static GtkWidget* title_command (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("title_command");
#endif

    GtkWidget *vtable;
    GtkWidget *table_title;
    GtkWidget *table_command;
    GtkWidget *frame_title;
    GtkWidget *frame_command;
    GtkWidget *label_initial_title;
    GtkWidget *label_dynamically_set;
    GtkWidget *label_custom_command;
    GtkWidget *label_command_exit;

    /* Create the outermost table and the two frames to go inside */
    vtable = gtk_table_new (2, 1, FALSE);

    frame_title = gtk_frame_new ("Title");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_title))), "<b>Title</b>");

    frame_command = gtk_frame_new ("Command");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_command))), "<b>Command</b>");

    /* Attach the frames to the table */
    gtk_table_attach (GTK_TABLE(vtable), frame_title, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(vtable), frame_command, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);

    /* Create the table and sub-items that will live inside the title frame */
    table_title = gtk_table_new (2, 2, FALSE);
    label_initial_title = gtk_label_new ("Initial Title:");
    label_dynamically_set = gtk_label_new ("Dynamically-set Title:");
    items.entry_title = gtk_entry_new ();

    items.combo_dynamically_set_title = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_dynamically_set_title), "Replace initial title");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_dynamically_set_title), "Goes before initial title");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_dynamically_set_title), "Goes after initial title");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_dynamically_set_title), "Isn't displayed");

    /* Get the current values from the config file */
    gtk_entry_set_text (GTK_ENTRY (items.entry_title), cfg_getstr (tw->tc, "title"));
    gtk_combo_box_set_active   (GTK_COMBO_BOX(items.combo_dynamically_set_title), cfg_getint (tw->tc, "d_set_title"));

    /* Attach everything into the table inside the title frame */
    gtk_table_attach (GTK_TABLE(table_title), label_initial_title, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_title), items.entry_title, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_title), label_dynamically_set, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_title), items.combo_dynamically_set_title, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_container_add (GTK_CONTAINER (frame_title), table_title);

    /* Create the table and sub-items that will live inside the command frame */
    table_command = gtk_table_new (3, 2, FALSE);
    items.check_run_custom_command = gtk_check_button_new_with_label ("Run a custom command instead of shell");
    label_custom_command = gtk_label_new ("Custom command:");
    items.entry_custom_command = gtk_entry_new ();
    label_command_exit = gtk_label_new ("When command exit:");

    items.combo_command_exit = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_command_exit), "Exit the terminal");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_command_exit), "Restart the command");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_command_exit), "Hold the terminal open");

    /* Get the current values from the config file */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_run_custom_command), cfg_getbool (tw->tc, "run_command"));
    gtk_entry_set_text (GTK_ENTRY (items.entry_custom_command), cfg_getstr (tw->tc, "command"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_run_custom_command), cfg_getbool (tw->tc, "run_command"));
    gtk_combo_box_set_active   (GTK_COMBO_BOX(items.combo_command_exit), cfg_getint(tw->tc, "command_exit"));

    /* Connect signals */
    g_signal_connect (GTK_WIDGET (items.check_run_custom_command), "clicked", GTK_SIGNAL_FUNC(toggle_check_run_custom_command), label_custom_command);

    /* Set sensitivity (force a callback, essentially) */
    toggle_check_run_custom_command (items.check_run_custom_command, label_custom_command);

    /* Attach everything into the table inside the command frame */
    gtk_table_attach (GTK_TABLE(table_command), items.check_run_custom_command, 0, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_command), label_custom_command, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_command), items.entry_custom_command, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_command), label_command_exit, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_command), items.combo_command_exit, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_container_add (GTK_CONTAINER (frame_command), table_command);

    /* Show everything */
    gtk_widget_show_all (vtable);

    /* Return this entire tree of widgets, which will become the Title and Command Notebook tab */
    return vtable;
}

static GtkWidget* appearance (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("appearance");
#endif

    GtkWidget *vtable;
    GtkWidget *frame_height;
    GtkWidget *frame_width;
    GtkWidget *frame_position;
    GtkWidget *frame_extras;

    GtkWidget *table_height;
    GtkWidget *label_height_percentage;
    GtkWidget *label_height_pixels;

    GtkWidget *table_width;
    GtkWidget *label_width_percentage;
    GtkWidget *label_width_pixels;

    GtkWidget *table_position;
    GtkWidget *label_x_pos;
    GtkWidget *label_y_pos;

    GtkWidget *table_extras;
    GtkWidget *label_level_of_transparency;
    GtkWidget *label_animation_delay;
    GtkWidget *label_orientation;
    GtkWidget *label_background_image;

    /* Create the table that will hold the 4 frames, and the frames to go inside */
    vtable = gtk_table_new (4, 1, FALSE);
    frame_height = gtk_frame_new ("Height");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_height))), "<b>Height</b>");
    frame_width = gtk_frame_new ("Width");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_width))), "<b>Width</b>");
    frame_position = gtk_frame_new ("Position");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_position))), "<b>Position</b>");
    frame_extras = gtk_frame_new ("Extras");
    gtk_label_set_markup (GTK_LABEL(gtk_frame_get_label_widget (GTK_FRAME(frame_extras))), "<b>Extras</b>");

    /* Attach the frames to the table */
    gtk_table_attach (GTK_TABLE(vtable), frame_height, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(vtable), frame_width, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(vtable), frame_position, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(vtable), frame_extras, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);

    /* ========== HEIGHT FRAME ========== */

    /* Create everything to fill the Height frame */
    table_height = gtk_table_new (1, 4, FALSE);
    label_height_percentage = gtk_label_new ("Percentage");
    items.spin_height_percentage = gtk_spin_button_new_with_range (0, 100, 1);
    label_height_pixels = gtk_label_new ("In Pixels");
    items.spin_height_pixels = gtk_spin_button_new_with_range (0, get_physical_height_pixels(), 1);

    /* Get the current values */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(items.spin_height_pixels), cfg_getint (tw->tc, "max_height"));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(items.spin_height_percentage), percentage_height (cfg_getint (tw->tc, "max_height")));

    /* Connect signals */
    items.cid_height_percentage = g_signal_connect (G_OBJECT(items.spin_height_percentage), "value-changed", G_CALLBACK(spin_height_percentage_changed), items.spin_height_pixels);
    items.cid_height_pixels = g_signal_connect (G_OBJECT(items.spin_height_pixels), "value-changed", G_CALLBACK(spin_height_pixels_changed), items.spin_height_percentage);

    /* Attach everything into the Height frame */
    gtk_table_attach (GTK_TABLE(table_height), label_height_percentage, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_height), items.spin_height_percentage, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_height), label_height_pixels, 2, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_height), items.spin_height_pixels, 3, 4, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_container_add (GTK_CONTAINER (frame_height), table_height);

    /* ========== WIDTH FRAME ========== */

    /* Create everything to fill the Width frame */
    table_width = gtk_table_new (1, 4, FALSE);
    label_width_percentage = gtk_label_new ("Percentage");
    items.spin_width_percentage = gtk_spin_button_new_with_range (0, 100, 1);
    label_width_pixels = gtk_label_new ("In Pixels");
    items.spin_width_pixels = gtk_spin_button_new_with_range (0, get_physical_width_pixels(), 1);

    /* Get the current values */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(items.spin_width_pixels), cfg_getint (tw->tc, "max_width"));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(items.spin_width_percentage), percentage_width (cfg_getint (tw->tc, "max_width")));

    /* Connect signals */
    items.cid_width_percentage = g_signal_connect (G_OBJECT(items.spin_width_percentage), "value-changed", G_CALLBACK(spin_width_percentage_changed), items.spin_width_pixels);
    items.cid_width_pixels = g_signal_connect (G_OBJECT(items.spin_width_pixels), "value-changed", G_CALLBACK(spin_width_pixels_changed), items.spin_width_percentage);

    /* Attach everything into the Width frame */
    gtk_table_attach (GTK_TABLE(table_width), label_width_percentage, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_width), items.spin_width_percentage, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_width), label_width_pixels, 2, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_width), items.spin_width_pixels, 3, 4, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_container_add (GTK_CONTAINER (frame_width), table_width);

    /* ========== POSITION FRAME ========== */

    /* Create everything to fill the Position frame */
    table_position = gtk_table_new (2, 4, FALSE);
    items.check_centered_vertically = gtk_check_button_new_with_label ("Centered Vertically");
    label_x_pos = gtk_label_new ("X Position");
    items.spin_x_position = gtk_spin_button_new_with_range (INT_MIN, INT_MAX, 1);
    items.check_centered_horizontally = gtk_check_button_new_with_label ("Centered Horizontally");
    label_y_pos = gtk_label_new ("Y Position");
    items.spin_y_position = gtk_spin_button_new_with_range (INT_MIN, INT_MAX, 1);

    /* Get the current values */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(items.spin_x_position), cfg_getint (tw->tc, "x_pos"));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(items.spin_y_position), cfg_getint (tw->tc, "y_pos"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(items.check_centered_horizontally), cfg_getbool (tw->tc, "centered_horizontally"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(items.check_centered_vertically), cfg_getbool (tw->tc, "centered_vertically"));

    /* Force callbacks for visibility */
    toggle_centered_horizontally (items.check_centered_horizontally, label_x_pos);
    toggle_centered_vertically (items.check_centered_vertically, label_y_pos);

    /* Connect signals */
    g_signal_connect (GTK_WIDGET(items.check_centered_horizontally), "clicked", GTK_SIGNAL_FUNC(toggle_centered_horizontally), label_x_pos);
    g_signal_connect (GTK_WIDGET(items.check_centered_vertically), "clicked", GTK_SIGNAL_FUNC(toggle_centered_vertically), label_y_pos);

    /* Attach everything into the Position frame */
    gtk_table_attach (GTK_TABLE(table_position), items.check_centered_horizontally, 0, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_position), items.check_centered_vertically, 2, 4, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_position), label_x_pos, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_position), items.spin_x_position, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_position), label_y_pos, 2, 3, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_position), items.spin_y_position, 3, 4, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_container_add (GTK_CONTAINER (frame_position), table_position);

    /* ========== EXTRAS FRAME ========== */

    /* Create everyting to fill the Extras frame */
    table_extras = gtk_table_new (4, 3, FALSE);
    items.check_enable_transparency = gtk_check_button_new_with_label ("Enable Transparency");
    label_level_of_transparency = gtk_label_new ("Level of Transparency");
    items.spin_level_of_transparency = gtk_spin_button_new_with_range (0, 100, 1);
    items.check_animated_pulldown = gtk_check_button_new_with_label ("Animated Pulldown");
    label_animation_delay = gtk_label_new ("Animation Delay (usec)");
    items.spin_animation_delay = gtk_spin_button_new_with_range (1, INT_MAX, 1);
    label_orientation = gtk_label_new ("Animation Orientation");
    items.combo_orientation = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_orientation), "RIGHT");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_orientation), "LEFT");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_orientation), "BOTTOM");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_orientation), "TOP");
    items.check_use_image_for_background = gtk_check_button_new_with_label ("Use Image for Background");
    label_background_image = gtk_label_new ("Background Image");

    items.chooser_background_image = gtk_file_chooser_dialog_new ("Open Background Image File",
            GTK_WINDOW (items.wizard_window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
            NULL);
    items.button_background_image = gtk_file_chooser_button_new_with_dialog (items.chooser_background_image);

    /* Connect signals */
    g_signal_connect (GTK_WIDGET(items.check_enable_transparency), "clicked", GTK_SIGNAL_FUNC(toggle_check_enable_transparency), label_level_of_transparency);
    g_signal_connect (GTK_WIDGET(items.check_use_image_for_background), "clicked", GTK_SIGNAL_FUNC(image_select), label_background_image);
    g_signal_connect (GTK_WIDGET(items.check_animated_pulldown), "clicked", GTK_SIGNAL_FUNC(toggle_check_animated_pulldown1), label_animation_delay);
    g_signal_connect (GTK_WIDGET(items.check_animated_pulldown), "clicked", GTK_SIGNAL_FUNC(toggle_check_animated_pulldown2), label_orientation);

    /* Get the current values */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(items.check_enable_transparency), cfg_getbool (tw->tc, "enable_transparency"));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(items.spin_level_of_transparency), cfg_getint (tw->tc, "transparency"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(items.check_animated_pulldown), cfg_getbool (tw->tc, "animation"));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(items.spin_animation_delay), cfg_getint (tw->tc, "slide_sleep_usec"));
    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_orientation), cfg_getint (tw->tc, "animation_orientation"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(items.check_use_image_for_background), cfg_getbool (tw->tc, "use_image"));
    if (cfg_getstr (tw->tc, "image") != NULL)
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(items.chooser_background_image),
                cfg_getstr (tw->tc, "image"));

    /* Force callbacks for visibility purposes */
    toggle_check_enable_transparency (items.check_enable_transparency, label_level_of_transparency);
    image_select (items.check_use_image_for_background, label_background_image);
    toggle_check_animated_pulldown1 (items.check_animated_pulldown, label_animation_delay);
    toggle_check_animated_pulldown2 (items.check_animated_pulldown, label_orientation);

    /* Attach everything to the Extras frame */
    gtk_table_attach (GTK_TABLE(table_extras), items.check_enable_transparency, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), label_level_of_transparency, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), items.spin_level_of_transparency, 2, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), items.check_animated_pulldown, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), label_animation_delay, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), items.spin_animation_delay, 2, 3, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), label_orientation, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), items.combo_orientation, 2, 3, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), items.check_use_image_for_background, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_table_attach (GTK_TABLE(table_extras), items.button_background_image, 1, 3, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 4);
    gtk_container_add (GTK_CONTAINER(frame_extras), table_extras);

    gtk_widget_show_all (vtable);
    return vtable;
}

static void scheme_colors_changed (tilda_window *tw)
{
#ifdef DEBUG
    puts("scheme_colors_changed");
#endif

    GdkColor gdk_text, gdk_back;

    gint scheme = gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_schemes));

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

            break;
    }

    gtk_color_button_set_color (GTK_COLOR_BUTTON(items.button_text_color), &gdk_text);
    gtk_color_button_set_color (GTK_COLOR_BUTTON(items.button_background_color), &gdk_back);
}

static void button_colors_changed (tilda_window *tw)
{
#ifdef DEBUG
    puts("button_colors_changed");
#endif

    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_schemes), 0);
}

static GtkWidget* colors (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("colors");
#endif

    GtkWidget *table;
    GtkWidget *label_color;
    GtkWidget *label_builtin;
    GtkWidget *label_text_color;
    GtkWidget *label_back_color;
    GdkColor text, back;

    table = gtk_table_new (4, 4, FALSE);

    label_color = gtk_label_new ("Foreground and Background Colors:");
    gtk_label_set_markup (GTK_LABEL(label_color), "<b>Foreground and Background Colors:</b>");
    label_builtin = gtk_label_new ("Built-in schemes:");
    label_text_color = gtk_label_new ("Text Color:");
    label_back_color = gtk_label_new ("Background Color:");

    items.combo_schemes = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_schemes), "White on Black");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_schemes), "Black on White");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_schemes), "Green on Black");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_schemes), "Custom");
    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_schemes), cfg_getint (tw->tc, "scheme"));

    text.red   =    cfg_getint (tw->tc, "text_red");
    text.green =    cfg_getint (tw->tc, "text_green");
    text.blue  =    cfg_getint (tw->tc, "text_blue");
    back.red   =    cfg_getint (tw->tc, "back_red");
    back.green =    cfg_getint (tw->tc, "back_green");
    back.blue  =    cfg_getint (tw->tc, "back_blue");

    items.button_text_color = gtk_color_button_new_with_color (&text);
    items.button_background_color = gtk_color_button_new_with_color (&back);

    g_signal_connect_swapped (G_OBJECT (items.combo_schemes), "changed",
                  G_CALLBACK (scheme_colors_changed), tw);
    g_signal_connect_swapped (G_OBJECT (items.button_text_color), "color-set",
                  G_CALLBACK (button_colors_changed), tw);
    g_signal_connect_swapped (G_OBJECT (items.button_background_color), "color-set",
                  G_CALLBACK (button_colors_changed), tw);

    gtk_table_attach (GTK_TABLE (table), label_color, 0, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_builtin, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.combo_schemes, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_text_color, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.button_text_color, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), label_back_color, 2, 3, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.button_background_color, 3, 4, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show_all (table);

    return table;
}

static GtkWidget* scrolling (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("scrolling");
#endif

    GtkWidget *table;
    GtkWidget *label_scrollback;
    GtkWidget *label_scroll_pos;

    table = gtk_table_new (3, 4, FALSE);

    items.combo_scrollbar_position = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_scrollbar_position), "RIGHT");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_scrollbar_position), "LEFT");
    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_scrollbar_position), cfg_getint (tw->tc, "scrollbar_pos"));

    label_scrollback = gtk_label_new("Scrollback:");
    label_scroll_pos = gtk_label_new("Scrollbar is:");
    items.check_show_scrollbar = gtk_check_button_new_with_label ("Show scrollbar");
    items.check_scroll_on_keystroke = gtk_check_button_new_with_label ("Scroll on keystroke");
    items.check_scroll_background = gtk_check_button_new_with_label ("Scroll Background");
    items.check_scroll_on_output = gtk_check_button_new_with_label ("Scroll on Output");

    items.spin_scrollback_amount = gtk_spin_button_new_with_range (0, INT_MAX, 1);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_show_scrollbar), cfg_getbool (tw->tc, "scrollbar"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_scroll_on_keystroke), cfg_getbool (tw->tc, "scroll_on_key"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_scroll_background), cfg_getbool (tw->tc, "scroll_background"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (items.check_scroll_on_output), cfg_getbool (tw->tc, "scroll_on_output"));

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (items.spin_scrollback_amount), cfg_getint (tw->tc, "lines"));

    gtk_table_attach (GTK_TABLE (table), items.check_show_scrollbar, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.check_scroll_on_keystroke, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), items.check_scroll_on_output, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.check_scroll_background, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_scroll_pos, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.combo_scrollbar_position,  1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_scrollback, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.spin_scrollback_amount,  1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show_all (table);

    return table;
}

static void revert_default_compatability (tilda_window *tw)
{
#ifdef DEBUG
    puts("revert_default_compatability");
#endif

    cfg_setint (tw->tc, "backspace_key", 0);
    cfg_setint (tw->tc, "delete_key", 1);

    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_backspace_key), cfg_getint (tw->tc, "backspace_key"));
    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_delete_key), cfg_getint (tw->tc, "delete_key"));
}

static GtkWidget* compatibility (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("compatibility");
#endif

    GtkWidget *table;
    GtkWidget *label_backspace;
    GtkWidget *label_delete;
    GtkWidget *button_defaults;

    table = gtk_table_new (2, 3, FALSE);

    label_backspace = gtk_label_new ("Backspace key generates: ");

    items.combo_backspace_key = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_backspace_key), "Control-H");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_backspace_key), "Escape Sequence");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_backspace_key), "ASCII DEL");
    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_backspace_key), cfg_getint (tw->tc, "backspace_key"));

    label_delete = gtk_label_new ("Delete key generates: ");

    items.combo_delete_key = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_delete_key), "Control-H");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_delete_key), "Escape Sequence");
    gtk_combo_box_prepend_text (GTK_COMBO_BOX(items.combo_delete_key), "ASCII DEL");
    gtk_combo_box_set_active (GTK_COMBO_BOX(items.combo_delete_key), cfg_getint (tw->tc, "delete_key"));

    items.button_revert_to_defaults = gtk_button_new_with_label ("Revert to Defaults");
    g_signal_connect_swapped (G_OBJECT (items.button_revert_to_defaults), "clicked",
                  G_CALLBACK (revert_default_compatability), tw);

    gtk_table_attach (GTK_TABLE (table), label_backspace, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.combo_backspace_key, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_delete, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.combo_delete_key, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), items.button_revert_to_defaults, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show_all (table);

    return table;
}

static GtkWidget* keybindings (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("keybindings");
#endif

    GtkWidget *table;
    GtkWidget *label_key;
    GtkWidget *label_warning;
    gchar s_temp[128];

    if (!cfg_getstr (tw->tc, "key"))
    {
        g_snprintf (s_temp, sizeof(s_temp), "None+F%i", tw->instance+1);
        cfg_setstr (tw->tc, "key", s_temp);
    }

    table = gtk_table_new (2, 3, FALSE);

    items.entry_keybinding = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (items.entry_keybinding), cfg_getstr (tw->tc, "key"));

    label_key = gtk_label_new ("Key Bindings");
    label_warning = gtk_label_new ("Note: You must restart Tilda for the change in keybinding to take affect");

    gtk_table_attach (GTK_TABLE (table), label_key, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), items.entry_keybinding, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_warning, 0, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show_all (table);

    return table;
}

static void apply_settings (tilda_window *tw)
{
#ifdef DEBUG
    puts("apply_settings");
#endif

    GdkColor gdk_text_color, gdk_back_color;
    gchar *tmp_str;
    tilda_term *tt;
    guint i;

    cfg_setstr (tw->tc, "key", gtk_entry_get_text (GTK_ENTRY (items.entry_keybinding)));
    cfg_setstr (tw->tc, "font", gtk_font_button_get_font_name (GTK_FONT_BUTTON (items.button_font)));
    cfg_setstr (tw->tc, "title", gtk_entry_get_text (GTK_ENTRY (items.entry_title)));
    cfg_setstr (tw->tc, "command", gtk_entry_get_text (GTK_ENTRY (items.entry_custom_command)));
    cfg_setstr (tw->tc, "image", gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (items.chooser_background_image)));

    cfg_setint (tw->tc, "max_height", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (items.spin_height_pixels)));
    cfg_setint (tw->tc, "max_width", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (items.spin_width_pixels)));
    cfg_setint (tw->tc, "lines", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (items.spin_scrollback_amount)));
    cfg_setint (tw->tc, "transparency", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (items.spin_level_of_transparency)));
    cfg_setint (tw->tc, "x_pos", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (items.spin_x_position)));
    cfg_setint (tw->tc, "y_pos", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (items.spin_y_position)));

    cfg_setint (tw->tc, "tab_pos", gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_tab_position)));
    cfg_setint (tw->tc, "backspace_key", gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_backspace_key)));
    cfg_setint (tw->tc, "delete_key", gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_delete_key)));
    cfg_setint (tw->tc, "d_set_title", gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_dynamically_set_title)));
    cfg_setint (tw->tc, "command_exit", gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_command_exit)));
    cfg_setint (tw->tc, "scheme", gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_schemes)));
    cfg_setint (tw->tc, "scrollbar_pos", gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_scrollbar_position)));
    cfg_setint (tw->tc, "slide_sleep_usec", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (items.spin_animation_delay)));
    cfg_setint (tw->tc, "animation_orientation", gtk_combo_box_get_active (GTK_COMBO_BOX(items.combo_orientation)));

    gtk_color_button_get_color (GTK_COLOR_BUTTON(items.button_text_color), &gdk_text_color);
    gtk_color_button_get_color (GTK_COLOR_BUTTON(items.button_background_color), &gdk_back_color);

    cfg_setint (tw->tc, "text_red", gdk_text_color.red);
    cfg_setint (tw->tc, "text_green", gdk_text_color.green);
    cfg_setint (tw->tc, "text_blue", gdk_text_color.blue);
    cfg_setint (tw->tc, "back_red", gdk_back_color.red);
    cfg_setint (tw->tc, "back_green", gdk_back_color.green);
    cfg_setint (tw->tc, "back_blue", gdk_back_color.blue);

    cfg_setbool (tw->tc, "notaskbar", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_do_not_show_in_taskbar)));
    cfg_setbool (tw->tc, "pinned", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_display_on_all_workspaces)));
    cfg_setbool (tw->tc, "above", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_always_on_top)));
    cfg_setbool (tw->tc, "antialias", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_enable_antialias)));
    cfg_setbool (tw->tc, "scrollbar", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_show_scrollbar)));
    cfg_setbool (tw->tc, "use_image", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_use_image_for_background)));
    cfg_setbool (tw->tc, "hidden", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_start_tilda_hidden)));
    cfg_setbool (tw->tc, "bold", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_allow_bold_text)));
    cfg_setbool (tw->tc, "blinks", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_cursor_blinks)));
    cfg_setbool (tw->tc, "bell", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_terminal_bell)));
    cfg_setbool (tw->tc, "run_command", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_run_custom_command)));
    cfg_setbool (tw->tc, "scroll_on_key", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_scroll_on_keystroke)));
    cfg_setbool (tw->tc, "scroll_on_output", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_scroll_on_output)));
    cfg_setbool (tw->tc, "scroll_background", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_scroll_background)));
    cfg_setbool (tw->tc, "notebook_border", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_show_notebook_border)));
    cfg_setbool (tw->tc, "animation", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_animated_pulldown)));
    cfg_setbool (tw->tc, "centered_horizontally", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_centered_horizontally)));
    cfg_setbool (tw->tc, "centered_vertically", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_centered_vertically)));
    cfg_setbool (tw->tc, "enable_transparency", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (items.check_enable_transparency)));

    generate_animation_positions (tw);

    write_config_file (tw);

    if (!in_main)
    {
        for (i=0;i<g_list_length(tw->terms);i++)
        {
            tt = (tilda_term *)g_list_nth_data (tw->terms, i);
            update_tilda (tw, tt, FALSE);
        }
    }
}

static gint ok (tilda_window *tw)
{
#ifdef DEBUG
    puts("ok");
#endif

    apply_settings (tw);
    exit_status = 0;

    if (in_main)
    {
        gtk_main_quit();
        in_main = FALSE;
    }

    return (TRUE);
}

static gint exit_app (GtkWidget *widget, gpointer data)
{
#ifdef DEBUG
    puts("exit_app");
#endif

    gtk_widget_destroy (items.wizard_window);

    if (in_main)
        gtk_main_quit();

    exit_status = 1;

    return (FALSE);
}

static void wizard_window_response_cb(GtkDialog* wizard_window,
                                      int response,
                                      tilda_window *tw)
{
    if (response == GTK_RESPONSE_OK) {
        ok (tw);
    }

}

int wizard (int argc, char **argv, tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("wizard");
#endif

    GtkWidget *button;
    GtkWidget *table;
    GtkWidget *notebook;
    GtkWidget *label;

    GtkWidget *table2;
    GtkWidget *image;
    GdkPixmap *image_pix;
    GdkBitmap *image_pix_mask;
    GtkStyle   *style;
    gchar *argv0 = NULL;
    gchar title[20];

    GtkWidget *contents[7];
    gchar *tabs[] = {"General", "Title and Command", "Appearance", "Colors", "Scrolling", "Compatibility", "Keybindings"};

    gint i;

    contents[0] = general (tw, tt);
    contents[1] = title_command (tw, tt);
    contents[2] = appearance (tw, tt);
    contents[3] = colors (tw, tt);
    contents[4] = scrolling (tw, tt);
    contents[5] = compatibility (tw, tt);
    contents[6] = keybindings (tw, tt);

    if (argv != NULL)
        argv0 = argv[0];

    if (argc != -1)
    {
        /* READ CONFIG HERE */

        in_main = TRUE;
        gtk_init (&argc, &argv);
    }

    items.wizard_window = gtk_dialog_new_with_buttons (
      "Configure Tilda",
      NULL,
      0,
      GTK_STOCK_CANCEL,
      GTK_RESPONSE_CANCEL,
      GTK_STOCK_OK,
      GTK_RESPONSE_OK,
      NULL);

    gtk_dialog_set_default_response (GTK_DIALOG (items.wizard_window), GTK_RESPONSE_OK);

    /*gtk_window_set_resizable (GTK_WINDOW(wizard_window), FALSE);*/
    /*gtk_window_set_position(GTK_WINDOW(wizard_window), GTK_WIN_POS_CENTER);*/
    /*gtk_widget_realize (wizard_window);*/

    g_snprintf (title, sizeof(title), "Tilda %i Config", tw->instance);
    gtk_window_set_title (GTK_WINDOW (items.wizard_window), title);

    /*g_signal_connect (G_OBJECT (wizard_window), "delete_event",
                  G_CALLBACK (exit_app), NULL);*/

    gtk_container_set_border_width (GTK_CONTAINER (items.wizard_window), 10);

    table = gtk_table_new (3, 3, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 10);
    gtk_table_set_col_spacings (GTK_TABLE (table), 10);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (items.wizard_window)->vbox), GTK_WIDGET (table), TRUE, TRUE, 7);

    /* Create a new notebook, place the position of the tabs */
    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_table_attach_defaults (GTK_TABLE (table), notebook, 0, 3, 1, 2);
    gtk_widget_show (notebook);

    /* Let's append a bunch of pages to the notebook */
    for (i = 0; i < 7; i++)
    {
        label = gtk_label_new (tabs[i]);
        gtk_notebook_append_page (GTK_NOTEBOOK(notebook), contents[i], label);
        /*
        table2 = (*contents[i])(tw, tt);

        gtk_widget_show (table2);
        label = gtk_label_new (tabs[i]);
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table2, label);
        */
    }

    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);


    g_signal_connect (G_OBJECT (items.wizard_window), "response",
      G_CALLBACK (wizard_window_response_cb), tw);

    gtk_widget_show (notebook);
    gtk_widget_show (table);
    /*gtk_widget_show (wizard_window);*/

    exit_status = gtk_dialog_run (GTK_DIALOG (items.wizard_window)) != GTK_RESPONSE_OK;

    gtk_widget_destroy (GTK_WIDGET (items.wizard_window));

    /*if (in_main)
        gtk_main ();*/

    return exit_status;
}

