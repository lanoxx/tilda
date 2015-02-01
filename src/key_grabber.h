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

#ifndef TILDA_KEY_GRABBER_C
#define TILDA_KEY_GRABBER_C

#include "tilda_window.h"

G_BEGIN_DECLS

void pull (struct tilda_window_ *tw, enum pull_action action, gboolean force_hide);

extern void generate_animation_positions (tilda_window *tw);

gboolean tilda_keygrabber_bind (const gchar *keystr, tilda_window *tw);
void tilda_keygrabber_unbind (const gchar *keystr);

/**
 * This function will make the tilda window active after it starts
 */
void tilda_window_set_active (tilda_window *tw);

G_END_DECLS

#endif
