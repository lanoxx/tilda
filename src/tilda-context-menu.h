#ifndef TILDA_CONTEXT_MENU_H
#define TILDA_CONTEXT_MENU_H

#include <gtk/gtk.h>

#include "tilda_terminal.h"
#include "tilda_window.h"

void
tilda_context_menu_popup (tilda_window *tw, tilda_term *tt, GdkEvent * event);

#endif
