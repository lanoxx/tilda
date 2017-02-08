#ifndef TILDA_KEYBINDING_H
#define TILDA_KEYBINDING_H

#include <gtk/gtk.h>

#include "tilda_window.h"

typedef struct _TildaKeybindingTreeView TildaKeybindingTreeView;

void
tilda_keybinding_apply (TildaKeybindingTreeView *keybinding);

TildaKeybindingTreeView *
tilda_keybinding_init (GtkBuilder *builder);

void
tilda_keybinding_free (TildaKeybindingTreeView *keybinding);

gboolean
tilda_keybinding_save (TildaKeybindingTreeView *keybinding,
                       tilda_window *tw);

void
tilda_keybinding_show_invalid_keybinding_dialog (GtkWindow *parent,
                                                 const gchar* message);

#endif //TILDA_KEYBINDING_H
