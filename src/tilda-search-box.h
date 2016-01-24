/*
 * Copyright (C) 2016 Sebastian Geiger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILDA_SEARCH_BOX_H
#define TILDA_SEARCH_BOX_H

#include <gtk/gtk.h>
#include <vte/vte.h>

G_BEGIN_DECLS

#define TILDA_TYPE_SEARCH_BOX tilda_search_box_get_type ()
G_DECLARE_FINAL_TYPE (TildaSearchBox, tilda_search_box, TILDA, SEARCH_BOX, GtkBox);

typedef enum {
  SEARCH_FORWARD,
  SEARCH_BACKWARD
} TildaSearchDirection;

GtkWidget *tilda_search_box_new    (void);

/**
 * Toggles the visibility of the search box.
 *
 * If this causes the search box to be made visible, then the search entry
 * widget will be focused.
 *
 * Otherwise if the search box was visible and will be hidden now, then the
 * ::focus-out signal will be emitted. Users of this widget can connect to
 * the ::focus-out signal and use it focus another widget instead.
 */
void       tilda_search_box_toggle (TildaSearchBox *box);

G_END_DECLS

#endif
