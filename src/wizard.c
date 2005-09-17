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
#include "wizard_xpm.h"
#include "load_tilda.h"

#define MAX_INT 2147483647

GtkWidget *wizard_window;
int exit_status=0;

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
gboolean in_main = FALSE;

void close_dialog (GtkWidget *widget, gpointer data)
{
    gtk_grab_remove (GTK_WIDGET (widget));
    gtk_widget_destroy (GTK_WIDGET (data));
}

GtkWidget* general (tilda_window *tw, tilda_term *tt)
{
    GtkWidget *table;
    GtkWidget *label_tab_pos;

    table = gtk_table_new (2, 8, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);
       
    button_font = gtk_font_button_new_with_font (tw->tc->s_font);

    check_antialias = gtk_check_button_new_with_label ("Enable anti-aliasing");

    if (strcasecmp (tw->tc->s_antialias, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_antialias), TRUE);
    
    label_tab_pos = gtk_label_new ("Position of Tabs: ");

    combo_tab_pos = gtk_combo_box_new_text    ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_pos, "RIGHT");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_pos, "LEFT");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_pos, "BOTTOM");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_tab_pos, "TOP");
    gtk_combo_box_set_active ((GtkComboBox *)combo_tab_pos, tw->tc->tab_pos);

    check_pinned = gtk_check_button_new_with_label ("Display on all workspaces");
    check_above = gtk_check_button_new_with_label ("Always on top");
    check_notaskbar = gtk_check_button_new_with_label ("Do not show in taskbar");
    
    check_down = gtk_check_button_new_with_label ("Display pulled down on start");
    
    check_allow_bold = gtk_check_button_new_with_label ("Allow bold text");
    check_cursor_blinks = gtk_check_button_new_with_label ("Cursor blinks");
    check_terminal_bell = gtk_check_button_new_with_label ("Terminal Bell");

    if (strcasecmp (tw->tc->s_pinned, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_pinned), TRUE);

    if (strcasecmp (tw->tc->s_above, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_above), TRUE);

    if (strcasecmp (tw->tc->s_notaskbar, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_notaskbar), TRUE);

    if (strcasecmp (tw->tc->s_down, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_down), TRUE);


    gtk_table_attach (GTK_TABLE (table), check_pinned, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_above,  1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_notaskbar, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_down, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    gtk_table_attach (GTK_TABLE (table), check_terminal_bell, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_cursor_blinks, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_antialias, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_allow_bold, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
   

    gtk_table_attach (GTK_TABLE (table), label_tab_pos, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_tab_pos, 1, 2, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), button_font, 0, 2, 5, 6, GTK_FILL, GTK_FILL, 3, 3);

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
GtkWidget *entry_title, *combo_set_title, *check_run_command, *entry_command, *combo_command_exit;
GtkWidget* title_command (tilda_window *tw, tilda_term *tt)
{
    GtkWidget *table;
    GtkWidget *label_title, *label_command;
    GtkWidget *label_initial_title, *label_dynamically_set_title_pos;
    GtkWidget *label_custom_command, *label_on_command_exit;

    table = gtk_table_new (2, 8, FALSE);
    
    label_title = gtk_label_new ("Title:");
    gtk_misc_set_alignment ((GtkMisc *)label_title, 0, 0);
    label_command = gtk_label_new ("Command:");
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
    gtk_combo_box_set_active ((GtkComboBox *)combo_set_title, 0);// tw->tc->tab_pos);
    
    combo_command_exit = gtk_combo_box_new_text    ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_command_exit, "Exit the terminal");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_command_exit, "Restart the command");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_command_exit, "Hold the terminal open");
    gtk_combo_box_set_active ((GtkComboBox *)combo_command_exit, 0);// tw->tc->tab_pos);
    
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
    
    return table;
}


void image_select (GtkWidget *widget, GtkWidget *label_image)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_use_image)) == TRUE)
    {
        gtk_widget_show (label_image);
        gtk_widget_show (button_image);
    }
    else
    {
        gtk_widget_hide (label_image);
        gtk_widget_hide (button_image);
    }
}

GtkWidget* appearance (tilda_window *tw, tilda_term *tt)
{
    GtkWidget *table;
    GtkWidget *label_image;
    GtkWidget *label_opacity;
    GtkWidget *label_height, *label_width;
    GtkWidget *label_x_pos, *label_y_pos;

    char s_max_height[6], s_max_width[6];
    char s_x_pos[6], s_y_pos[6];

    table = gtk_table_new (3, 8, FALSE);
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

    sprintf (s_max_height, "%d", tw->tc->max_height);
    sprintf (s_max_width, "%d", tw->tc->max_width);
    sprintf (s_x_pos, "%d", tw->tc->x_pos);
    sprintf (s_y_pos, "%d", tw->tc->y_pos);
    gtk_entry_set_text (GTK_ENTRY (entry_height), s_max_height);
    gtk_entry_set_text (GTK_ENTRY (entry_width), s_max_width);
    gtk_entry_set_text (GTK_ENTRY (entry_x_pos), s_x_pos);
    gtk_entry_set_text (GTK_ENTRY (entry_y_pos), s_y_pos);
    gtk_range_set_value (GTK_RANGE (slider_opacity), tw->tc->transparency);

    check_use_image = gtk_check_button_new_with_label ("Use Image for Background");

    image_chooser = gtk_file_chooser_dialog_new ("Open Background Image File",
                      GTK_WINDOW (wizard_window),
                      GTK_FILE_CHOOSER_ACTION_OPEN,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                      NULL);
    button_image = gtk_file_chooser_button_new_with_dialog (image_chooser);

    if (QUICK_STRCMP (tw->tc->s_image, "none") != 0);

    if (strcasecmp (tw->tc->s_use_image, "TRUE") == 0)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_use_image), TRUE);
        gtk_widget_show (label_image);
        gtk_widget_show (button_image);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_use_image), FALSE);
        gtk_widget_hide (label_image);
        gtk_widget_hide (button_image);
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

    gtk_table_attach (GTK_TABLE (table), check_use_image, 0, 3, 5, 6, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_image,  0, 1, 6, 7, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), button_image, 1, 3, 6, 7, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);

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
    gtk_widget_show (check_use_image);

    return table;
}

void colors_changed (tilda_window *tw)
{
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

GtkWidget* colors (tilda_window *tw, tilda_term *tt)
{
    GtkWidget *table;
    GtkWidget *label_color, *label_builtin, *label_text_color, *label_back_color;
    GdkColor white, black;

    table = gtk_table_new (4, 4, FALSE);
    
    label_color = gtk_label_new ("Foreground and Background Colors:");
    label_builtin = gtk_label_new ("Built-in schemes:");
    label_text_color = gtk_label_new ("Text Color:");
    label_back_color = gtk_label_new ("Background Color:");
    
    combo_schemes = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_schemes, "White on Black");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_schemes, "Black on White");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_schemes, "Green on Black");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_schemes, "Custom");
    gtk_combo_box_set_active ((GtkComboBox *)combo_schemes, 0);//tw->tc->tab_pos);
    
    black.red = black.green = black.blue = 0x0000;
    white.red = white.green = white.blue = 0xffff;
    
    text_color = gtk_color_button_new_with_color (&black);
    back_color = gtk_color_button_new_with_color (&white);
    
    g_signal_connect_swapped (G_OBJECT (combo_schemes), "changed",
                  G_CALLBACK (colors_changed), tw);
    
    gtk_table_attach (GTK_TABLE (table), label_color, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
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
    
    return table;
}

GtkWidget* scrolling (tilda_window *tw, tilda_term *tt)
{
    GtkWidget *table;
    GtkWidget *label_scrollback, *label_scroll_pos;
    
    table = gtk_table_new (3, 8, FALSE);

    combo_scroll_pos = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_scroll_pos, "RIGHT");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_scroll_pos, "LEFT");
    gtk_combo_box_set_active ((GtkComboBox *)combo_scroll_pos, 0);//tw->tc->tab_pos);

    label_scrollback = gtk_label_new("Scrollback:");
    label_scroll_pos = gtk_label_new("Scrollbar is:");
    check_scrollbar = gtk_check_button_new_with_label ("Show scrollbar");
    check_scroll_on_keystroke = gtk_check_button_new_with_label ("Scroll on keystroke");
    
    spin_scrollback = gtk_spin_button_new_with_range (0, MAX_INT, 1);

    if (strcasecmp (tw->tc->s_scrollbar, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_scrollbar), TRUE);
        
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_scrollback), tw->tc->lines);
    
    gtk_table_attach (GTK_TABLE (table), check_scrollbar, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_scroll_on_keystroke, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_scroll_pos, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_scroll_pos,  1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_scrollback, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), spin_scrollback,  1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

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
    tw->tc->backspace_key = 0;
    tw->tc->delete_key = 1;
    
    gtk_combo_box_set_active ((GtkComboBox *)combo_backspace, tw->tc->backspace_key);
    gtk_combo_box_set_active ((GtkComboBox *)combo_delete, tw->tc->delete_key);    
}

GtkWidget* compatibility (tilda_window *tw, tilda_term *tt)
{
    GtkWidget *table;
    GtkWidget *label_backspace, *label_delete;
    GtkWidget *button_defaults;
    
    table = gtk_table_new (2, 3, FALSE);
    
    label_backspace = gtk_label_new ("Backspace key generates: ");

    combo_backspace = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_backspace, "Control-H");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_backspace, "Espace sequence");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_backspace, "ASCII DEL");
    gtk_combo_box_set_active ((GtkComboBox *)combo_backspace, tw->tc->backspace_key);

    label_delete = gtk_label_new ("Delete key generates: ");

    combo_delete = gtk_combo_box_new_text ();
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_delete, "Control-H");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_delete, "Espace sequence");
    gtk_combo_box_prepend_text ((GtkComboBox *)combo_delete, "ASCII DEL");
    gtk_combo_box_set_active ((GtkComboBox *)combo_delete, tw->tc->delete_key);
    
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
    GtkWidget *table;
    GtkWidget *label_key;

    if (strcasecmp (tw->tc->s_key, "null") == 0)
        sprintf (tw->tc->s_key, "None+F%i", tw->instance+1);

    table = gtk_table_new (2, 3, FALSE);

    entry_key = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (entry_key), tw->tc->s_key);

    label_key = gtk_label_new("Key Bindings");

    gtk_table_attach (GTK_TABLE (table), label_key, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_key, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show (label_key);
    gtk_widget_show (entry_key);

    return table;
}

/* Check that the string has some length, then write it to the config file.
 * Returns TRUE if a line was written, FALSE otherwise. */
int write_str_to_config (FILE *fp, const char *label, const char *value)
{
    if (strlen (value) > 0)
    {
        fprintf (fp, "%s : %s\n", label, value);
        return TRUE;
    }

    return FALSE;
}

/* This does not check integer values for anything, but it could be made to
 * check for non-negative values, etc. */
int write_int_to_config (FILE *fp, const char *label, const int value)
{
    fprintf (fp, "%s : %i\n", label, value);

    return TRUE;
}

/* Same as above, but for long's */
int write_lng_to_config (FILE *fp, const char *label, const long value)
{
    fprintf (fp, "%s : %ld\n", label, value);

    return TRUE;
}

/* Same as above, but for unsigned int's */
int write_uns_to_config (FILE *fp, const char *label, const unsigned int value)
{
    fprintf (fp, "%s : %u\n", label, value);

    return TRUE;
}

void apply_settings (tilda_window *tw)
{
    GdkColor gdk_text_color, gdk_back_color;
    FILE *fp;
    gchar *home_dir, *tmp_str;
    tilda_term *tt;
    gint i;

    g_strlcpy (tw->tc->s_key, gtk_entry_get_text (GTK_ENTRY (entry_key)), sizeof(tw->tc->s_key));
    g_strlcpy (tw->tc->s_font, gtk_font_button_get_font_name (GTK_FONT_BUTTON (button_font)), sizeof (tw->tc->s_font));
    g_strlcpy (tw->tc->s_title, gtk_entry_get_text (GTK_ENTRY (entry_title)), sizeof (tw->tc->s_title));
    g_strlcpy (tw->tc->s_command, gtk_entry_get_text (GTK_ENTRY (entry_command)), sizeof (tw->tc->s_command));
        
    if (NULL != (tmp_str = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (image_chooser))))
        g_strlcpy (tw->tc->s_image,  tmp_str, sizeof (tw->tc->s_image));

    old_max_height = tw->tc->max_height;
    old_max_width = tw->tc->max_width;
    tw->tc->max_height = atoi (gtk_entry_get_text (GTK_ENTRY (entry_height)));
    tw->tc->max_width = atoi (gtk_entry_get_text (GTK_ENTRY (entry_width)));
    tw->tc->lines = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin_scrollback));
    tw->tc->transparency = gtk_range_get_value (GTK_RANGE (slider_opacity));
    tw->tc->x_pos = atoi (gtk_entry_get_text (GTK_ENTRY (entry_x_pos)));
    tw->tc->y_pos = atoi (gtk_entry_get_text (GTK_ENTRY (entry_y_pos)));

    home_dir = getenv ("HOME");
    g_strlcpy (tw->config_file, home_dir, sizeof(tw->config_file));
    g_strlcat (tw->config_file, "/.tilda/", sizeof(tw->config_file));

    mkdir (tw->config_file,  S_IRUSR | S_IWUSR | S_IXUSR);

    g_strlcat (tw->config_file, "config", sizeof(tw->config_file));
    sprintf (tw->config_file, "%s_%i", tw->config_file, tw->instance);

    tw->tc->tab_pos = gtk_combo_box_get_active ((GtkComboBox *) combo_tab_pos);
    
    tw->tc->backspace_key = gtk_combo_box_get_active ((GtkComboBox *) combo_backspace);
    tw->tc->delete_key = gtk_combo_box_get_active ((GtkComboBox *) combo_delete);
    tw->tc->d_set_title = gtk_combo_box_get_active ((GtkComboBox *) combo_set_title);
    tw->tc->command_exit = gtk_combo_box_get_active ((GtkComboBox *) combo_command_exit);
    tw->tc->scheme = gtk_combo_box_get_active ((GtkComboBox *) combo_schemes);
    tw->tc->scrollbar_pos = gtk_combo_box_get_active ((GtkComboBox *) combo_scroll_pos);
    
    if (tw->tc->scheme != 0)
    {
        switch (tw->tc->scheme)
        {
            case 1:
                tw->tc->text_red = tw->tc->text_blue = 0x0000;
                tw->tc->text_green = 0xffff;
                tw->tc->back_red = tw->tc->back_green = tw->tc->back_blue = 0xffff;            
                break;
            case 2:
                tw->tc->text_red = tw->tc->text_green = tw->tc->text_blue = 0x0000;
                tw->tc->back_red = tw->tc->back_green = tw->tc->back_blue = 0xffff;
                break;
            case 3:
                tw->tc->text_red = tw->tc->text_green = tw->tc->text_blue = 0xffff;
                tw->tc->back_red = tw->tc->back_green = tw->tc->back_blue = 0x0000;
                break;
            default:
                break;   
        }
    }
    else {
        gtk_color_button_get_color ((GtkColorButton *) text_color, &gdk_text_color);
        gtk_color_button_get_color ((GtkColorButton *) back_color, &gdk_back_color);
    
        tw->tc->text_red = gdk_text_color.red;
        tw->tc->text_green = gdk_text_color.green;
        tw->tc->text_blue = gdk_text_color.blue;
        tw->tc->back_red = gdk_back_color.red;
        tw->tc->back_green = gdk_back_color.green;
        tw->tc->back_blue = gdk_back_color.blue;
    }
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_notaskbar)) == TRUE)
        g_strlcpy (tw->tc->s_notaskbar, "TRUE", sizeof(tw->tc->s_notaskbar));
    else
        g_strlcpy (tw->tc->s_notaskbar, "FALSE", sizeof(tw->tc->s_notaskbar));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_pinned)) == TRUE)
        g_strlcpy (tw->tc->s_pinned, "TRUE", sizeof(tw->tc->s_pinned));
    else
        g_strlcpy (tw->tc->s_pinned, "FALSE", sizeof(tw->tc->s_pinned));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_above)) == TRUE)
        g_strlcpy (tw->tc->s_above, "TRUE", sizeof(tw->tc->s_above));
    else
        g_strlcpy (tw->tc->s_above, "FALSE", sizeof(tw->tc->s_above));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_antialias)) == TRUE)
        g_strlcpy (tw->tc->s_antialias, "TRUE", sizeof(tw->tc->s_antialias));
    else
        g_strlcpy (tw->tc->s_antialias, "FALSE", sizeof(tw->tc->s_antialias));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_scrollbar)) == TRUE)
        g_strlcpy (tw->tc->s_scrollbar, "TRUE", sizeof(tw->tc->s_scrollbar));
    else
        g_strlcpy (tw->tc->s_scrollbar, "FALSE", sizeof(tw->tc->s_scrollbar));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_use_image)) == TRUE)
        g_strlcpy (tw->tc->s_use_image, "TRUE", sizeof(tw->tc->s_use_image));
    else
        g_strlcpy (tw->tc->s_use_image, "FALSE", sizeof(tw->tc->s_use_image));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_down)) == TRUE)
        g_strlcpy (tw->tc->s_down, "TRUE", sizeof(tw->tc->s_down));
    else
        g_strlcpy (tw->tc->s_down, "FALSE", sizeof(tw->tc->s_down));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_allow_bold)) == TRUE)
        g_strlcpy (tw->tc->s_bold, "TRUE", sizeof(tw->tc->s_bold));
    else
        g_strlcpy (tw->tc->s_bold, "FALSE", sizeof(tw->tc->s_bold));
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_cursor_blinks)) == TRUE)
        g_strlcpy (tw->tc->s_blinks, "TRUE", sizeof(tw->tc->s_blinks));
    else
        g_strlcpy (tw->tc->s_blinks, "FALSE", sizeof(tw->tc->s_blinks));
        
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_terminal_bell)) == TRUE)
        g_strlcpy (tw->tc->s_bell, "TRUE", sizeof(tw->tc->s_bell));
    else
        g_strlcpy (tw->tc->s_bell, "FALSE", sizeof(tw->tc->s_bell));
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_run_command)) == TRUE)
        g_strlcpy (tw->tc->s_run_command, "TRUE", sizeof(tw->tc->s_run_command));
    else
        g_strlcpy (tw->tc->s_run_command, "FALSE", sizeof(tw->tc->s_run_command));
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_scroll_on_keystroke)) == TRUE)
        g_strlcpy (tw->tc->s_scroll_on_key, "TRUE", sizeof(tw->tc->s_scroll_on_key));
    else
        g_strlcpy (tw->tc->s_scroll_on_key, "FALSE", sizeof(tw->tc->s_scroll_on_key));

    if((fp = fopen(tw->config_file, "w")) == NULL)
    {
        perror("fopen");
        exit(1);
    }
    else
    {
        write_int_to_config (fp, "max_height",      tw->tc->max_height);
        write_int_to_config (fp, "max_width",       tw->tc->max_width);
        write_int_to_config (fp, "min_height",      1);
        write_int_to_config (fp, "min_width",       tw->tc->max_width);
        write_str_to_config (fp, "notaskbar",       tw->tc->s_notaskbar);
        write_str_to_config (fp, "above",           tw->tc->s_above);
        write_str_to_config (fp, "pinned",          tw->tc->s_pinned);
        write_str_to_config (fp, "image",           tw->tc->s_image);
        write_str_to_config (fp, "background",      tw->tc->s_background);
        write_str_to_config (fp, "font",            tw->tc->s_font);
        write_str_to_config (fp, "antialias",       tw->tc->s_antialias);
        write_str_to_config (fp, "scrollbar",       tw->tc->s_scrollbar);
        write_int_to_config (fp, "transparency",    tw->tc->transparency);
        write_int_to_config (fp, "x_pos",           tw->tc->x_pos);
        write_int_to_config (fp, "y_pos",           tw->tc->y_pos);
        write_lng_to_config (fp, "scrollback",      tw->tc->lines);
        write_str_to_config (fp, "use_image",       tw->tc->s_use_image);
        write_str_to_config (fp, "grab_focus",      tw->tc->s_grab_focus);
        write_str_to_config (fp, "key",             tw->tc->s_key);
        write_str_to_config (fp, "down",            tw->tc->s_down);
        write_int_to_config (fp, "tab_pos",         tw->tc->tab_pos);
        write_int_to_config (fp, "backspace_key",   tw->tc->backspace_key);
        write_int_to_config (fp, "delete_key",      tw->tc->delete_key);
        write_str_to_config (fp, "title",           tw->tc->s_title);
        write_str_to_config (fp, "bold",            tw->tc->s_bold);
        write_str_to_config (fp, "blinks",          tw->tc->s_blinks);
        write_str_to_config (fp, "bell",            tw->tc->s_bell);
        write_int_to_config (fp, "d_set_title",     tw->tc->d_set_title);
        write_str_to_config (fp, "run_command",     tw->tc->s_run_command);
        write_str_to_config (fp, "command",         tw->tc->s_command);
        write_int_to_config (fp, "command_exit",    tw->tc->command_exit);
        write_int_to_config (fp, "scheme",          tw->tc->scheme);
        write_str_to_config (fp, "scroll_on_key",   tw->tc->s_scroll_on_key);
        write_int_to_config (fp, "scrollbar_pos",   tw->tc->scrollbar_pos);
        write_uns_to_config (fp, "back_red",        tw->tc->back_red);
        write_uns_to_config (fp, "back_green",      tw->tc->back_green);
        write_uns_to_config (fp, "back_blue",       tw->tc->back_blue);
        write_uns_to_config (fp, "text_red",        tw->tc->text_red);
        write_uns_to_config (fp, "text_green",      tw->tc->text_green);
        write_uns_to_config (fp, "text_blue",       tw->tc->text_blue);  
        
        fclose (fp);
    }

    if (!in_main)
    {
        for (i=0;i<g_list_length(tw->terms);i++)
        {
            tt = (tilda_term *)g_list_nth_data (tw->terms, i);
            update_tilda (tw, tt, FALSE);
        }
    }
}

gint ok (tilda_collect *tc)
{
    apply_settings (tc->tw);
    exit_status = 0;
    gtk_widget_destroy (wizard_window);

    if (in_main)
        gtk_main_quit();
    else
        free (tc);

    return (TRUE);
}

void apply (tilda_collect *tc)
{
    apply_settings (tc->tw);
}

gint exit_app (GtkWidget *widget, gpointer data, tilda_collect *tc)
{
    gtk_widget_destroy (wizard_window);
    
    if (in_main)
        gtk_main_quit();
    else
        //free (tc);

    exit_status = 1;

    return (FALSE);
}

int wizard (int argc, char **argv, tilda_window *tw, tilda_term *tt)
{
    GtkWidget *button;
    GtkWidget *table;
    GtkWidget *notebook;
    GtkWidget *label;
    GtkWidget *table2;
    GtkWidget *image;
    GdkPixmap *image_pix;
    GdkBitmap *image_pix_mask;
    GtkStyle   *style;
    tilda_collect *t_collect;
    gchar *argv0 = NULL;
    gchar title[20];
    gchar *tabs[] = {"General", "Title and Command", "Appearance", "Colors", "Scrolling", "Compatibility", "Keybindings"};

    GtkWidget* (*contents[7])(tilda_window *, tilda_term *);

    contents[0] = general;
    contents[1] = title_command; 
    contents[2] = appearance;
    contents[3] = colors;
    contents[4] = scrolling;
    contents[5] = compatibility;
    contents[6] = keybindings;

    FILE *fp;
    gint i;

    if (argv != NULL)
        argv0 = argv[0];

    t_collect = (tilda_collect *) malloc (sizeof (tilda_collect));
    t_collect->tw = tw;
    t_collect->tt = tt;

    if (argc != -1)
    {
        if((fp = fopen(tw->config_file, "r")) != NULL)
        {
            if (read_config_file (argv0, tw->tilda_config, NUM_ELEM, tw->config_file) < 0)
            {
                /* This should _NEVER_ happen, but it's here just in case */
                perror ("Error reading config file, terminating");
                perror ("If you created your config file prior to release .06 then you must delete it and start over, sorry :(");
                exit (1);
            }

            fclose (fp);
        }

        in_main = TRUE;
        gtk_init (&argc, &argv);
    }

    wizard_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_realize (wizard_window);

    g_strlcpy (title, "Tilda", sizeof (title));
    sprintf (title, "%s %i Config", title, tw->instance);
    gtk_window_set_title (GTK_WINDOW (wizard_window), title);

    g_signal_connect (G_OBJECT (wizard_window), "delete_event",
                  G_CALLBACK (exit_app), t_collect);

    gtk_container_set_border_width (GTK_CONTAINER (wizard_window), 10);

    table = gtk_table_new (3, 3, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 10);
    gtk_table_set_col_spacings (GTK_TABLE (table), 10);

    gtk_container_add (GTK_CONTAINER (wizard_window), table);

    style = gtk_widget_get_style(wizard_window);
    image_pix = gdk_pixmap_create_from_xpm_d (GTK_WIDGET(wizard_window)->window,
            &image_pix_mask, &style->bg[GTK_STATE_NORMAL],
            (gchar **)wizard_xpm);
    image = gtk_pixmap_new (image_pix, image_pix_mask);
    gdk_pixmap_unref (image_pix);
    gdk_pixmap_unref (image_pix_mask);
    gtk_table_attach_defaults (GTK_TABLE (table), image, 0, 3, 0, 1);

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
                  G_CALLBACK (ok), t_collect);
    gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 2, 3);
    gtk_widget_show (button);

    button = gtk_button_new_with_label ("Apply");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                  G_CALLBACK (apply), t_collect);
    gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 2, 3);
    gtk_widget_show (button);

    button = gtk_button_new_with_label ("Cancel");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                  G_CALLBACK (exit_app), t_collect);
    gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 2, 3);
    gtk_widget_show (button);

    gtk_widget_show (image);
    gtk_widget_show (notebook);
    gtk_widget_show (table);
    gtk_widget_show (wizard_window);

    if (in_main)
    {
        gtk_main ();
        free (t_collect);
    }
    
    return exit_status;
}

