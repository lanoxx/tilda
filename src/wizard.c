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

GtkWidget *window;
int exit_status=0;

GtkWidget *entry_height, *entry_width, *entry_x_pos, *entry_y_pos, *entry_key;
GtkWidget *entry_scrollback, *entry_opacity, *button_image, *image_chooser;
GtkWidget *check_xbindkeys, *check_antialias, *check_use_image, *check_grab_focus;
GtkWidget *check_pinned, *check_above, *check_notaskbar, *check_scrollbar, *check_down;
GtkWidget *radio_white, *radio_black;
GtkWidget *button_font;


void close_dialog (GtkWidget *widget, gpointer data)
{
    gtk_grab_remove (GTK_WIDGET (widget));
    gtk_widget_destroy (GTK_WIDGET (data));
}

GtkWidget* general ()
{
    GtkWidget *table;
    GtkWidget *label_scrollback;
    char s_scrollback[20];
    
    table = gtk_table_new (2, 5, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);
    
    label_scrollback = gtk_label_new("Scrollback:");
    
    check_pinned = gtk_check_button_new_with_label ("Display on all workspaces");
    check_above = gtk_check_button_new_with_label ("Always on top");
    check_notaskbar = gtk_check_button_new_with_label ("Do not show in taskbar");
    check_grab_focus = gtk_check_button_new_with_label ("Grab focus on pull down");
    check_scrollbar = gtk_check_button_new_with_label ("Show scrollbar");
    check_down = gtk_check_button_new_with_label ("Display pulled down on start");
    
    entry_scrollback = gtk_entry_new ();
    
    if (strcasecmp (s_scrollbar, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_scrollbar), TRUE);
    
    if (strcasecmp (s_pinned, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_pinned), TRUE);
  
    if (strcasecmp (s_above, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_above), TRUE);

    if (strcasecmp (s_notaskbar, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_notaskbar), TRUE);

	if (strcasecmp (s_grab_focus, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_grab_focus), TRUE);

	if (strcasecmp (s_down, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_down), TRUE);

    sprintf (s_scrollback, "%ld", lines); 
    gtk_entry_set_text (GTK_ENTRY (entry_scrollback), s_scrollback);
    
    gtk_table_attach (GTK_TABLE (table), check_pinned, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_above, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    gtk_table_attach (GTK_TABLE (table), check_notaskbar, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    //gtk_table_attach (GTK_TABLE (table), check_grab_focus, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), check_down, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    gtk_table_attach (GTK_TABLE (table), check_scrollbar, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), label_scrollback, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_scrollback, 1, 2, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    
    //gtk_widget_show (check_grab_focus);
    gtk_widget_show (check_down);
    gtk_widget_show (check_pinned);
    gtk_widget_show (check_above);
    gtk_widget_show (check_notaskbar);
    gtk_widget_show (check_scrollbar);
    gtk_widget_show (label_scrollback);
    gtk_widget_show (entry_scrollback);
    
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

GtkWidget* appearance ()
{
    GtkWidget *table;
    GtkWidget *label_image;
    GtkWidget *label_color, *label_opacity;
    GtkWidget *label_height, *label_width;
    GtkWidget *label_x_pos, *label_y_pos;
    
    char s_max_height[6], s_max_width[6];
    char s_x_pos[6], s_y_pos[6], s_transparency[4];
    
    table = gtk_table_new (3, 8, FALSE);
    label_height = gtk_label_new ("Height in Pixels:");
    label_width = gtk_label_new ("Width in Pixels:");
    label_x_pos = gtk_label_new ("X Pixel Postion to Start:");
    label_y_pos = gtk_label_new ("Y Pixel Postion to Start:");
    label_color = gtk_label_new ("Background Color:");
    label_opacity = gtk_label_new ("Level of Transparency (0-100%):");
    label_image = gtk_label_new ("Background Image:");
    
    entry_height = gtk_entry_new ();
    entry_width = gtk_entry_new ();
    entry_x_pos = gtk_entry_new ();
    entry_y_pos = gtk_entry_new ();
    entry_opacity = gtk_entry_new ();
    
    sprintf (s_max_height, "%d", max_height);
    sprintf (s_max_width, "%d", max_width);
    sprintf (s_x_pos, "%d", x_pos);
    sprintf (s_y_pos, "%d", y_pos); 
    sprintf (s_transparency, "%d", transparency); 
    gtk_entry_set_text (GTK_ENTRY (entry_height), s_max_height);
    gtk_entry_set_text (GTK_ENTRY (entry_width), s_max_width);
    gtk_entry_set_text (GTK_ENTRY (entry_x_pos), s_x_pos);
    gtk_entry_set_text (GTK_ENTRY (entry_y_pos), s_y_pos);
    gtk_entry_set_text (GTK_ENTRY (entry_opacity), s_transparency);
    
    check_use_image = gtk_check_button_new_with_label ("Use Image for Background");
    
    image_chooser = gtk_file_chooser_dialog_new ("Open Background Image File",
                      GTK_WINDOW (window),
                      GTK_FILE_CHOOSER_ACTION_OPEN,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                      NULL);
    button_image = gtk_file_chooser_button_new_with_dialog (image_chooser);
    
    if (strcmp (s_image, "none") != 0);
    
    radio_white = gtk_radio_button_new_with_label (NULL, "White");
    radio_black = gtk_radio_button_new_with_label(gtk_radio_button_group (GTK_RADIO_BUTTON (radio_white)), "Black");
       
    if (strcasecmp (s_background, "white") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_white), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_black), TRUE);
        
    if (strcasecmp (s_use_image, "TRUE") == 0)
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
    gtk_table_attach (GTK_TABLE (table), entry_opacity, 1, 3, 4, 5, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
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
    gtk_widget_show (entry_opacity);
    gtk_widget_show (label_color);    
    gtk_widget_show (entry_x_pos);
    gtk_widget_show (label_x_pos);
    gtk_widget_show (entry_y_pos);
    gtk_widget_show (label_y_pos);
    gtk_widget_show (check_use_image);
    
    return table;
}

GtkWidget* font ()
{
    GtkWidget *table;
    
    table = gtk_table_new (2, 2, FALSE);

    button_font = gtk_font_button_new_with_font (s_font);
    
    check_antialias = gtk_check_button_new_with_label ("Enable anti-aliasing");
    
    if (strcasecmp (s_antialias, "TRUE") == 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_antialias), TRUE);
    
    gtk_table_set_row_spacings (GTK_TABLE (table), 3);
    gtk_table_set_col_spacings (GTK_TABLE (table), 3);
  
    gtk_table_attach (GTK_TABLE (table), button_font, 0, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
    
    gtk_table_attach (GTK_TABLE (table), check_antialias, 0, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
    
    gtk_widget_show (button_font);   
    gtk_widget_show (check_antialias);
    
    return table;
}


GtkWidget* keybindings ()
{
    GtkWidget *table;
    GtkWidget *label_key;
   
    table = gtk_table_new (2, 3, FALSE);

    entry_key = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (entry_key), s_key);
    
    label_key = gtk_label_new("Key Bindings");

    gtk_table_attach (GTK_TABLE (table), label_key, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
    gtk_table_attach (GTK_TABLE (table), entry_key, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);  
    
    gtk_widget_show (label_key);    
    gtk_widget_show (entry_key);
    
    return table;
}

void apply_settings ()
{
    FILE *fp;
    char *home_dir, *tmp_str, config_file[80];
    
    strlcpy (s_key, gtk_entry_get_text (GTK_ENTRY (entry_key)), sizeof(s_key));
    strlcpy (s_font, gtk_font_button_get_font_name (GTK_FONT_BUTTON (button_font)), sizeof (s_font));
    
    if (NULL != (tmp_str = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (image_chooser))))
        strlcpy (s_image,  tmp_str, sizeof (s_image));
    
    max_height = atoi (gtk_entry_get_text (GTK_ENTRY (entry_height)));
    max_width = atoi (gtk_entry_get_text (GTK_ENTRY (entry_width)));
    lines = atoi (gtk_entry_get_text (GTK_ENTRY (entry_scrollback)));
    transparency = atoi (gtk_entry_get_text (GTK_ENTRY (entry_opacity)));
    x_pos = atoi (gtk_entry_get_text (GTK_ENTRY (entry_x_pos)));
    y_pos = atoi (gtk_entry_get_text (GTK_ENTRY (entry_y_pos))); 
        
    home_dir = getenv ("HOME");
    strlcpy (config_file, home_dir, sizeof(config_file));
    strlcat (config_file, "/.tilda/", sizeof(config_file));
    
    mkdir (config_file,  S_IRUSR | S_IWUSR | S_IXUSR);
 
    strlcat (config_file, "config", sizeof(config_file));
    sprintf (config_file, "%s_%i", config_file, instance);
        
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_notaskbar)) == TRUE)
        strlcpy (s_notaskbar, "TRUE", sizeof(s_notaskbar));
    else
        strlcpy (s_notaskbar, "FALSE", sizeof(s_notaskbar));
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_pinned)) == TRUE)
        strlcpy (s_pinned, "TRUE", sizeof(s_pinned));
    else
        strlcpy (s_pinned, "FALSE", sizeof(s_pinned));
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_above)) == TRUE)
        strlcpy (s_above, "TRUE", sizeof(s_above));
    else
        strlcpy (s_above, "FALSE", sizeof(s_above));
        
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_antialias)) == TRUE)
        strlcpy (s_antialias, "TRUE", sizeof(s_antialias));
    else
        strlcpy (s_antialias, "FALSE", sizeof(s_antialias));
        
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_scrollbar)) == TRUE)
        strlcpy (s_scrollbar, "TRUE", sizeof(s_scrollbar));
    else
        strlcpy (s_scrollbar, "FALSE", sizeof(s_scrollbar));    
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_white)))
        strlcpy (s_background, "white", sizeof (s_background));	
    else
        strlcpy (s_background, "black", sizeof (s_background));
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_use_image)) == TRUE)
        strlcpy (s_use_image, "TRUE", sizeof(s_use_image));
    else
        strlcpy (s_use_image, "FALSE", sizeof(s_use_image));    
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_grab_focus)) == TRUE)
        strlcpy (s_grab_focus, "TRUE", sizeof(s_grab_focus));
    else
        strlcpy (s_grab_focus, "FALSE", sizeof(s_grab_focus));    
    
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_down)) == TRUE)
        strlcpy (s_down, "TRUE", sizeof(s_grab_focus));
    else
        strlcpy (s_down, "FALSE", sizeof(s_down)); 
        
    if((fp = fopen(config_file, "w")) == NULL) 
    {
        perror("fopen");
        exit(1);
    }
    else
    {
        fprintf (fp, "max_height : %i\n", max_height);
        fprintf (fp, "max_width : %i\n", max_width);
        fprintf (fp, "min_height : %i\n", 1);
        fprintf (fp, "min_width : %i\n", max_width);  
        fprintf (fp, "notaskbar : %s\n", s_notaskbar);
        fprintf (fp, "above : %s\n", s_above);
        fprintf (fp, "pinned : %s\n", s_pinned);
        fprintf (fp, "image : %s\n", s_image);
        fprintf (fp, "background : %s\n", s_background);
        fprintf (fp, "font : %s\n", s_font);
        fprintf (fp, "antialias : %s\n", s_antialias);
        fprintf (fp, "scrollbar : %s\n", s_scrollbar);
        fprintf (fp, "transparency : %i\n", transparency);
        fprintf (fp, "x_pos : %i\n", x_pos);
        fprintf (fp, "y_pos : %i\n", y_pos);
        fprintf (fp, "scrollback : %ld\n", lines);
        fprintf (fp, "use_image : %s\n", s_use_image);
        fprintf (fp, "grab_focus : %s\n", s_grab_focus);
        fprintf (fp, "key : %s\n", s_key);
        fprintf (fp, "down : %s\n", s_down);
        
        fclose (fp);
    }
}

gint ok ()
{
    apply_settings ();
        
    gtk_widget_destroy (window);
    gtk_main_quit();
    
    return (TRUE);
}

void apply ()
{
    apply_settings ();
}

gint exit_app (GtkWidget *widget, gpointer data)
{   
    gtk_widget_destroy (window);
    gtk_main_quit();
    
    exit_status = 1;
    
    return (FALSE);
}

int wizard (int argc, char **argv)
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
    
    char title[20];
    char *tabs[] = {"General", "Appearance", "Font", "Keybindings"};
    GtkWidget* (*contents[4])();
    
    contents[0] = general;
    contents[1] = appearance; 
    contents[2] = font;
    contents[3] = keybindings; 
    
    FILE *fp;
    char *home_dir;
    char config_file[80];
    int i;

    home_dir = getenv ("HOME");
    strlcpy (config_file, home_dir, sizeof(config_file));
    strlcat (config_file, "/.tilda/config", sizeof(config_file));
    sprintf (config_file, "%s_%i", config_file, instance);

    //read in height width settings already set
    if((fp = fopen(config_file, "r")) == NULL) 
    {
        strlcpy (s_notaskbar, "TRUE", sizeof(s_notaskbar));
        strlcpy (s_pinned, "TRUE", sizeof(s_pinned));
        strlcpy (s_above, "TRUE", sizeof(s_above));
        max_height = 1;
        max_width = 1;
        min_height = 1;
        min_width = 1;
    }
    else
    {
        if (read_config_file (argv[0], tilda_config, NUM_ELEM(tilda_config), config_file) < 0)
        {
            /* This should _NEVER_ happen, but it's here just in case */
            puts("Error reading config file, terminating");
            puts("If you created your config file prior to release .06 then you must delete it and start over, sorry :(");
            exit(1);
        }

        fclose (fp);
    }
   
    gtk_init (&argc, &argv);
    
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_show (window);
    
    strlcpy (title, "Tilda", sizeof (title));
    sprintf (title, "%s %i Config", title, instance);
    gtk_window_set_title (GTK_WINDOW (window), title);
          
    g_signal_connect (G_OBJECT (window), "delete_event",
                  G_CALLBACK (exit_app), NULL);
    
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    
    table = gtk_table_new (3, 3, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), 10);
    gtk_table_set_col_spacings (GTK_TABLE (table), 10);
    
    gtk_container_add (GTK_CONTAINER (window), table);
      
    style = gtk_widget_get_style(window);   
    image_pix = gdk_pixmap_create_from_xpm_d (GTK_WIDGET(window)->window,&image_pix_mask, &style->bg[GTK_STATE_NORMAL],(gchar **)wizard_xpm);
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
        table2 = (*contents[i])();
        
        gtk_widget_show (table2);	
        label = gtk_label_new (tabs[i]);
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table2, label);
    }

    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);
    
    button = gtk_button_new_with_label ("OK");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                  G_CALLBACK (ok), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 2, 3);
    gtk_widget_show (button);
    
    button = gtk_button_new_with_label ("Apply");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                  G_CALLBACK (apply), NULL);
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
    
    gtk_main ();
    
    return exit_status;
}

