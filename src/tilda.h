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

#ifndef TILDA_H
#define TILDA_H

#include <tilda_window.h>

#include <gtk/gtk.h>

/* FIXME: temporary until all files are switched to include
 * FIXME: debug.h on their own. */
#include <debug.h>

G_BEGIN_DECLS

#define TILDA_VERSION PACKAGE_NAME " " PACKAGE_VERSION
#define QUICK_STRCMP(a, b) (*(a)!=*(b)? \
    (int) ((unsigned char) *(a) - (unsigned char) *(b)) : \
    strcmp ((a), (b)))

gboolean image_set_clo, antialias_set_clo, scroll_set_clo;
gint old_max_height, old_max_width;

void clean_up (tilda_window *tw);
void clean_up_no_args ();

extern int find_centering_coordinate (const int screen_dimension, const int tilda_dimension);
extern void print_and_exit (gchar *message, gint exitval);
extern void getinstance (tilda_window *tw);
extern int write_config_file (tilda_window *tw);

#define HEIGHT 0
#define WIDTH  1

#define CONFIG1_OLDER -1
#define CONFIGS_SAME   0
#define CONFIG1_NEWER  1

int get_display_dimension (int dimension);
#define get_physical_height_pixels() get_display_dimension(HEIGHT)
#define get_physical_width_pixels()  get_display_dimension(WIDTH)

G_END_DECLS

#endif
