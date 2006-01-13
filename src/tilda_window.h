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

#include "tilda_terminal.h"
#include <confuse.h>

G_BEGIN_DECLS;

typedef struct tilda_window_ tilda_window;

struct tilda_window_
{
    GtkWidget *window;
    GtkWidget *notebook;
    GList *terms;

    gchar *home_dir;
    gchar *lock_file;
    gchar *config_file;
    gint instance;

    /* Stores all configuration */
    cfg_t *tc;
};

void add_tab (tilda_window *tw);
void add_tab_menu_call (gpointer data, guint callback_action, GtkWidget *w);
void close_tab (gpointer data, guint callback_action, GtkWidget *w);
gboolean init_tilda_window (tilda_window *tw, tilda_term *tt);
void init_tilda_window_instance (int *argc, char ***argv, tilda_window *tw, tilda_term *tt);

G_END_DECLS;

#endif
