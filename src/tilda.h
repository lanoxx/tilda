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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILDA_H
#define TILDA_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TILDA_VERSION PACKAGE_NAME " " PACKAGE_VERSION

struct lock_info
{
    gint pid;
    gint instance;
    gint file_descriptor;
};

#define GUINT16_TO_FLOAT(color) ((color) / (double) 0xFFFF)
#define GUINT16_FROM_FLOAT(value) ((int) ((value) * 0xFFFF + 0.5))

#define RGB(r,g,b) (r) / (gdouble)G_MAXUINT16, \
                     (g) / (gdouble)G_MAXUINT16, \
                     (b) / (gdouble)G_MAXUINT16, 1.

G_END_DECLS

#endif
