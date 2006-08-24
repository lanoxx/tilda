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

#ifndef WIZARD_H
#define WIZARD_H

#include <gtk/gtk.h>
#include <confuse.h>

G_BEGIN_DECLS;

/* This struct will hold things that we need in all the functions in
 * wizard.c.
 *
 * NOTE: This only needs to hold things that we need to get the value
 * of later, not things like labels that we never need to get the value
 * from.
 */
struct wizard_items
{
    GtkWidget *wizard_window;

    /* General Notebook Tab */
    GtkWidget *check_display_on_all_workspaces;
    GtkWidget *check_always_on_top;
    GtkWidget *check_do_not_show_in_taskbar;
    GtkWidget *check_start_tilda_hidden;
    GtkWidget *check_terminal_bell;
    GtkWidget *check_cursor_blinks;
    GtkWidget *check_enable_antialias;
    GtkWidget *check_allow_bold_text;
    GtkWidget *check_show_notebook_border;
    GtkWidget *combo_tab_position;
    GtkWidget *button_font;

    /* Title and Command Notebook Tab */
    GtkWidget *entry_title;
    GtkWidget *combo_dynamically_set_title;
    GtkWidget *check_run_custom_command;
    GtkWidget *entry_custom_command;
    GtkWidget *combo_command_exit;

    /* Appearance Notebook Tab */
    GtkWidget *check_centered_vertically;
    GtkWidget *spin_height_percentage;
    GtkWidget *spin_height_pixels;
    GtkWidget *check_centered_horizontally;
    GtkWidget *spin_width_percentage;
    GtkWidget *spin_width_pixels;
    GtkWidget *spin_x_position;
    GtkWidget *spin_y_position;
    GtkWidget *spin_level_of_transparency;
    GtkWidget *check_animated_pulldown;
    GtkWidget *spin_animation_delay;
    GtkWidget *combo_orientation;
    GtkWidget *check_use_image_for_background;
    GtkWidget *button_background_image;
    GtkWidget *chooser_background_image;

    gulong    cid_height_pixels;
    gulong    cid_height_percentage;
    gulong    cid_width_pixels;
    gulong    cid_width_percentage;

    /* Colors Notebook Tab */
    GtkWidget *combo_schemes;
    GtkWidget *button_text_color;
    GtkWidget *button_background_color;

    /* Scrolling Notebook Tab */
    GtkWidget *check_show_scrollbar;
    GtkWidget *check_scroll_on_keystroke;
    GtkWidget *check_scroll_on_output;
    GtkWidget *check_scroll_background;
    GtkWidget *combo_scrollbar_position;
    GtkWidget *spin_scrollback_amount;

    /* Compatibility Notebook Tab */
    GtkWidget *combo_backspace_key;
    GtkWidget *combo_delete_key;
    GtkWidget *button_revert_to_defaults;

    /* Keybindings Notebook Tab */
    GtkWidget *entry_keybinding;
};

int wizard (int argc, char **argv, tilda_window *tw, tilda_term *tt);

G_END_DECLS;

#endif
