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

#include <gtk/gtk.h>
#include "tilda_window.h"

G_BEGIN_DECLS;

#define TILDA_VERSION PACKAGE_NAME " " PACKAGE_VERSION
#define DEFAULT_LINES 100
#define QUICK_STRCMP(a, b) (*(a)!=*(b)? \
    (int) ((unsigned char) *(a) - (unsigned char) *(b)) : \
    strcmp ((a), (b)))
#define SCROLLBAR_LEFT 0 /* false */

gboolean image_set_clo, antialias_set_clo, scroll_set_clo;
gint old_max_height, old_max_width;

/* global commandline arg values */
gchar *command;
gchar *working_directory;

void clean_up (tilda_window *tw);
void clean_up_no_args ();

G_END_DECLS;

#endif
