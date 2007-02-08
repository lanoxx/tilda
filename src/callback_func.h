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

#ifndef CALLBACK_FUNC_H
#define CALLBACK_FUNC_H

#include <tilda_window.h>
#include <tilda_terminal.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

G_BEGIN_DECLS

/* Functions for switching between tabs */
extern gboolean goto_tab_1 (tilda_window *tw);
extern gboolean goto_tab_2 (tilda_window *tw);
extern gboolean goto_tab_3 (tilda_window *tw);
extern gboolean goto_tab_4 (tilda_window *tw);
extern gboolean goto_tab_5 (tilda_window *tw);
extern gboolean goto_tab_6 (tilda_window *tw);
extern gboolean goto_tab_7 (tilda_window *tw);
extern gboolean goto_tab_8 (tilda_window *tw);
extern gboolean goto_tab_9 (tilda_window *tw);
extern gboolean goto_tab_10 (tilda_window *tw);

/* Functions for the popup menu */
extern void ccopy (tilda_window *tw);
extern void cpaste (tilda_window *tw);
extern void prev_tab (tilda_window *tw);
extern void next_tab (tilda_window *tw);

extern char* get_window_title (GtkWidget *widget, tilda_window *tw);
extern void fix_size_settings (tilda_window *tw);
extern void window_title_changed (GtkWidget *widget, gpointer data);
extern void clean_up_no_args ();
extern void free_and_remove (tilda_window *tw);
extern void clean_up (tilda_window *tw);
extern void close_tab_on_exit (GtkWidget *widget, gpointer data);
extern void icon_title_changed (GtkWidget *widget, gpointer win);
extern void commit (VteTerminal *vteterminal, gpointer user_data);
extern void deleted_and_quit (GtkWidget *widget, GdkEvent *event, gpointer data);
extern void destroy_and_quit (GtkWidget *widget, gpointer data);
extern void destroy_and_quit_eof (GtkWidget *widget, gpointer data);
extern void destroy_and_quit_exited (GtkWidget *widget, gpointer data);
extern void status_line_changed (GtkWidget *widget, gpointer data);
extern int  button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data);
extern void iconify_window (GtkWidget *widget, gpointer data);
extern void deiconify_window (GtkWidget *widget, gpointer data);
extern void raise_window (GtkWidget *widget, gpointer data);
extern void lower_window (GtkWidget *widget, gpointer data);
extern void maximize_window (GtkWidget *widget, gpointer data);
extern void restore_window (GtkWidget *widget, gpointer data);
extern void refresh_window (GtkWidget *widget, gpointer data);
extern void resize_window (GtkWidget *widget, guint width, guint height, gpointer data);
extern void move_window (GtkWidget *widget, guint x, guint y, gpointer data);
extern void adjust_font_size (GtkWidget *widget, gpointer data, gint howmuch);
extern void increase_font_size(GtkWidget *widget, gpointer data);
extern void decrease_font_size (GtkWidget *widget, gpointer data);
extern void focus_term (GtkWidget *widget, gpointer data);
extern void start_program(tilda_collect *collect);

G_END_DECLS

#endif

