#include <gtk/gtk.h>
#include <stdio.h>

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
	strcat (config_file, "/.tilde/config");
	
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
	GtkWidget *vbox1, *hbox1;
	GtkWidget *image;
	GtkWidget *menu, *option_wm;
	GtkWidget **menuitem;
	GtkWidget *bcancel, *bapply, *bok;
	gpointer window_manager;
	gpointer items[] = {"KDE 3.x", "Fluxbox", "Window Maker", "Other"};

	int i;
	
	menuitem = (GtkWidget **) malloc (sizeof (GtkWidget) * 4);

	gtk_init(&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  	bcancel = gtk_button_new_with_label ("Cancel");
	bok = gtk_button_new_with_label ("OK");
	bapply = gtk_button_new_with_label ("Apply");
  
  	gtk_signal_connect (GTK_OBJECT (window), "delete_event", GTK_SIGNAL_FUNC (exit_app), NULL); 
	gtk_signal_connect (GTK_OBJECT (bcancel), "clicked", GTK_SIGNAL_FUNC (exit_app), NULL); 
	gtk_signal_connect (GTK_OBJECT (bok), "clicked", GTK_SIGNAL_FUNC (ok), NULL); 
	gtk_signal_connect (GTK_OBJECT (bapply), "clicked", GTK_SIGNAL_FUNC (apply), NULL); 
	
	
	image = gtk_image_new_from_file ("wizard.png");
	
	vbox1 = gtk_vbox_new (FALSE, 0);
	hbox1 = gtk_hbox_new (FALSE, 0);
	
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
	
	gtk_container_add (GTK_CONTAINER (window), vbox1);	
	gtk_container_add (GTK_CONTAINER (window), hbox1);	
	
	gtk_box_pack_start (GTK_BOX (hbox1), bok, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox1), bapply, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox1), bcancel, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), option_wm, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), entry_height, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), entry_width, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), entry_key, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);
	
	gtk_widget_show (bok);
	gtk_widget_show (bapply);
	gtk_widget_show (bcancel);
	gtk_widget_show (option_wm);
	gtk_widget_show (entry_height);
	gtk_widget_show (entry_width);
	gtk_widget_show (entry_key);
	gtk_widget_show (image);
	gtk_widget_show (vbox1);
	gtk_widget_show (hbox1);
	gtk_widget_show ((GtkWidget *) window);
	
	gtk_main();

	return 0;
}
