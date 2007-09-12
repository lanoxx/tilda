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

#include <tilda_window.h>
#include <tilda_terminal.h>

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct tilda_window_ tilda_window;

struct tilda_window_
{
    GtkWidget *window;
    GtkWidget *notebook;
    GList *terms;

    gchar *home_dir;
    gchar *lock_file;
    gchar *config_file;
    gboolean config_writing_disabled;
    gint instance;
    gboolean have_argb_visual;

    /* This field MUST be set before calling pull()! */
    enum tilda_positions { UP, DOWN } current_state;
};

enum notebook_tab_positions { NB_TOP, NB_BOTTOM, NB_LEFT, NB_RIGHT };

gint tilda_window_add_tab (tilda_window *tw);
gint tilda_window_close_tab (tilda_window *tw, gint tab_position);
tilda_window *tilda_window_init (const gchar *config_file, const gint instance);
gint tilda_window_free (tilda_window *tw);

gint tilda_window_set_tab_position (tilda_window *tw, enum notebook_tab_positions pos);
void tilda_window_close_current_tab (tilda_window *tw);

#define TILDA_WINDOW(data) ((tilda_window *)(data))

G_END_DECLS

#endif
