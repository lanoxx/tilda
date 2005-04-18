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

#ifndef TILDA_H
#define TILDA_H

#include <gtk/gtk.h>

/* These are from readconf, which is a little program that makes reading
 * config files nice and easy.
 * The source was: http://www.fadden.com/dl-misc/
 *
 * It is released as public domain, so there's no problem using it in tilda
 */
#include "readconf.h"
#include "readconf.c"

/* These are from OpenBSD. They are safe string handling functions.
 * The source was: ftp://ftp.openbsd.org//pub/OpenBSD/src/lib/libc/string/strlcat.c
 *                 ftp://ftp.openbsd.org//pub/OpenBSD/src/lib/libc/string/strlcpy.c
 *
 * Here is a little guide on usage: http://www.courtesan.com/todd/papers/strlcpy.html
 * 
 * In short, the syntax is just like strncpy() and strncat().
 */
#include "strlcat.c"
#include "strlcpy.c"

#define DEFAULT_LINES 100
#define NUM_ELEM(a)        ((int)(sizeof(a) / sizeof((a)[0])))

extern int wizard (int argc, char **argv);
extern int write_key_bindings (char wm[], char key[]);
extern void popup (char *message, char *b1_message, char *b2_message, void (*func1)(),void (*func2)());
extern void redo_wizard (GtkWidget *widget, gpointer data);
extern void add_anyway (GtkWidget *widget, gpointer data);

gint max_width, max_height, min_width, min_height;
long lines = DEFAULT_LINES;
char s_xbindkeys[6], s_above[6], s_notaskbar[6], s_pinned[6];

const CONFIG tilda_config[] = {
        { CF_INT,       "max_height",   &max_height,    0,                      NULL, 0, NULL },
        { CF_INT,       "max_width",    &max_width,     0,                      NULL, 0, NULL },
        { CF_INT,       "min_height",   &min_height,    0,                      NULL, 0, NULL },
        { CF_INT,       "min_width",    &min_width,     0,                      NULL, 0, NULL },
        { CF_STRING,    "notaskbar",    s_notaskbar,    sizeof(s_notaskbar),    NULL, 0, NULL },
        { CF_STRING,    "above",        s_above,        sizeof(s_above),        NULL, 0, NULL },
        { CF_STRING,    "pinned",       s_pinned,       sizeof(s_pinned),       NULL, 0, NULL },
        { CF_STRING,    "xbindkeys",    s_xbindkeys,    sizeof(s_xbindkeys),    NULL, 0, NULL },
        { CF_INT,       "scrollback",   &lines,         0,                      NULL, 0, NULL }
};

#include "wizard.c"
#include "key_bindings.c"

#endif
