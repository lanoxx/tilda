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

#ifndef TILDA_URL_SPAWNER_H
#define TILDA_URL_SPAWNER_H

#include <gtk/gtk.h>

#include "tilda-match-registry.h"

void tilda_url_spawner_spawn_browser_for_match (GtkWindow * parent,
                                                const gchar * match,
                                                TildaMatchRegistryEntry * entry);

#endif // TILDA_URL_SPAWNER_H
