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

/* strstr() and friends */
#include <string.h>

#include "tilda.h"
#include "../tilda-config.h"
#include "config.h"
#include "wizard.h"
#include "load_tilda.h"

#define MAX_INT 2147483647

int exit_status=0;

GtkWidget *wizard_window;
GtkWidget *check_notebook_border, *spin_slide_sleep_usec;
GtkWidget *check_scroll_background, *check_scroll_on_output;
GtkWidget *entry_title, *combo_set_title, *check_run_command, *entry_command, *combo_command_exit;
GtkWidget *check_allow_bold, *check_cursor_blinks, *check_terminal_bell;
GtkWidget *text_color, *back_color, *combo_schemes;
GtkWidget *entry_height, *entry_width, *entry_x_pos, *entry_y_pos, *entry_key;
GtkWidget *button_image, *image_chooser, *combo_tab_pos;
GtkWidget *check_xbindkeys, *check_antialias, *check_use_image;
GtkWidget *check_pinned, *check_above, *check_notaskbar, *check_scrollbar, *check_down;
GtkWidget *slider_opacity, *spin_scrollback;
GtkWidget *combo_backspace, *combo_delete;
GtkWidget *button_font;
GtkWidget *combo_scroll_pos, *check_scroll_on_keystroke;
GtkWidget *check_animation;
GtkWidget *label_tab_orientation, *combo_tab_orientation;

gboolean in_main = FALSE;

void close_dialog (GtkWidget *widget, gpointer data)
{
#ifdef DEBUG
    puts("close_dialog");
#endif

    gtk_grab_remove (GTK_WIDGET (widget));
    gtk_widget_destroy (GTK_WIDGET (data));
}

GtkWidget* general (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("general");
#endif

    GtkWidget *table;
    GtkWidget *label_tab_pos;

    table = gtk_table_new (2, 7, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);

    button_font = gtk_font_button_new_with_font (cfg_getstr(tw->tc, "font"));

    check_antialias = gtk_check_button_new_with_label ("Enable anti-aliasing");

    if (cfg_getbool(tw->tc, "antialias"))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_antialias), TRUE);

    label_tab_pos = gtk_label_new ("Position of Tabs: ");

    combo_tab_pos = gtk_combo_box_new_text    ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_pos, "RIGHT");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_pos, "LEFT");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_pos, "BOTTOM");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_pos, "TOP");
    gtk_combo_box_set_active ((GtkComboBox *)combo_tab_pos, cfg_getint (tw->tc, "tab_pos"));

    check_pinned = gtk_check_button_new_with_label ("Display on all workspaces");
    check_above = gtk_check_button_new_with_label ("Always on top");
    check_notaskbar = gtk_check_button_new_with_label ("Do not show in taskbar");
    check_notebook_border = gtk_check_button_new_with_label ("Show Notebook Border");

    check_down = gtk_check_button_new_with_label ("Display pulled down on start");

    check_allow_bold = gtk_check_button_new_with_label ("Allow bold text");
    check_cursor_blinks = gtk_check_button_new_with_label ("Cursor blinks");
    check_terminal_bell = gtk_check_button_new_with_label ("Terminal Bell");


    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_pinned),         cfg_getbool (tw->tc, "pinned"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_above),          cfg_getbool (tw->tc, "above"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_notaskbar),      cfg_getbool (tw->tc, "notaskbar"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_down),           cfg_getbool (tw->tc, "down"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_terminal_bell),  cfg_getbool (tw->tc, "bell"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_cursor_blinks),  cfg_getbool (tw->tc, "blinks"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_antialias),      cfg_getbool (tw->tc, "antialias"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_allow_bold),     cfg_getbool (tw->tc, "bold"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_notebook_border), cfg_getbool (tw->tc, "notebook_border"));

    gtk_table_attach (GTK_TABLE (table), check_pinned, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_above,  1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_notaskbar, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_down, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_terminal_bell, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_cursor_blinks, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_antialias, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_allow_bold, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_notebook_border, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_tab_pos, 0, 1, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_tab_pos, 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), button_font, 0, 2, 6, 7, GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show (check_notebook_border);
    gtk_widget_show (label_tab_pos);
    gtk_widget_show (combo_tab_pos);
    gtk_widget_show (check_down);
    gtk_widget_show (check_pinned);
    gtk_widget_show (check_above);
    gtk_widget_show (check_notaskbar);
    gtk_widget_show (button_font);
    gtk_widget_show (check_antialias);
    gtk_widget_show (check_allow_bold);
    gtk_widget_show (check_cursor_blinks);
    gtk_widget_show (check_terminal_bell);

    return table;
}

GtkWidget* title_command (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("title_command");
#endif

    GtkWidget *table;
    GtkWidget *label_title, *label_command;
    GtkWidget *label_initial_title, *label_dynamically_set_title_pos;
    GtkWidget *label_custom_command, *label_on_command_exit;
    gchar *markup, *second_markup;

    table = gtk_table_new (2, 8, FALSE);

    label_title = gtk_label_new ("");
    markup = g_markup_printf_escaped ("<b>%s</b>", "Title:");
    gtk_label_set_markup (GTK_LABEL (label_title), markup);
    gtk_misc_set_alignment ((GtkMisc *)label_title, 0, 0);
    label_command = gtk_label_new ("");
    second_markup = g_markup_printf_escaped ("<b>%s</b>", "Command:");
    gtk_label_set_markup (GTK_LABEL (label_command), second_markup);
    gtk_misc_set_alignment ((GtkMisc *)label_command, 0, 0);
    label_initial_title = gtk_label_new ("Initial title:");
    gtk_misc_set_alignment ((GtkMisc *)label_initial_title, 0, 0);
    label_dynamically_set_title_pos = gtk_label_new ("Dynamically-set title:");
    gtk_misc_set_alignment ((GtkMisc *)label_dynamically_set_title_pos, 0, 0);
    label_custom_command = gtk_label_new ("Custom command:");
    gtk_misc_set_alignment ((GtkMisc *)label_custom_command, 0, 0);
    label_on_command_exit = gtk_label_new ("When command exit:");
    gtk_misc_set_alignment ((GtkMisc *)label_on_command_exit, 0, 0);

    entry_title = gtk_entry_new ();
    entry_command = gtk_entry_new ();

    check_run_command = gtk_check_button_new_with_label ("Run a custom command instead of shell");

    combo_set_title = gtk_combo_box_new_text    ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_set_title, "Replace initial title");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_set_title, "Goes before initial title");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_set_title, "Goes after initial title");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_set_title, "Isn't displayed");
    gtk_combo_box_set_active ((GtkComboBox *)combo_set_title, cfg_getint (tw->tc, "d_set_title"));

    combo_command_exit = gtk_combo_box_new_text    ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_command_exit, "Exit the terminal");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_command_exit, "Restart the command");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_command_exit, "Hold the terminal open");
    gtk_combo_box_set_active ((GtkComboBox *)combo_command_exit, cfg_getint(tw->tc, "command_exit"));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_run_command), cfg_getbool (tw->tc, "run_command"));

    gtk_entry_set_text (GTK_ENTRY (entry_title), cfg_getstr (tw->tc, "title"));
    gtk_entry_set_text (GTK_ENTRY (entry_command), cfg_getstr (tw->tc, "command"));

    gtk_table_attach (GTK_TABLE (table), label_title, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_initial_title, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_title, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_dynamically_set_title_pos, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_set_title, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_command, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_run_command, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_custom_command, 0, 1, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_command, 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_on_command_exit, 0, 1, 6, 7, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_command_exit, 1, 2, 6, 7, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show (label_title);
    gtk_widget_show (label_initial_title);
    gtk_widget_show (entry_title);
    gtk_widget_show (label_dynamically_set_title_pos);
    gtk_widget_show (combo_set_title);
    gtk_widget_show (label_command);
    gtk_widget_show (check_run_command);
    gtk_widget_show (label_custom_command);
    gtk_widget_show (entry_command);
    gtk_widget_show (label_on_command_exit);
    gtk_widget_show (combo_command_exit);

    g_free (markup);
    g_free (second_markup);

    return table;
}


void image_select (GtkWidget *widget, GtkWidget *label_image)
{
#ifdef DEBUG
    puts("image_select");
#endif

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_use_image)) == TRUE)
    {
        gtk_widget_set_sensitive ((GtkWidget *) label_image, TRUE);
        gtk_widget_set_sensitive ((GtkWidget *) button_image, TRUE);
    }
    else
    {
        gtk_widget_set_sensitive ((GtkWidget *) label_image, FALSE);
        gtk_widget_set_sensitive ((GtkWidget *) button_image, FALSE);
    }
}

GtkWidget* appearance (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("appearance");
#endif

    GtkWidget *table;
    GtkWidget *label_image;
    GtkWidget *label_opacity;
    GtkWidget *label_height, *label_width;
    GtkWidget *label_x_pos, *label_y_pos;
    GtkWidget *label_slide_sleep_usec;
 
    char s_max_height[6], s_max_width[6];
    char s_x_pos[6], s_y_pos[6];

    table = gtk_table_new (3, 9, FALSE);
    label_height = gtk_label_new ("Height in Pixels:");
    label_width = gtk_label_new ("Width in Pixels:");
    label_x_pos = gtk_label_new ("X Pixel Postion to Start:");
    label_y_pos = gtk_label_new ("Y Pixel Postion to Start:");
    label_opacity = gtk_label_new ("Level of Transparency:");
    label_image = gtk_label_new ("Background Image:");

    entry_height = gtk_entry_new ();
    entry_width = gtk_entry_new ();
    entry_x_pos = gtk_entry_new ();
    entry_y_pos = gtk_entry_new ();
    slider_opacity = gtk_hscale_new_with_range (0, 100, 1);

    sprintf (s_max_height, "%d", cfg_getint (tw->tc, "max_height"));
    sprintf (s_max_width, "%d",  cfg_getint (tw->tc, "max_width"));
    sprintf (s_x_pos, "%d",      cfg_getint (tw->tc, "x_pos"));
    sprintf (s_y_pos, "%d",      cfg_getint (tw->tc, "y_pos"));
    gtk_entry_set_text (GTK_ENTRY (entry_height), s_max_height);
    gtk_entry_set_text (GTK_ENTRY (entry_width), s_max_width);
    gtk_entry_set_text (GTK_ENTRY (entry_x_pos), s_x_pos);
    gtk_entry_set_text (GTK_ENTRY (entry_y_pos), s_y_pos);
    gtk_range_set_value (GTK_RANGE (slider_opacity), cfg_getint (tw->tc, "transparency"));
    
    label_slide_sleep_usec = gtk_label_new ("Animation Delay (in usecs):");
    spin_slide_sleep_usec = gtk_spin_button_new_with_range (0, MAX_INT, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_slide_sleep_usec), cfg_getint (tw->tc, "slide_sleep_usec"));

    label_tab_orientation = gtk_label_new("Orientation:");
    combo_tab_orientation = gtk_combo_box_new_text    ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_orientation, "RIGHT");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_orientation, "LEFT");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_orientation, "BOTTOM");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_orientation, "TOP");
    gtk_combo_box_set_active ((GtkComboBox *)combo_tab_orientation, cfg_getint (tw->tc, "animation_orientation"));
    
    check_use_image = gtk_check_button_new_with_label ("Use Image for Background");
    check_animation = gtk_check_button_new_with_label ("Animated Pulldown");

    image_chooser = gtk_file_chooser_dialog_new ("Open Background Image File",
                      GTK_WINDOW (wizard_window),
                      GTK_FILE_CHOOSER_ACTION_OPEN,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                      NULL);
    button_image = gtk_file_chooser_button_new_with_dialog (image_chooser);

    if (cfg_getbool (tw->tc, "use_image"))
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_use_image), TRUE);
        gtk_widget_set_sensitive ((GtkWidget *) label_image, TRUE);
        gtk_widget_set_sensitive ((GtkWidget *) button_image, TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_use_image), FALSE);
        gtk_widget_set_sensitive ((GtkWidget *) label_image, FALSE);
        gtk_widget_set_sensitive ((GtkWidget *) button_image, FALSE);
    }

    if (cfg_getbool (tw->tc, "animation"))
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_animation), TRUE);
    }

    gtk_signal_connect (GTK_OBJECT (check_use_image), "clicked", GTK_SIGNAL_FUNC(image_select), label_image);

    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);

    gtk_table_attach (GTK_TABLE (table), label_height, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_height, 1, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_width, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_width, 1, 3, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_x_pos, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_x_pos, 1, 3, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_y_pos, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_y_pos, 1, 3, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_opacity,  0, 1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), slider_opacity, 1, 3, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_animation, 0, 1, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), label_slide_sleep_usec, 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), spin_slide_sleep_usec,  2, 3, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), label_tab_orientation, 1, 2, 6, 7, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_tab_orientation,  2, 3, 6, 7, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_use_image, 0, 3, 7, 8, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_image,  0, 1, 8, 9, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), button_image, 1, 3, 8, 9, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);

    gtk_widget_show (entry_height);
    gtk_widget_show (label_height);
    gtk_widget_show (entry_width);
    gtk_widget_show (label_width);
    gtk_widget_show (label_opacity);
    gtk_widget_show (slider_opacity);
    gtk_widget_show (entry_x_pos);
    gtk_widget_show (label_x_pos);
    gtk_widget_show (entry_y_pos);
    gtk_widget_show (label_y_pos);
    gtk_widget_show (check_animation);
    gtk_widget_show (label_slide_sleep_usec);
    gtk_widget_show (spin_slide_sleep_usec);
    gtk_widget_show (check_use_image);
    gtk_widget_show (label_image);
    gtk_widget_show (button_image);
    gtk_widget_show (label_tab_orientation);
    gtk_widget_show (combo_tab_orientation);

    return table;
}

void scheme_colors_changed (tilda_window *tw)
{
#ifdef DEBUG
    puts("scheme_colors_changed");
#endif

    GdkColor gdk_text, gdk_back;

    gint scheme = gtk_combo_box_get_active ((GtkComboBox *) combo_schemes);

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

    gtk_color_button_set_color ((GtkColorButton *) text_color, &gdk_text);
    gtk_color_button_set_color ((GtkColorButton *) back_color, &gdk_back);
}

void button_colors_changed (tilda_window *tw)
{
#ifdef DEBUG
    puts("button_colors_changed");
#endif

    gtk_combo_box_set_active ((GtkComboBox *)combo_schemes, 0);
}

GtkWidget* colors (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("colors");
#endif

    GtkWidget *table;
    GtkWidget *label_color, *label_builtin, *label_text_color, *label_back_color;
    GdkColor text, back;
    gchar *markup;

    table = gtk_table_new (4, 4, FALSE);

    label_color = gtk_label_new ("");
    markup = g_markup_printf_escaped ("<b>%s</b>", "Foreground and Background Colors:");
    gtk_label_set_markup (GTK_LABEL (label_color), markup);
    label_builtin = gtk_label_new ("Built-in schemes:");
    label_text_color = gtk_label_new ("Text Color:");
    label_back_color = gtk_label_new ("Background Color:");

    combo_schemes = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_schemes, "White on Black");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_schemes, "Black on White");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_schemes, "Green on Black");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_schemes, "Custom");
    gtk_combo_box_set_active ((GtkComboBox *)combo_schemes, cfg_getint (tw->tc, "scheme"));

    text.red =      cfg_getint (tw->tc, "text_red");
    text.green =    cfg_getint (tw->tc, "text_green");
    text.blue =     cfg_getint (tw->tc, "text_blue");
    back.red =      cfg_getint (tw->tc, "back_red");
    back.green =    cfg_getint (tw->tc, "back_green");
    back.blue =     cfg_getint (tw->tc, "back_blue");

    text_color = gtk_color_button_new_with_color (&text);
    back_color = gtk_color_button_new_with_color (&back);

    g_signal_connect_swapped (G_OBJECT (combo_schemes), "changed",
                  G_CALLBACK (scheme_colors_changed), tw);
    g_signal_connect_swapped (G_OBJECT (text_color), "color-set",
                  G_CALLBACK (button_colors_changed), tw);
    g_signal_connect_swapped (G_OBJECT (back_color), "color-set",
                  G_CALLBACK (button_colors_changed), tw);

    gtk_table_attach (GTK_TABLE (table), label_color, 0, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_builtin, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_schemes, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_text_color, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), text_color, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), label_back_color, 2, 3, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), back_color, 3, 4, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show (label_builtin);
    gtk_widget_show (combo_schemes);
    gtk_widget_show (label_color);
    gtk_widget_show (label_text_color);
    gtk_widget_show (text_color);
    gtk_widget_show (label_back_color);
    gtk_widget_show (back_color);

    g_free (markup);

    return table;
}

GtkWidget* scrolling (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("scrolling");
#endif

    GtkWidget *table;
    GtkWidget *label_scrollback, *label_scroll_pos;

    table = gtk_table_new (3, 4, FALSE);

    combo_scroll_pos = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_scroll_pos, "RIGHT");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_scroll_pos, "LEFT");
    gtk_combo_box_set_active ((GtkComboBox *)combo_scroll_pos, cfg_getint (tw->tc, "scrollbar_pos"));

    label_scrollback = gtk_label_new("Scrollback:");
    label_scroll_pos = gtk_label_new("Scrollbar is:");
    check_scrollbar = gtk_check_button_new_with_label ("Show scrollbar");
    check_scroll_on_keystroke = gtk_check_button_new_with_label ("Scroll on keystroke");
    check_scroll_background = gtk_check_button_new_with_label ("Scroll Background");
    check_scroll_on_output = gtk_check_button_new_with_label ("Scroll on Output");

    spin_scrollback = gtk_spin_button_new_with_range (0, MAX_INT, 1);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_scrollbar), cfg_getbool (tw->tc, "scrollbar"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_scroll_on_keystroke), cfg_getbool (tw->tc, "scroll_on_key"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_scroll_background), cfg_getbool (tw->tc, "scroll_background"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_scroll_on_output), cfg_getbool (tw->tc, "scroll_on_output"));

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_scrollback), cfg_getint (tw->tc, "lines"));

    gtk_table_attach (GTK_TABLE (table), check_scrollbar, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_scroll_on_keystroke, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_scroll_on_output, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_scroll_background, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_scroll_pos, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_scroll_pos,  1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_scrollback, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), spin_scrollback,  1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show (check_scroll_on_output);
    gtk_widget_show (check_scroll_background);
    gtk_widget_show (label_scroll_pos);
    gtk_widget_show (check_scroll_on_keystroke);
    gtk_widget_show (combo_scroll_pos);
    gtk_widget_show (check_scrollbar);
    gtk_widget_show (label_scrollback);
    gtk_widget_show (spin_scrollback);

    return table;
}

void revert_default_compatability (tilda_window *tw)
{
#ifdef DEBUG
    puts("revert_default_compatability");
#endif

    cfg_setint (tw->tc, "backspace_key", 0);
    cfg_setint (tw->tc, "delete_key", 1);

    gtk_combo_box_set_active ((GtkComboBox *)combo_backspace, cfg_getint (tw->tc, "backspace_key"));
    gtk_combo_box_set_active ((GtkComboBox *)combo_delete, cfg_getint (tw->tc, "delete_key"));
}

GtkWidget* compatibility (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("compatibility");
#endif

    GtkWidget *table;
    GtkWidget *label_backspace, *label_delete;
    GtkWidget *button_defaults;

    table = gtk_table_new (2, 3, FALSE);

    label_backspace = gtk_label_new ("Backspace key generates: ");

    combo_backspace = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_backspace, "Control-H");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_backspace, "Escape Sequence");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_backspace, "ASCII DEL");
    gtk_combo_box_set_active ((GtkComboBox *)combo_backspace, cfg_getint (tw->tc, "backspace_key"));

    label_delete = gtk_label_new ("Delete key generates: ");

    combo_delete = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_delete, "Control-H");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_delete, "Escape Sequence");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_delete, "ASCII DEL");
    gtk_combo_box_set_active ((GtkComboBox *)combo_delete, cfg_getint (tw->tc, "delete_key"));

    button_defaults = gtk_button_new_with_label ("Revert to Defaults");
    g_signal_connect_swapped (G_OBJECT (button_defaults), "clicked",
                  G_CALLBACK (revert_default_compatability), tw);

    gtk_table_attach (GTK_TABLE (table), label_backspace, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_backspace, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_delete, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_delete, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), button_defaults, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show (label_backspace);
    gtk_widget_show (combo_backspace);
    gtk_widget_show (label_delete);
    gtk_widget_show (combo_delete);
    gtk_widget_show (button_defaults);

    return table;
}

GtkWidget* keybindings (tilda_window *tw, tilda_term *tt)
{
#ifdef DEBUG
    puts("keybindings");
#endif

    GtkWidget *table;
    GtkWidget *label_key, *label_warning;
    gchar s_temp[128];

    if (!cfg_getstr (tw->tc, "key"))
    {
        g_snprintf (s_temp, sizeof(s_temp), "None+F%i", tw->instance+1);
        cfg_setstr (tw->tc, "key", s_temp);
    }

    table = gtk_table_new (2, 3, FALSE);

    entry_key = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (entry_key), cfg_getstr (tw->tc, "key"));

    label_key = gtk_label_new ("Key Bindings");
    label_warning = gtk_label_new ("Note: You must restart Tilda for the change in keybinding to take affect");

    gtk_table_attach (GTK_TABLE (table), label_key, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_key, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_warning, 0, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show (label_key);
    gtk_widget_show (entry_key);
    gtk_widget_show (label_warning);

    return table;
}

void apply_settings (tilda_window *tw)
{
#ifdef DEBUG
    puts("apply_settings");
#endif

    GdkColor gdk_text_color, gdk_back_color;
    FILE *fp;
    gchar *tmp_str;
    tilda_term *tt;
    guint i;

    cfg_setstr (tw->tc, "key", gtk_entry_get_text (GTK_ENTRY (entry_key)));
    cfg_setstr (tw->tc, "font", gtk_font_button_get_font_name (GTK_FONT_BUTTON (button_font)));
    cfg_setstr (tw->tc, "title", gtk_entry_get_text (GTK_ENTRY (entry_title)));
    cfg_setstr (tw->tc, "command", gtk_entry_get_text (GTK_ENTRY (entry_command)));
    cfg_setstr (tw->tc, "image", gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (image_chooser)));

    old_max_height = cfg_getint (tw->tc, "max_height");
    old_max_width =  cfg_getint (tw->tc, "max_width");

    cfg_setint (tw->tc, "max_height", atoi (gtk_entry_get_text (GTK_ENTRY (entry_height))));
    cfg_setint (tw->tc, "max_width", atoi (gtk_entry_get_text (GTK_ENTRY (entry_width))));
    cfg_setint (tw->tc, "lines", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin_scrollback)));
    cfg_setint (tw->tc, "transparency", gtk_range_get_value (GTK_RANGE (slider_opacity)));
    cfg_setint (tw->tc, "x_pos", atoi (gtk_entry_get_text (GTK_ENTRY (entry_x_pos))));
    cfg_setint (tw->tc, "y_pos", atoi (gtk_entry_get_text (GTK_ENTRY (entry_y_pos))));

    cfg_setint (tw->tc, "tab_pos", gtk_combo_box_get_active ((GtkComboBox *) combo_tab_pos));
    cfg_setint (tw->tc, "backspace_key", gtk_combo_box_get_active ((GtkComboBox *) combo_backspace));
    cfg_setint (tw->tc, "delete_key", gtk_combo_box_get_active ((GtkComboBox *) combo_delete));
    cfg_setint (tw->tc, "d_set_title", gtk_combo_box_get_active ((GtkComboBox *) combo_set_title));
    cfg_setint (tw->tc, "command_exit", gtk_combo_box_get_active ((GtkComboBox *) combo_command_exit));
    cfg_setint (tw->tc, "scheme", gtk_combo_box_get_active ((GtkComboBox *) combo_schemes));
    cfg_setint (tw->tc, "scrollbar_pos", gtk_combo_box_get_active ((GtkComboBox *) combo_scroll_pos));
    cfg_setint (tw->tc, "slide_sleep_usec", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin_slide_sleep_usec)));
    cfg_setint (tw->tc, "animation_orientation", gtk_combo_box_get_active ((GtkComboBox *) combo_tab_orientation));

    gtk_color_button_get_color ((GtkColorButton *) text_color, &gdk_text_color);
    gtk_color_button_get_color ((GtkColorButton *) back_color, &gdk_back_color);

    cfg_setint (tw->tc, "text_red", gdk_text_color.red);
    cfg_setint (tw->tc, "text_green", gdk_text_color.green);
    cfg_setint (tw->tc, "text_blue", gdk_text_color.blue);
    cfg_setint (tw->tc, "back_red", gdk_back_color.red);
    cfg_setint (tw->tc, "back_green", gdk_back_color.green);
    cfg_setint (tw->tc, "back_blue", gdk_back_color.blue);

    cfg_setbool (tw->tc, "notaskbar", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_notaskbar)));
    cfg_setbool (tw->tc, "pinned", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_pinned)));
    cfg_setbool (tw->tc, "above", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_above)));
    cfg_setbool (tw->tc, "antialias", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_antialias)));
    cfg_setbool (tw->tc, "scrollbar", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_scrollbar)));
    cfg_setbool (tw->tc, "use_image", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_use_image)));
    cfg_setbool (tw->tc, "down", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_down)));
    cfg_setbool (tw->tc, "bold", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_allow_bold)));
    cfg_setbool (tw->tc, "blinks", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_cursor_blinks)));
    cfg_setbool (tw->tc, "bell", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_terminal_bell)));
    cfg_setbool (tw->tc, "run_command", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_run_command)));
    cfg_setbool (tw->tc, "scroll_on_key", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_scroll_on_keystroke)));
    cfg_setbool (tw->tc, "scroll_on_output", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_scroll_on_output)));
    cfg_setbool (tw->tc, "scroll_background", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_scroll_background)));
    cfg_setbool (tw->tc, "notebook_border", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_notebook_border)));
    cfg_setbool (tw->tc, "animation", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_animation)));

    /* Write out the config file */
    fp = fopen(tw->config_file, "w");
    cfg_print (tw->tc, fp);
    fclose (fp);

    if (!in_main)
    {
        for (i=0;i<g_list_length(tw->terms);i++)
        {
            tt = (tilda_term *)g_list_nth_data (tw->terms, i);
            update_tilda (tw, tt, FALSE);
        }
    }
}

gint ok (tilda_window *tw)
{
#ifdef DEBUG
    puts("ok");
#endif

    apply_settings (tw);
    exit_status = 0;
    gtk_widget_destroy (wizard_window);

    if (in_main)
    {
        gtk_main_quit();
        in_main = FALSE;
    }

    return (TRUE);
}

void apply (tilda_window *tw)
{
#ifdef DEBUG
    puts("apply");
#endif

    apply_settings (tw);
}

gint exit_app (GtkWidget *widget, gpointer data)
{
#ifdef DEBUG
    puts("exit_app");
#endif

    gtk_widget_destroy (wizard_window);

    if (in_main)
        gtk_main_quit();

    exit_status = 1;

    return (FALSE);
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
    gchar *tabs[] = {"General", "Title and Command", "Appearance", "Colors", "Scrolling", "Compatibility", "Keybindings"};

    FILE *fp;
    gint i;

    GtkWidget* (*contents[7])(tilda_window *, tilda_term *);

    contents[0] = general;
    contents[1] = title_command;
    contents[2] = appearance;
    contents[3] = colors;
    contents[4] = scrolling;
    contents[5] = compatibility;
    contents[6] = keybindings;

    if (argv != NULL)
        argv0 = argv[0];

    if (argc != -1)
    {
        /* READ CONFIG HERE */

        in_main = TRUE;
        gtk_init (&argc, &argv);
    }

    wizard_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable (GTK_WINDOW(wizard_window), FALSE);
    gtk_window_set_position(GTK_WINDOW(wizard_window), GTK_WIN_POS_CENTER);
    gtk_widget_realize (wizard_window);

    g_strlcpy (title, "Tilda", sizeof (title));
    sprintf (title, "%s %i Config", title, tw->instance);
    gtk_window_set_title (GTK_WINDOW (wizard_window), title);

    g_signal_connect (G_OBJECT (wizard_window), "delete_event",
                  G_CALLBACK (exit_app), NULL);

    gtk_container_set_border_width (GTK_CONTAINER (wizard_window), 10);

    table = gtk_table_new (3, 3, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 10);
    gtk_table_set_col_spacings (GTK_TABLE (table), 10);

    gtk_container_add (GTK_CONTAINER (wizard_window), table);

    /* Create a new notebook, place the position of the tabs */
    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_table_attach_defaults (GTK_TABLE (table), notebook, 0, 3, 1, 2);
    gtk_widget_show (notebook);

    /* Let's append a bunch of pages to the notebook */
    for (i = 0; i < 7; i++)
    {
        table2 = (*contents[i])(tw, tt);

        gtk_widget_show (table2);
        label = gtk_label_new (tabs[i]);
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table2, label);
    }

    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);

    button = gtk_button_new_with_label ("OK");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                  G_CALLBACK (ok), tw);
    gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 2, 3);
    gtk_widget_show (button);

    button = gtk_button_new_with_label ("Apply");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                  G_CALLBACK (apply), tw);
    gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 2, 3);
    gtk_widget_show (button);

    button = gtk_button_new_with_label ("Cancel");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                  G_CALLBACK (exit_app), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 2, 3);
    gtk_widget_show (button);

    gtk_widget_show (notebook);
    gtk_widget_show (table);
    gtk_widget_show (wizard_window);

    if (in_main)
        gtk_main ();

    return exit_status;
}

