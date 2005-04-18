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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "tilda.h"

char* xbindconvert (char key[])
{
    return key;
}

void redo_wizard (GtkWidget *widget, gpointer data)
{
    //printf ("Redo Wizard\n");
    gtk_grab_remove (GTK_WIDGET (widget));
    gtk_widget_destroy (GTK_WIDGET (data));
    gtk_main_quit();
}

void add_anyway (GtkWidget *widget, gpointer data)
{
    //printf ("Add Anyway\n");
    gtk_widget_destroy (GTK_WIDGET (data));
}

void xbindkeys (char key[])
{
    FILE *fp, *tmp;
    char tmpname[] = "/tmp/tildaXXXXXX";
    int  tmpdesc;
    char *home_dir, config_file[80];
    char tmp_string[101];
    int key_num, total=0, done=0, i=0;
    
    tmpdesc = mkstemp(tmpname);
    
    if (tmpdesc != -1) 
    {
        /* Managed to get a file ?
        Associate a FILE* with the descriptor
         */
        if ((tmp = fdopen(tmpdesc, "w")) == NULL ) 
        {
            /* Failed to associate FILE* */
            perror("fdopen tmpdesc");
            exit(1);
        }
    }
    else 
    {
        /* Failed to create a temporary file */
        perror("mkstemp(tmpname)");
        exit(1);
    }
    
    home_dir = getenv ("HOME");
    strlcpy (config_file, home_dir, sizeof(config_file));
    strlcat (config_file, "/.xbindkeysrc", sizeof(config_file));
                   
    for (i=0;i<strlen(key);i++)
    {
        if (key[i] == ',')
            total++;
    }
    i=0;
                   
    if((fp = fopen(config_file, "r")) == NULL) 
    {
        if((fp = fopen(config_file, "w")) == NULL) 
        {
            perror("fopen");
            exit(1);
        }

        fprintf (fp, "\n\"tilda -T 0\"\n%s\n", key);
        fclose (fp);
    }
    else
    {           
        fgets (tmp_string, 100, fp);
        do
        {
            if (strstr (tmp_string, "tilda -T") != NULL)
            {
                sscanf (tmp_string, "\"tilda -T %d\"", &key_num);
                fprintf (tmp, "%s", tmp_string);
                fgets (tmp_string, 100, fp);
                
                while (i<key_num)
                {
                    if (*key == ',')
                        i++;
                    key++;
                }
                
                while (*key != ',' && *key != '\0')
                {
                    fprintf (tmp, "%c", *key);
                    key++;
                }
                
                fprintf (tmp, "\n");
                done++;
            }   
            else 
                fprintf (tmp, "%s", tmp_string);
            
            fgets (tmp_string, 100, fp);
        }while (!feof (fp));

        for (;done<=total;done++)
        {
            fprintf (tmp, "\n\"tilda -T %d\"\n", done);
            while (i<done)
            {
                if (*key == ',')
                    i++;
                key++;
            }
            
            while (*key != ',' && *key != '\0')
            {
                fprintf (tmp, "%c", *key);
                key++;
            }
        }       
        
        fclose (fp);
        fclose (tmp);
        rename (tmpname, config_file);
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
    strlcpy (config_file, home_dir, sizeof(config_file));
    strlcat (config_file, "/.fluxbox/keys", sizeof(config_file));
    
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
    
    strlcpy (command, getcwd (command, 80), sizeof(command));
    
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
