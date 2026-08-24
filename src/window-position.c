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

#include "window-position.h"

void
tilda_window_constrain_position (const GdkRectangle *workarea,
                                 gint window_width,
                                 gint window_height,
                                 gint *x,
                                 gint *y)
{
    g_return_if_fail (workarea != NULL);
    g_return_if_fail (x != NULL);
    g_return_if_fail (y != NULL);

    gint max_x = workarea->x;
    gint max_y = workarea->y;

    if (window_width < workarea->width)
        max_x += workarea->width - window_width;

    if (window_height < workarea->height)
        max_y += workarea->height - window_height;

    *x = CLAMP (*x, workarea->x, max_x);
    *y = CLAMP (*y, workarea->y, max_y);
}
