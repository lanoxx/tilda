#include <gtk/gtk.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "tilda.h"

GtkWidget *dialog;
char wm[20];
GtkWidget *entry_height, *entry_width, *entry_key;
GtkWidget *check_pinned, *check_above, *check_taskbar, *check_xbindkeys, *check_devilspie;

void close_dialog (GtkWidget *widget, gpointer data)
{
	gtk_grab_remove (GTK_WIDGET (widget));
	gtk_widget_destroy (GTK_WIDGET (data));
}

void use_devilspie ()
{
	gboolean b_sensitive;
	
	b_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_devilspie));
		
	gtk_widget_set_sensitive (check_pinned, b_sensitive);
	gtk_widget_set_sensitive (check_above, b_sensitive);
	gtk_widget_set_sensitive (check_taskbar, b_sensitive);
}

int check_config_file (char config_file[])
{
	FILE *fp;
	char tmp_string[255];
	int result = 0;
	
	if ((fp = fopen(config_file, "r")) == NULL)
	{
		result = 0;
	}
	else
	{
		while (!feof (fp))
		{
			fgets (tmp_string, 254, fp);
			if (strstr (tmp_string, "</devilspie>") != NULL)
				result = 1;
		}
		
		fclose (fp);
	}

	return result;
}

void write_devilspie (gboolean pinned, gboolean above, gboolean taskbar)
{
	FILE *fp, *tmp_file;
	char tmp_name[L_tmpnam];
	char *tmp_filename;
	char *home_dir, config_file[80];
	char tmp_string[255];
	char s_pinned[6], s_above[6], s_taskbar[6];
	int flag=0;
	
	tmp_filename = tmpnam (tmp_name);
	if ((tmp_file = fopen (tmp_filename, "w")) == NULL)
	{
		perror ("fopen tmpfile");
		exit (1);
	}

	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.devilspie.xml");
	
	if (!pinned)
		strcpy (s_pinned, "FALSE");
	else
		strcpy (s_pinned, "TRUE");
	if (!above)
		strcpy (s_above, "FALSE");
	else
		strcpy (s_above, "TRUE");	
	if (!taskbar)
		strcpy (s_taskbar, "FALSE");
	else
		strcpy (s_taskbar, "TRUE");
		
	if(!(check_config_file (config_file))) 
	{
		if((fp = fopen(config_file, "w")) == NULL) 
		{
			perror("fopen");
        	exit(1);
		}

		fprintf (fp, "<?xml version=\"1.0\"?>\n\n<devilspie>\n\n</devilspie>");
		fclose (fp);
	}
	
	if((fp = fopen(config_file, "r")) == NULL) 
	{
		perror("fopen");
        exit(1);
	}	
	
	
	while (!feof (fp))
    {
		fgets (tmp_string, 254, fp);
	    fprintf (tmp_file, "%s", tmp_string);
		if (strstr (tmp_string, "Tilda Flurb") == NULL)
	    {
			continue;
	    }
	    else
	    {
			flag=1;

		    while (!feof (fp))
		    {
				if (fgets (tmp_string, 254, fp) == NULL)
			 		break;
					
				if (strstr (tmp_string, "skip_taskbar") != NULL)
			    {
				    fprintf (tmp_file, "			<property name=\"skip_tasklist\" value=\"%s\"/>\n", s_taskbar);	        		
			    }
			    else if (strstr (tmp_string, "above") != NULL)
			    {
				    fprintf (tmp_file, "			<property name=\"above\" value=\"%s\"/>\n", s_above); 	
			    }
			    else if (strstr (tmp_string, "pinned") != NULL)
			    {
				    fprintf (tmp_file, "				<property name=\"pinned\" value=\"%s\"/>\n", s_pinned);  	
			    }
				else
					fprintf (tmp_file, "%s", tmp_string);
			}
		    break;
		}
	}

    fclose (fp);
	fclose (tmp_file);
	
    if (flag == 0)
    {
		if ((tmp_file = fopen (tmp_filename, "w")) == NULL)
		{
			perror ("fopen tmpfile");
			exit (1);
		}
	
		if((fp = fopen(config_file, "r")) == NULL) 
		{
        	perror("fopen");
		    exit(1);
	    }
	    else
	    {
			while (!feof (fp))
			{
				fgets (tmp_string, 19, fp);

				if (strstr (tmp_string, "</devilspie>") != NULL)
				{
					fprintf (tmp_file, "\n<flurb name=\"Tilda Flurb\">\n");
		      		fprintf (tmp_file, "		<matchers>\n");
		       	 	fprintf (tmp_file, "			<matcher name=\"DevilsPieMatcherWindowName\">\n");
					fprintf (tmp_file, "				<property name=\"application name\" value=\"tilda*\"/>\n");
        			fprintf (tmp_file, "			</matcher>\n");
          	 	 	fprintf (tmp_file, "		</matchers>\n");
          		  	fprintf (tmp_file, "		<actions>\n");
          		  	fprintf (tmp_file, "		<action name=\"DevilsPieActionHide\">\n");
            		fprintf (tmp_file, "			<property name=\"skip_tasklist\" value=\"%s\"/>\n", s_taskbar);
	        		fprintf (tmp_file, "		</action>\n");
        			fprintf (tmp_file, "		<action name=\"DevilsPieActionLayer\">\n");
        			fprintf (tmp_file, "			<property name=\"above\" value=\"%s\"/>\n", s_above);
        	    	fprintf (tmp_file, "		</action>\n");
        	    	fprintf (tmp_file, "			<action name=\"DevilsPieActionSetWorkspace\">\n");
        	    	fprintf (tmp_file, "				<property name=\"pinned\" value=\"%s\"/>\n", s_pinned);
        	    	fprintf (tmp_file, "			 </action>\n");
        	    	fprintf (tmp_file, "		 </actions>\n");
       			    fprintf (tmp_file, "</flurb>\n");
					fprintf (tmp_file, "%s", tmp_string);
					break;
				}
				fprintf (tmp_file, "%s", tmp_string);
			}
			fclose (tmp_file);
			fclose (fp);
	    }
    }
	
	if (rename(tmp_filename, config_file) == -1) 
	{
    	error("rename (2):");
    	exit(1);
	}
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
	char s_xbindkeys[5], s_devilspie[5];
	gboolean pinned, above, taskbar;
	gchar height[10], width[10], key[20];
	
 	strcpy (height, gtk_entry_get_text (GTK_ENTRY (entry_height)));
	strcpy (width, gtk_entry_get_text (GTK_ENTRY (entry_width)));
	strcpy (key, gtk_entry_get_text (GTK_ENTRY (entry_key)));
	
	printf ("%s\n", height);
	
	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.tilda/");
	
	mkdir (config_file,  S_IRUSR | S_IWUSR | S_IXUSR);
	
	strcat (config_file, "config");
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_xbindkeys)) == TRUE)
		strcpy (s_xbindkeys, "TRUE");
	else
		strcpy (s_xbindkeys, "FALSE");
		
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_devilspie)) == TRUE)
		strcpy (s_devilspie, "TRUE");
	else
		strcpy (s_devilspie, "FALSE");
	
	if((fp = fopen(config_file, "w")) == NULL) 
	{
        perror("fopen");
		exit(1);
	}
	else
	{
		fprintf (fp, "xbindkeys=%s\n", s_xbindkeys);
		fprintf (fp, "devilspie=%s\n", s_devilspie);
		fprintf (fp, "max_height=%s\n", height);
		fprintf (fp, "max_width=%s\n", width);
		fprintf (fp, "min_height=%i\n", 1);
		fprintf (fp, "min_width=%s\n", width);
		fclose (fp);
	}

	write_key_bindings (wm, key);  

	pinned = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_pinned));
	above = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_above));
	taskbar = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_taskbar));
	
	write_devilspie (pinned, above, taskbar);
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
	
	printf ("%s\n", wm);
}

int wizard (int argc, char **argv)
{
	GtkWidget *table;
	GtkWidget *image;
	GtkWidget *vbox, *hbox;
	GtkWidget *menu, *option_wm;
	GtkWidget **menuitem;
	GtkWidget *label_wm, *label_height, *label_width, *label_key;
	GtkWidget *bcancel, *bapply, *bok;
	GtkWidget *vbox2;
	gpointer window_manager;
	gpointer items[] = {"XBindKeys"};//{"KDE 3.x", "Fluxbox", "Window Maker", "Other"};
	
	FILE *fp;
	char *home_dir;
	char config_file[80];
	int i; 
	char max_height[5], max_width[5], min_height[5], min_width[5], key[51], tmp_string[255];
	char s_xbindkeys[5], s_devilspie[5];
	int pinned_flag=0, above_flag=0, taskbar_flag=0;
	char s_temp[5];
	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.tilda/config");

	//read in height width settings already set
	if((fp = fopen(config_file, "r")) == NULL) 
	{
		strcpy (s_xbindkeys, "TRUE");
		strcpy (s_devilspie, "TRUE");
		strcpy (max_height, "1");
		strcpy (max_width, "1");
		strcpy (min_height, "1");
		strcpy (min_width , "1");
    	}
	else
	{
		fscanf (fp, "xbindkeys=%s\n", s_xbindkeys);
		fscanf (fp, "devilspie=%s\n", s_devilspie);
		fscanf (fp, "max_height=%s\n", max_height);
		fscanf (fp, "max_width=%s\n", max_width);
		fscanf (fp, "min_height=%s\n", min_height);
		fscanf (fp, "min_width=%s\n", min_width);
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
		
		key[strlen(key)] = '\0';
		
		fclose (fp);
	}
	
	//read in devilspie settings already set
	strcpy (config_file, home_dir);
	strcat (config_file, "/.devilspie.xml");
	if ((fp = fopen (config_file, "r")) == NULL)
	{
		strcpy (key, " ");
	}
	else
	{
		while (!feof (fp))
		{
			fgets (tmp_string, 254, fp);
			if (strstr (tmp_string, "Tilda Flurb") != NULL)
			{
				while (!feof (fp))
				{
					fgets (tmp_string, 254, fp);
					if (strstr (tmp_string, "skip_tasklist") != NULL)
					{
						sscanf (tmp_string, "%s %s value=\"%s", tmp_string, tmp_string, s_temp);
						if (s_temp[0] == 'T')
							taskbar_flag = 1;
						else
							taskbar_flag = 0;
					}
					if (strstr (tmp_string, "above") != NULL)
					{
						sscanf (tmp_string, "%s %s value=\"%s", tmp_string, tmp_string, s_temp);
						if (s_temp[0] == 'T')
							above_flag = 1;
						else
							above_flag = 0;
					}
					if (strstr (tmp_string, "pinned") != NULL)
					{
						sscanf (tmp_string, "%s %s value=\"%s", tmp_string, tmp_string, s_temp);
						if (s_temp[0] == 'T')
							pinned_flag = 1;
						else
							pinned_flag = 0;
					}	
				}
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

  	vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	vbox2 = gtk_vbox_new (FALSE, 0);
	
  	gtk_container_add (GTK_CONTAINER (dialog), vbox);
  	table = gtk_table_new (4, 4, FALSE);
   	gtk_container_add (GTK_CONTAINER (dialog), table);
  	gtk_container_add (GTK_CONTAINER (dialog), vbox2);
  
  	bcancel = gtk_button_new_with_label ("Cancel");
	bok = gtk_button_new_with_label ("OK");
	bapply = gtk_button_new_with_label ("Apply");
  
  	check_pinned = gtk_check_button_new_with_label ("Display on all workspaces");
  	check_above = gtk_check_button_new_with_label ("Always on top");
  	check_taskbar = gtk_check_button_new_with_label ("Do not show in taskbar");
	check_xbindkeys = gtk_check_button_new_with_label ("Start xbindkeys on load");
  	check_devilspie = gtk_check_button_new_with_label ("Use Devil's Pie");
	
	if (strcasecmp (s_xbindkeys, "TRUE") == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_xbindkeys), TRUE);
	
	if (strcasecmp (s_devilspie, "TRUE") == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_devilspie), TRUE);
	
	if (pinned_flag == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_pinned), TRUE);
  
	if (above_flag == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_above), TRUE);

	if (taskbar_flag == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_taskbar), TRUE);
  	
	
	use_devilspie ();
  
  	gtk_signal_connect (GTK_OBJECT (dialog), "delete_event", GTK_SIGNAL_FUNC (exit_app), NULL); 
	gtk_signal_connect (GTK_OBJECT (bcancel), "clicked", GTK_SIGNAL_FUNC (exit_app), NULL); 
	gtk_signal_connect (GTK_OBJECT (bok), "clicked", GTK_SIGNAL_FUNC (ok), NULL); 
	gtk_signal_connect (GTK_OBJECT (bapply), "clicked", GTK_SIGNAL_FUNC (apply), NULL); 
	gtk_signal_connect (GTK_OBJECT (check_devilspie), "clicked", GTK_SIGNAL_FUNC (use_devilspie), NULL); 	
	
	image = gtk_image_new_from_file ("src/wizard.png");

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
	
	gtk_box_pack_start(GTK_BOX (vbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (vbox), table, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bok, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bapply, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (hbox), bcancel, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_xbindkeys, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_devilspie, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_pinned, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_above, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox2), check_taskbar, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX (vbox), vbox2, FALSE, FALSE, 4);
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
	gtk_widget_show (check_pinned);
	gtk_widget_show (check_above);
	gtk_widget_show (check_taskbar);
	gtk_widget_show (check_xbindkeys);
	gtk_widget_show (check_devilspie);
	gtk_widget_show (image);
	gtk_widget_show (table);
	gtk_widget_show (hbox);
	gtk_widget_show (vbox2);
	gtk_widget_show (vbox);
	gtk_widget_show ((GtkWidget *) dialog);

	gtk_main();

	return 0;
}
