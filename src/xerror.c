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

   See the COPYING file for a copy of the GNU General Public License.
*/

#include <tilda-config.h>

#include <xerror.h>
#include <debug.h>
#include <translation.h>

#include <glib.h>
#include <X11/Xlib.h>

static gboolean xerror_ignore = FALSE;
gboolean xerror_occurred = FALSE;

gint xerror_handler(Display *d, XErrorEvent *e)
{
    DEBUG_FUNCTION ("xerror_handler");

    xerror_occurred = TRUE;
#ifdef DEBUG
    if (!xerror_ignore) {
        gchar errtxt[128];

        /*if (e->error_code != BadWindow) */
        {
            XGetErrorText(d, e->error_code, errtxt, 127);
            if (e->error_code == BadWindow)
                /*g_warning("X Error: %s", errtxt)*/;
            else
                g_error(_("X Error: %s"), errtxt);
        }
    }
#else
    (void)d; (void)e;
#endif
    return 0;
}

void xerror_set_ignore(Display *dpy, gboolean ignore)
{
    DEBUG_FUNCTION ("xerror_set_ignore");
    DEBUG_ASSERT (dpy != NULL);

    XSync(dpy, FALSE);
    xerror_ignore = ignore;
}

