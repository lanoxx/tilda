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

GtkWidget *dialog;
char wm[20] = "xbindkeys";
GtkWidget *entry_height, *entry_width, *entry_key;
GtkWidget *check_pinned, *check_above, *check_notaskbar, *check_xbindkeys;

void close_dialog (GtkWidget *widget, gpointer data)
{
	gtk_grab_remove (GTK_WIDGET (widget));
	gtk_widget_destroy (GTK_WIDGET (data));
}

void popup (char *message, char *b1_message, char *b2_message, void (*func1)(), void (*func2)())
{
	GtkWidget *label;
	GtkWidget *b1, *b2;
	GtkWidget *dialog_window;
	
	dialog_window = gtk_dialog_new ();
	
	gtk_signal_connect (GTK_OBJECT (dialog_window), "destroy", GTK_SIGNAL_FUNC (close_dialog),	&dialog_window);
	
	gtk_window_set_title (GTK_WINDOW (dialog_window), "Tilda Dialog");
	
	gtk_container_border_width (GTK_CONTAINER (dialog_window), 5);
	
	label = gtk_label_new (message);
	
	gtk_misc_set_padding (GTK_MISC (label), 10, 10);
	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), label, TRUE, TRUE, 0);
	
	gtk_widget_show (label);
	
	b1 = gtk_button_new_with_label (b1_message);
	b2 = gtk_button_new_with_label (b2_message);
	
	gtk_signal_connect (GTK_OBJECT (b1), "clicked", GTK_SIGNAL_FUNC (func1), dialog_window);
	gtk_signal_connect (GTK_OBJECT (b2), "clicked", GTK_SIGNAL_FUNC (func2), dialog_window);
	
	GTK_WIDGET_SET_FLAGS (b2, GTK_CAN_DEFAULT);
	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), b1, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), b2, TRUE, TRUE, 0);
	
	gtk_widget_grab_default (b2);
	
	gtk_widget_show (b1);
	gtk_widget_show (b2);
	
	gtk_widget_show (dialog_window);
	
	gtk_grab_add (dialog_window);
}

void apply_settings ()
{
	FILE *fp;
	char *home_dir, config_file[80];
	char s_xbindkeys[5], s_notaskbar[5], s_pinned[5], s_above[5];
	gchar height[10], width[10], key[20];
	
 	strcpy (height, gtk_entry_get_text (GTK_ENTRY (entry_height)));
	strcpy (width, gtk_entry_get_text (GTK_ENTRY (entry_width)));
	strcpy (key, gtk_entry_get_text (GTK_ENTRY (entry_key)));
	
	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.tilda/");
	
	mkdir (config_file,  S_IRUSR | S_IWUSR | S_IXUSR);
	
	strcat (config_file, "config");
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_xbindkeys)) == TRUE)
		strcpy (s_xbindkeys, "TRUE");
	else
		strcpy (s_xbindkeys, "FALSE");
		
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_notaskbar)) == TRUE)
		strcpy (s_notaskbar, "TRUE");
	else
		strcpy (s_notaskbar, "FALSE");
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_pinned)) == TRUE)
		strcpy (s_pinned, "TRUE");
	else
		strcpy (s_pinned, "FALSE");
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_above)) == TRUE)
		strcpy (s_above, "TRUE");
	else
		strcpy (s_above, "FALSE");
	
	if((fp = fopen(config_file, "w")) == NULL) 
	{
        perror("fopen");
		exit(1);
	}
	else
	{
		fprintf (fp, "max_height=%s\n", height);
		fprintf (fp, "max_width=%s\n", width);
		fprintf (fp, "min_height=%i\n", 1);
		fprintf (fp, "min_width=%s\n", width);	
		fprintf (fp, "notaskbar=%s\n", s_notaskbar);
		fprintf (fp, "above=%s\n", s_above);
		fprintf (fp, "pinned=%s\n", s_pinned);
		fprintf (fp, "xbindkeys=%s\n", s_xbindkeys);
		fclose (fp);
	}

	write_key_bindings (wm, key);  
}


gint ok ()
{
	apply_settings ();
		
	gtk_widget_destroy (dialog);
	gtk_main_quit();
	return (FALSE);
}

void apply ()
{
	apply_settings ();
}

gint exit_app (GtkWidget *widget, gpointer data)
{	
	gtk_widget_destroy (dialog);
	gtk_main_quit();
	return (FALSE);
}

void selected (gpointer window_manager)
{
	strcpy (wm, window_manager);
}

int wizard (int argc, char **argv)
{
	GtkWidget *table;
	GtkWidget *image;
	GtkWidget *vbox, *hbox, *vbox2;
	GtkWidget *menu, *option_wm;
	GtkWidget **menuitem;
	GtkWidget *label_wm, *label_height, *label_width, *label_key;
	GtkWidget *bcancel, *bapply, *bok;
	GdkPixmap *image_pix;
    GdkBitmap *image_pix_mask;
	GtkStyle   *style;
	gpointer items[] = {"XBindKeys"};//{"KDE 3.x", "Fluxbox", "Window Maker", "Other"};
	
	FILE *fp;
	char *home_dir;
	char config_file[80];
	int i; 
	char max_height[5], max_width[5], min_height[5], min_width[5], key[51], tmp_string[255];
	char s_xbindkeys[5], s_notaskbar[5], s_above[5], s_pinned[5];
	
	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.tilda/config");

	//read in height width settings already set
	if((fp = fopen(config_file, "r")) == NULL) 
	{
		strcpy (s_xbindkeys, "TRUE");
		strcpy (s_notaskbar, "TRUE");
		strcpy (s_pinned, "TRUE");
		strcpy (s_above, "TRUE");
		strcpy (max_height, "1");
		strcpy (max_width, "1");
		strcpy (min_height, "1");
		strcpy (min_width , "1");
    	}
	else
	{
		fscanf (fp, "max_height=%s\n", max_height);
		fscanf (fp, "max_width=%s\n", max_width);
		fscanf (fp, "min_height=%s\n", min_height);
		fscanf (fp, "min_width=%s\n", min_width);
		fscanf (fp, "notaskbar=%s\n", s_notaskbar);
		fscanf (fp, "above=%s\n", s_above);
		fscanf (fp, "pinned=%s\n", s_pinned);
		fscanf (fp, "xbindkeys=%s\n", s_xbindkeys);
		fclose (fp);
	}
	
	//read in keybinding settings already set
	strcpy (config_file, home_dir);
	strcat (config_file, "/.xbindkeysrc");
	if ((fp = fopen (config_file, "r")) == NULL)
	{
		strcpy (key, " ");
	}
	else
	{
		while (!feof (fp))
		{
			fgets (tmp_string, 254, fp);
			if (strstr (tmp_string, "tilda -T") != NULL)
			{
				fgets (key, 50, fp);
				break;
			}
		}
		
		key[strlen(key)-1] = '\0';
		
		fclose (fp);
	}
	
	menuitem = (GtkWidget **) malloc (sizeof (GtkWidget) * 4);

	gtk_init(&argc, &argv);
	
	label_wm = gtk_label_new("Key Bindings For:");
	label_height = gtk_label_new("Height in Pixels");
	label_width = gtk_label_new("Width in Pixels");
	label_key = gtk_label_new("Key Binding");

	dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);	
	gtk_widget_show ((GtkWidget *) dialog);

	vbox = gtk_vbox_new (FALSE, 4);
	hbox = gtk_hbox_new (FALSE, 4);
	vbox2 = gtk_vbox_new (FALSE, 4);
	
	table = gtk_table_new (4, 4, FALSE);
	
	gtk_container_add (GTK_CONTAINER (dialog), vbox);
	
  	bcancel = gtk_button_new_with_label ("Cancel");
	bok = gtk_button_new_with_label ("OK");
	bapply = gtk_button_new_with_label ("Apply");
  
  	check_pinned = gtk_check_button_new_with_label ("Display on all workspaces");
  	check_above = gtk_check_button_new_with_label ("Always on top");
  	check_notaskbar = gtk_check_button_new_with_label ("Do not show in taskbar");
	check_xbindkeys = gtk_check_button_new_with_label ("Start xbindkeys on load");
  	
	if (strcasecmp (s_xbindkeys, "TRUE") == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_xbindkeys), TRUE);
	
	if (strcasecmp (s_pinned, "TRUE") == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_pinned), TRUE);
  
	if (strcasecmp (s_above, "TRUE") == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_above), TRUE);

	if (strcasecmp (s_notaskbar, "TRUE") == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_notaskbar), TRUE);

  	gtk_signal_connect (GTK_OBJECT (dialog), "delete_event", GTK_SIGNAL_FUNC (exit_app), NULL); 
	gtk_signal_connect (GTK_OBJECT (bcancel), "clicked", GTK_SIGNAL_FUNC (exit_app), NULL); 
	gtk_signal_connect (GTK_OBJECT (bok), "clicked", GTK_SIGNAL_FUNC (ok), NULL); 
	gtk_signal_connect (GTK_OBJECT (bapply), "clicked", GTK_SIGNAL_FUNC (apply), NULL); 
	
	style = gtk_widget_get_style(dialog);	
	image_pix = gdk_pixmap_create_from_xpm_d (GTK_WIDGET(dialog)->window,&image_pix_mask, &style->bg[GTK_STATE_NORMAL],(gchar **)wizard_xpm);
    image = gtk_pixmap_new (image_pix, image_pix_mask);
	gdk_pixmap_unref (image_pix);
    gdk_pixmap_unref (image_pix_mask);
	
	option_wm = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_wm), menu);
	
	for (i=0;i<(sizeof(items)/sizeof(int));i++)
	{
		menuitem[i] = gtk_radio_menu_item_new_with_label (NULL, items[i]);
		gtk_menu_append (GTK_MENU (menu), menuitem[i]);
		gtk_widget_show (menuitem[i]);
		gtk_signal_connect_object (GTK_OBJECT (menuitem[i]), "activate", GTK_SIGNAL_FUNC (selected), (gpointer) items[i]);
	}
	
	gtk_option_menu_set_history (GTK_OPTION_MENU (option_wm), 0);
	
	entry_height = gtk_entry_new ();
	entry_width = gtk_entry_new ();
	entry_key = gtk_entry_new ();
	
	gtk_entry_set_text (GTK_ENTRY (entry_height), max_height);
	gtk_entry_set_text (GTK_ENTRY (entry_width), max_width);
	gtk_entry_set_text (GTK_ENTRY (entry_key), key);
	
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table),5);
	
	gtk_table_attach_defaults (GTK_TABLE (table), label_height, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), entry_height, 1, 2, 0, 1);
	
	gtk_table_attach_defaults (GTK_TABLE (table), label_width, 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), entry_width, 1, 2, 1, 2);
	
	gtk_table_attach_defaults (GTK_TABLE (table), label_wm, 0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), option_wm, 1, 2, 2, 3);
	
	gtk_table_attach_defaults (GTK_TABLE (table), label_key, 0, 1, 3, 4);
	gtk_table_attach_defaults (GTK_TABLE (table), entry_key, 1, 2, 3, 4);

	gtk_box_pack_start(GTK_BOX (vbox), image, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (vbox), table, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bok, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bapply, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bcancel, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_xbindkeys, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_pinned, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_above, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_notaskbar, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox), vbox2, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, FALSE, 4);	
		
	gtk_widget_show (image);	
	gtk_widget_show (bok);
	gtk_widget_show (bapply);
	gtk_widget_show (bcancel);
	gtk_widget_show (label_wm);
	gtk_widget_show (option_wm);
	gtk_widget_show (label_height);
	gtk_widget_show (entry_height);
	gtk_widget_show (label_width);
	gtk_widget_show (entry_width);
	gtk_widget_show (label_key);	
	gtk_widget_show (entry_key);
	gtk_widget_show (check_pinned);
	gtk_widget_show (check_above);
	gtk_widget_show (check_notaskbar);
	gtk_widget_show (check_xbindkeys);
	
	gtk_widget_show (table);
	gtk_widget_show (hbox);
	gtk_widget_show (vbox2);
	gtk_widget_show (vbox);

	gtk_main();

	return 0;
}
