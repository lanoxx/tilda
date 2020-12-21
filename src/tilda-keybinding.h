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

#ifndef TILDA_KEYBINDING_H
#define TILDA_KEYBINDING_H

#include <gtk/gtk.h>

#include "tilda_window.h"

typedef struct TildaKeybindingTreeView_ TildaKeybindingTreeView;

TildaKeybindingTreeView *
tilda_keybinding_init (GtkBuilder *builder,
                       gboolean allow_empty_pull_shortcut);

void
tilda_keybinding_apply (TildaKeybindingTreeView *keybinding);

gboolean
tilda_keybinding_save (TildaKeybindingTreeView *keybinding,
                       tilda_window *tw);

void
tilda_keybinding_show_invalid_keybinding_dialog (GtkWindow *parent,
                                                 const gchar* message);

void
tilda_keybinding_free (TildaKeybindingTreeView *keybinding);

#endif //TILDA_KEYBINDING_H
