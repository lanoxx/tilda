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

#ifndef TILDA_TERMINALN_H
#define TILDA_TERMINALN_H

#include "tilda_window.h"
#include "tilda-palettes.h"
#include "tilda-match-registry.h"

#include <gtk/gtk.h>
#include <vte/vte.h>

G_BEGIN_DECLS

typedef struct tilda_term_ tilda_term;

struct tilda_term_
{
    GtkWidget *vte_term;
    GtkWidget *hbox;
    GtkWidget *scrollbar;
    GRegex *http_regexp;
    VteRegex *vte_regexp;
    TildaMatchRegistry * registry;
    GPid pid;
    /* We remember if we have already dropped to the default
     * shell before, if so, then we know that this time we can
     * exit the program.
     */
    gboolean dropped_to_default_shell;
    gchar *initial_working_dir;

    struct tilda_window_ *tw;
};

enum tilda_term_scrollbar_positions { RIGHT, LEFT, DISABLED };
enum delete_keys { ASCII_DELETE, DELETE_SEQUENCE, ASCII_BACKSPACE, AUTO };

/**
 * tilda_term_init ()
 *
 * Initialize and return a new struct tilda_term_.
 *
 * @param tw The main tilda window, which must be initialized.
 *
 * Success: return a non-NULL struct tilda_term_ *.
 * Failure: return NULL.
 *
 * Notes: you must call tilda_term_free() on the returned struct tilda_term_
 *        when you are finished using it, and it has been removed from all GTK
 *        structures, such as the notebook.
 */
struct tilda_term_ *tilda_term_init (struct tilda_window_ *tw);

/**
 * tilda_term_free ()
 *
 * Free a struct tilda_term_* created with tilda_term_init (). This will
 * clean up any memory allocations that were made to create the object. It
 * should only be called when there is no more need to access the object.
 *
 * Success: return 0
 * Failure: return non-zero
 */
gint tilda_term_free (struct tilda_term_ *term);


void tilda_term_set_scrollbar_position (tilda_term *tt, enum tilda_term_scrollbar_positions pos);
char* tilda_term_get_cwd(tilda_term* tt);


/* Adjust the font-size scale for the terminal */
void tilda_term_adjust_font_scale(tilda_term *term, gdouble scale);

gchar * tilda_terminal_get_full_title (tilda_term *tt);
gchar * tilda_terminal_get_title (tilda_term *tt);

void tilda_terminal_update_matches (tilda_term *tt);

#define TILDA_TERM(tt) ((tilda_term *)(tt))

G_END_DECLS

/* vim: set ts=4 sts=4 sw=4 expandtab: */

#endif /* TILDA_TERMINALN_H */

