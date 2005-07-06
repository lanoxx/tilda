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
#include "config.h"
#include "wizard.h"

#define MAX_INT 2147483647

GtkWidget *wizard_window;
int exit_status=0;

GtkWidget *entry_height, *entry_width, *entry_x_pos, *entry_y_pos, *entry_key;
GtkWidget *button_image, *image_chooser, *combo_tab_pos;
GtkWidget *check_xbindkeys, *check_antialias, *check_use_image, *check_grab_focus;
GtkWidget *check_pinned, *check_above, *check_notaskbar, *check_scrollbar, *check_down;
GtkWidget *radio_white, *radio_black;
GtkWidget *slider_opacity, *spin_scrollback;
GtkWidget *button_font;
gboolean in_main = FALSE;

gboolean update_tilda ();
void fix_size_settings ();

void close_dialog (GtkWidget *widget, gpointer data)
{
    gtk_grab_remove (GTK_WIDGET (widget));
    gtk_widget_destroy (GTK_WIDGET (data));
}

GtkWidget* general (tilda_window *tw, tilda_term *tt)
{
    GtkWidget *table;
    GtkWidget *label_scrollback, *label_tab_pos;

    table = gtk_table_new (2, 6, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);

    label_scrollback = gtk_label_new("Scrollback:");
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
    check_grab_focus = gtk_check_button_new_with_label ("Grab focus on pull down");
    check_scrollbar = gtk_check_button_new_with_label ("Show scrollbar");
    check_down = gtk_check_button_new_with_label ("Display pulled down on start");

    spin_scrollback = gtk_spin_button_new_with_range (0, MAX_INT, 1);

    if (strcasecmp (tw->tc->s_scrollbar, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_scrollbar), TRUE);

    if (strcasecmp (tw->tc->s_pinned, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_pinned), TRUE);

    if (strcasecmp (tw->tc->s_above, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_above), TRUE);

    if (strcasecmp (tw->tc->s_notaskbar, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_notaskbar), TRUE);

    if (strcasecmp (tw->tc->s_grab_focus, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_grab_focus), TRUE);

    if (strcasecmp (tw->tc->s_down, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_down), TRUE);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_scrollback), tw->tc->lines);

    gtk_table_attach (GTK_TABLE (table), check_pinned, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_above, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_notaskbar, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    //gtk_table_attach (GTK_TABLE (table), check_grab_focus, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_down, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    gtk_table_attach (GTK_TABLE (table), check_scrollbar, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    gtk_table_attach (GTK_TABLE (table), label_scrollback, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), spin_scrollback, 1, 2, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    gtk_table_attach (GTK_TABLE (table), label_tab_pos, 0, 1, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), combo_tab_pos, 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    //gtk_widget_show (check_grab_focus);
    gtk_widget_show (label_tab_pos);
    gtk_widget_show (combo_tab_pos);
    gtk_widget_show (check_down);
    gtk_widget_show (check_pinned);
    gtk_widget_show (check_above);
    gtk_widget_show (check_notaskbar);
    gtk_widget_show (check_scrollbar);
    gtk_widget_show (label_scrollback);
    gtk_widget_show (spin_scrollback);
    
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
    GtkWidget *label_color, *label_opacity;
    GtkWidget *label_height, *label_width;
    GtkWidget *label_x_pos, *label_y_pos;

    char s_max_height[6], s_max_width[6];
    char s_x_pos[6], s_y_pos[6];

    table = gtk_table_new (3, 8, FALSE);
    label_height = gtk_label_new ("Height in Pixels:");
    label_width = gtk_label_new ("Width in Pixels:");
    label_x_pos = gtk_label_new ("X Pixel Postion to Start:");
    label_y_pos = gtk_label_new ("Y Pixel Postion to Start:");
    label_color = gtk_label_new ("Background Color:");
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

    radio_white = gtk_radio_button_new_with_label (NULL, "White");
    radio_black = gtk_radio_button_new_with_label(gtk_radio_button_group (GTK_RADIO_BUTTON (radio_white)), "Black");

    if (strcasecmp (tw->tc->s_background, "white") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_white), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_black), TRUE);

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

    gtk_table_attach (GTK_TABLE (table), label_opacity, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), slider_opacity, 1, 3, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_color, 0, 1, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), radio_white, 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), radio_black, 2, 3, 5, 6, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_use_image, 0, 3, 6, 7, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), label_image, 0, 1, 7, 8, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), button_image, 1, 3, 7, 8, GTK_EXPAND | GTK_FILL,GTK_FILL, 3, 3);

    gtk_widget_show (entry_height);
    gtk_widget_show (label_height);
    gtk_widget_show (entry_width);
    gtk_widget_show (label_width);

    gtk_widget_show (radio_white);
    gtk_widget_show (radio_black);

    gtk_widget_show (label_opacity);
    gtk_widget_show (slider_opacity);
    gtk_widget_show (label_color);
    gtk_widget_show (entry_x_pos);
    gtk_widget_show (label_x_pos);
    gtk_widget_show (entry_y_pos);
    gtk_widget_show (label_y_pos);
    gtk_widget_show (check_use_image);

    return table;
}

GtkWidget* font (tilda_window *tw, tilda_term *tt)
{
    GtkWidget *table;

    table = gtk_table_new (2, 2, FALSE);

    button_font = gtk_font_button_new_with_font (tw->tc->s_font);

    check_antialias = gtk_check_button_new_with_label ("Enable anti-aliasing");

    if (strcasecmp (tw->tc->s_antialias, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_antialias), TRUE);

    gtk_table_set_row_spacings (GTK_TABLE (table), 3);
    gtk_table_set_col_spacings (GTK_TABLE (table), 3);

    gtk_table_attach (GTK_TABLE (table), button_font, 0, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

    gtk_table_attach (GTK_TABLE (table), check_antialias, 0, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

    gtk_widget_show (button_font);
    gtk_widget_show (check_antialias);

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

void apply_settings (tilda_window *tw, tilda_term *tt)
{
    FILE *fp;
    char *home_dir, *tmp_str, config_file[80];

    g_strlcpy (tw->tc->s_key, gtk_entry_get_text (GTK_ENTRY (entry_key)), sizeof(tw->tc->s_key));
    g_strlcpy (tw->tc->s_font, gtk_font_button_get_font_name (GTK_FONT_BUTTON (button_font)), sizeof (tw->tc->s_font));

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
    
    switch (gtk_combo_box_get_active ((GtkComboBox *) combo_tab_pos))
    {
        case 0:
            tw->tc->tab_pos = 0;
            break;
        case 1:
            tw->tc->tab_pos = 1;
            break;
        case 2:
            tw->tc->tab_pos = 2;
            break;
        case 3:
            tw->tc->tab_pos = 3;
            break;
        default:
            tw->tc->tab_pos = 0;    
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

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_white)))
        g_strlcpy (tw->tc->s_background, "white", sizeof (tw->tc->s_background));
    else
        g_strlcpy (tw->tc->s_background, "black", sizeof (tw->tc->s_background));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_use_image)) == TRUE)
        g_strlcpy (tw->tc->s_use_image, "TRUE", sizeof(tw->tc->s_use_image));
    else
        g_strlcpy (tw->tc->s_use_image, "FALSE", sizeof(tw->tc->s_use_image));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_grab_focus)) == TRUE)
        g_strlcpy (tw->tc->s_grab_focus, "TRUE", sizeof(tw->tc->s_grab_focus));
    else
        g_strlcpy (tw->tc->s_grab_focus, "FALSE", sizeof(tw->tc->s_grab_focus));

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_down)) == TRUE)
        g_strlcpy (tw->tc->s_down, "TRUE", sizeof(tw->tc->s_grab_focus));
    else
        g_strlcpy (tw->tc->s_down, "FALSE", sizeof(tw->tc->s_down));

    if((fp = fopen(tw->config_file, "w")) == NULL)
    {
        perror("fopen");
        exit(1);
    }
    else
    {
        fprintf (fp, "max_height : %i\n", tw->tc->max_height);
        fprintf (fp, "max_width : %i\n", tw->tc->max_width);
        fprintf (fp, "min_height : %i\n", 1);
        fprintf (fp, "min_width : %i\n", tw->tc->max_width);
        fprintf (fp, "notaskbar : %s\n", tw->tc->s_notaskbar);
        fprintf (fp, "above : %s\n", tw->tc->s_above);
        fprintf (fp, "pinned : %s\n", tw->tc->s_pinned);
        fprintf (fp, "image : %s\n", tw->tc->s_image);
        fprintf (fp, "background : %s\n", tw->tc->s_background);
        fprintf (fp, "font : %s\n", tw->tc->s_font);
        fprintf (fp, "antialias : %s\n", tw->tc->s_antialias);
        fprintf (fp, "scrollbar : %s\n", tw->tc->s_scrollbar);
        fprintf (fp, "transparency : %i\n", tw->tc->transparency);
        fprintf (fp, "x_pos : %i\n", tw->tc->x_pos);
        fprintf (fp, "y_pos : %i\n", tw->tc->y_pos);
        fprintf (fp, "scrollback : %ld\n", tw->tc->lines);
        fprintf (fp, "use_image : %s\n", tw->tc->s_use_image);
        fprintf (fp, "grab_focus : %s\n", tw->tc->s_grab_focus);
        fprintf (fp, "key : %s\n", tw->tc->s_key);
        fprintf (fp, "down : %s\n", tw->tc->s_down);
        fprintf (fp, "tab_pos : %i\n", tw->tc->tab_pos);

        fclose (fp);
    }

    if (!in_main)
        update_tilda (tw, tt, FALSE);
}

gint ok (tilda_collect *tc)
{
    apply_settings (tc->tw, tc->tt);
	exit_status = 0;
    gtk_widget_destroy (wizard_window);

    if (in_main)
		gtk_main_quit();
	
    return (TRUE);
}

void apply (tilda_collect *tc)
{
    apply_settings (tc->tw, tc->tt);
}

gint exit_app (GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy (wizard_window);

    if (in_main)
        gtk_main_quit();

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
    char *argv0 = NULL;
    char title[20];
    char *tabs[] = {"General", "Appearance", "Font", "Keybindings"};

    GtkWidget* (*contents[4])(tilda_window *, tilda_term *);

    contents[0] = general;
    contents[1] = appearance;
    contents[2] = font;
    contents[3] = keybindings;

    FILE *fp;
    int i;
	
    if (argv != NULL)
        argv0 = argv[0];

	if((fp = fopen(tw->config_file, "r")) != NULL)
	{
        if (read_config_file (argv0, tw->tilda_config, NUM_ELEM, tw->config_file) < 0)
        {
            /* This should _NEVER_ happen, but it's here just in case */
            puts("Error reading config file, terminating");
            puts("If you created your config file prior to release .06 then you must delete it and start over, sorry :(");
            exit(1);
        }

        fclose (fp);
    }

    t_collect = (tilda_collect *) malloc (sizeof (tilda_collect));
    t_collect->tw = tw;
    t_collect->tt = tt;

    if (argc != -1)
    {
        in_main = TRUE;
        gtk_init (&argc, &argv);
    }

    wizard_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
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

    style = gtk_widget_get_style(wizard_window);
    image_pix = gdk_pixmap_create_from_xpm_d (GTK_WIDGET(wizard_window)->window,&image_pix_mask, &style->bg[GTK_STATE_NORMAL],(gchar **)wizard_xpm);
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
    for (i = 0; i < 4; i++)
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
                  G_CALLBACK (exit_app), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, 2, 3);
    gtk_widget_show (button);

    gtk_widget_show (image);
    gtk_widget_show (notebook);
    gtk_widget_show (table);
    gtk_widget_show (wizard_window);

    if (in_main)
        gtk_main ();

    return exit_status;
}
