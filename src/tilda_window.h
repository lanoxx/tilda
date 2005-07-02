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

#ifndef TILDA_WINDOW_H
#define TILDA_WINDOW_H

#include <gtk/gtk.h>

#include "readconf.h"
#include "tilda_terminal.h"

G_BEGIN_DECLS;

#define NUM_ELEM 21

typedef struct tilda_window_ tilda_window;
typedef struct tilda_conf_ tilda_conf;

struct tilda_window_
{
    GtkWidget *window;
    GtkWidget *notebook;

    tilda_conf *tc;
    gchar lock_file[80];   
    gchar config_file[80];   
    gint instance;
    CONFIG tilda_config[NUM_ELEM];
};

struct tilda_conf_
{
    gint max_width;
    gint max_height;
    gint min_width;
    gint min_height;
    glong lines;
    gchar s_above[6];
    gchar s_notaskbar[6];
    gchar s_pinned[6];
    gchar s_image[100]; 
    gchar s_background[6];
    gchar s_font[64];
    gchar s_down[6];
    gchar s_antialias[6];
    gchar s_scrollbar[6];
    gchar s_use_image[6];
    gchar s_grab_focus[6];
    gchar s_key[50];
    gint transparency;
    gint x_pos;
    gint y_pos;
    gint tab_pos;
};

void add_tab (tilda_window *tw);
void add_tab_menu_call (gpointer data, guint callback_action, GtkWidget *w);
void close_tab (gpointer data, guint callback_action, GtkWidget *w);
gboolean init_tilda_window (tilda_window *tw, tilda_term *tt);
void init_tilda_window_configs (tilda_window *tw);

G_END_DECLS;

#endif
