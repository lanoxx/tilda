#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tilda.h"

char* xbindconvert (char key[])
{
	return key;
}

void redo_wizard (GtkWidget *widget, gpointer data)
{
	printf ("Redo Wizard\n");
	gtk_widget_destroy (GTK_WIDGET (data));
}

void add_anyway (GtkWidget *widget, gpointer data)
{
	printf ("Add Anyway\n");
	gtk_widget_destroy (GTK_WIDGET (data));
}

int xbindkeys (char key[])
{
	FILE *fp;
	char *home_dir, config_file[80], command[80];
	char tmp, tmp_string[20];
	int i=0;
	
	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.xbindkeysrc");
	                        
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
			if (strchr (tmp_string, '#') == NULL)
			{
				if (strstr (tmp_string, key) == NULL)
				{
					continue;
				}
				else
				{
					perror ("already there!!!\n");
					popup ("Key Already In Use", "Add Anyway", "Redo", redo_wizard, add_anyway);
					i = 1;
					break;
				}
			}
		}
		
		fclose (fp);
		
		if (i != 1)
		{
			if((fp = fopen(config_file, "a")) == NULL) 
			{
        		perror("fopen");
				exit(1);
    		}
			else
			{
				fprintf (fp, "\n\"tilda\"\n%s\n", key);
				fclose (fp);
			}
		}
	}
}

int gnome (char key[])
{

	return 0;
}

int kde (char key[])
{

	return 0;
}

int window_maker (char key[])
{
	
	return 0;
}

int fluxbox (char key[])
{
	FILE *fp;
	char *home_dir, config_file[80], command[80];
	char tmp, tmp_string[20];
	int i=0;
	
	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.fluxbox/keys");
	
	if((fp = fopen(config_file, "r")) == NULL) 
	{
        perror("fopen");
		exit(1);
    }
	else
	{
		while (!feof (fp))
		{
			while ((tmp = fgetc (fp)) != ':' && i != 80)
			{
				tmp_string[i] = tmp;
				i++;
			}
			
			tmp_string[i] = '\0';
			
			while ((fgetc (fp)) != '\n' && !feof (fp));
			
			i=0;

			if (strncasecmp (tmp_string, key, strlen (key)) == 0)
			{
				printf ("Already there!!!\n");
				i=1;
				break;
			}
		}

		fclose (fp);
	}
	
	strcpy (command, getcwd (command, 80));
	printf ("%s\n", command);
	
	if((fp = fopen(config_file, "a")) == NULL) 
	{
        perror("fopen");
		exit(1);
    }	
	else 
	{
		if (i != 1)
			fprintf (fp, "%s :ExecCommand %s/tilda -T", key, command);
	
		fclose (fp);
	}
	
	free (home_dir);
	
	return 0;
}

int xfce (char key[])
{

	return 0;
}

int write_key_bindings (char wm[], char key[])
{
	if (strcasecmp (wm, "xbindkeys") == 0)
	{
		xbindkeys (xbindconvert (key));
	}
	else if (strcasecmp (wm, "fluxbox") == 0)
	{
		fluxbox (key);
	}
	else if (strcasecmp (wm, "xfce") == 0)
	{
		xfce (key);
	}
	else if (strcasecmp (wm, "window maker") == 0)
	{
		window_maker (key);
	}
	else if (strcasecmp (wm, "kde") == 0)
	{
		kde (key);
	}
	else if (strcasecmp (wm, "gnome") == 0)
	{
		gnome (key);
	}

	
	return 0;
}
