#include <gtk/gtk.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

char wm[20];
GtkWidget *entry_height, *entry_width, *entry_key;

void apply_settings ()
{
	FILE *fp;
	char *home_dir, config_file[80];
	gchar height[10], width[10], key[10];
	
 	strcpy (height, gtk_entry_get_text (GTK_ENTRY (entry_height)));
	strcpy (width, gtk_entry_get_text (GTK_ENTRY (entry_width)));
	strcpy (key, gtk_entry_get_text (GTK_ENTRY (entry_key)));
	
	printf ("%s\n", height);
	
	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.tilda/");
	
	mkdir (config_file,  S_IRUSR | S_IWUSR | S_IXUSR);
	
	strcat (config_file, "config");
	

	if((fp = fopen(config_file, "wb")) == NULL) 
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
		fclose (fp);
	}

	free (home_dir);
}


gint ok ()
{
	apply_settings ();
		
	gtk_main_quit ();
	
	return (FALSE);
}

void apply ()
{
	apply_settings ();
}

gint exit_app (GtkWidget *widget, gpointer gdata)
{
	gtk_main_quit ();
	
	return (FALSE);
}

void selected (gpointer window_manager)
{
	strcpy (wm, window_manager);
	
	printf ("%s\n", wm);
}

int main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *table;
	GtkWidget *image;
	GtkWidget *vbox, *hbox;
	GtkWidget *menu, *option_wm;
	GtkWidget **menuitem;
	GtkWidget *label_wm, *label_height, *label_width, *label_key;
	GtkWidget *bcancel, *bapply, *bok;
	gpointer window_manager;
	gpointer items[] = {"KDE 3.x", "Fluxbox", "Window Maker", "Other"};

	int i;
	
	menuitem = (GtkWidget **) malloc (sizeof (GtkWidget) * 4);

	gtk_init(&argc, &argv);

	
	label_wm = gtk_label_new("Window Manager");
	label_height = gtk_label_new("Height in Pixels");
	label_width = gtk_label_new("Width in Pixels");
	label_key = gtk_label_new("Key Binding");

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  	

  	vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	
  	gtk_container_add (GTK_CONTAINER (window), vbox);
  	table = gtk_table_new (4, 4, FALSE);
    gtk_container_add (GTK_CONTAINER (window), table);

  
  	bcancel = gtk_button_new_with_label ("Cancel");
	bok = gtk_button_new_with_label ("OK");
	bapply = gtk_button_new_with_label ("Apply");
  
  	gtk_signal_connect (GTK_OBJECT (window), "delete_event", GTK_SIGNAL_FUNC (exit_app), NULL); 
	gtk_signal_connect (GTK_OBJECT (bcancel), "clicked", GTK_SIGNAL_FUNC (exit_app), NULL); 
	gtk_signal_connect (GTK_OBJECT (bok), "clicked", GTK_SIGNAL_FUNC (ok), NULL); 
	gtk_signal_connect (GTK_OBJECT (bapply), "clicked", GTK_SIGNAL_FUNC (apply), NULL); 
	
	
	image = gtk_image_new_from_file ("wizard.png");

	option_wm = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_wm), menu);
	
	for (i=0;i<4;i++)
	{
		menuitem[i] = gtk_radio_menu_item_new_with_label (NULL, items[i]);
		gtk_menu_append (GTK_MENU (menu), menuitem[i]);
		gtk_widget_show (menuitem[i]);
		gtk_signal_connect_object (GTK_OBJECT (menuitem[i]), "activate", GTK_SIGNAL_FUNC (selected), (gpointer) items[i]);
	}
	
	entry_height = gtk_entry_new ();
	entry_width = gtk_entry_new ();
	entry_key = gtk_entry_new ();
	
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
	
	gtk_box_pack_start(GTK_BOX (vbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (vbox), table, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bok, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bapply, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bcancel, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, FALSE, 4);	
	
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
	gtk_widget_show (image);
	gtk_widget_show (table);
	gtk_widget_show (hbox);
	gtk_widget_show (vbox);
	gtk_widget_show ((GtkWidget *) window);
	
	gtk_main();

	return 0;
}
