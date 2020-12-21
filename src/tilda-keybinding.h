#ifndef TILDA_KEYBINDING_H
#define TILDA_KEYBINDING_H

#include <gtk/gtk.h>

#include "tilda_window.h"

typedef struct _TildaKeybindingTreeView TildaKeybindingTreeView;

TildaKeybindingTreeView *
tilda_keybinding_init (GtkBuilder *builder);

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
