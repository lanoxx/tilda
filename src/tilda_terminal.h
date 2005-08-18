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

#ifndef TILDA_TERMINAL_H
#define TILDA_TERMINAL_H

#include <gtk/gtk.h>
#include "tilda_window.h"

G_BEGIN_DECLS;

typedef struct tilda_term_ tilda_term;
typedef struct tilda_collect_ tilda_collect;

struct tilda_term_
{
    GtkWidget *vte_term;
    GtkWidget *hbox;
    GtkWidget *scrollbar;
};


struct tilda_collect_
{
    struct tilda_window_ *tw;
    struct tilda_term_ *tt;
};

gboolean init_tilda_terminal (struct tilda_window_ *tw, struct tilda_term_ *tt, gboolean in_main);

G_END_DECLS;

#endif

