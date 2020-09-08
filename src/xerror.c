/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-
   vim: set ts=4 sts=4 sw=4 expandtab textwidth=92:

   xerror.c for the Openbox window manager
   Copyright (c) 2006        Mikael Magnusson
   Copyright (c) 2003        Ben Jansens

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include "xerror.h"
#include "debug.h"

#include <glib/gi18n.h>

gint xerror_handler(Display *d, XErrorEvent *e)
{
    DEBUG_FUNCTION ("xerror_handler");

    gchar errtxt[128];

    /* https://tronche.com/gui/x/xlib/event-handling/protocol-errors/default-handlers.html */
    if ((e->error_code != BadWindow) &&
        /* https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html#requests:SetInputFocus */
        ((e->request_code != 42 /* SetInputFocus */) || (e->error_code != BadMatch)))
    {
        XGetErrorText(d, e->error_code, errtxt, 127);
        g_error(_("X Error [opcode %d]: %s"), e->request_code, errtxt);
    }

    return 0;
}
