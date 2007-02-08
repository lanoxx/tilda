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

#ifndef LOAD_TILDA_H
#define LOAD_TILDA_H

#include <tilda_window.h>
#include <tilda_terminal.h>

#include <glib.h>

G_BEGIN_DECLS

gboolean update_tilda (tilda_window *tw, tilda_term *tt, gboolean from_main);

G_END_DECLS

#endif
