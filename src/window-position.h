/*
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILDA_WINDOW_POSITION_H
#define TILDA_WINDOW_POSITION_H

#include <gdk/gdk.h>

G_BEGIN_DECLS

void tilda_window_constrain_position (const GdkRectangle *workarea,
                                      gint window_width,
                                      gint window_height,
                                      gint *x,
                                      gint *y);

G_END_DECLS

#endif
