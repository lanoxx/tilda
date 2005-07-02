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

#include <gtk/gtk.h>

G_BEGIN_DECLS;

 void window_title_changed (GtkWidget *widget, gpointer win);
 void icon_title_changed (GtkWidget *widget, gpointer win);
 void char_size_changed (GtkWidget *widget, guint width, guint height, gpointer data);
 void deleted_and_quit (GtkWidget *widget, GdkEvent *event, gpointer data);
 void destroy_and_quit (GtkWidget *widget, gpointer data);
 void destroy_and_quit_eof (GtkWidget *widget, gpointer data);
 void destroy_and_quit_exited (GtkWidget *widget, gpointer data);
 void status_line_changed (GtkWidget *widget, gpointer data);
 int button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data);
 void iconify_window (GtkWidget *widget, gpointer data);
 void deiconify_window (GtkWidget *widget, gpointer data);
 void raise_window (GtkWidget *widget, gpointer data);
 void lower_window (GtkWidget *widget, gpointer data);
 void maximize_window (GtkWidget *widget, gpointer data);
 void restore_window (GtkWidget *widget, gpointer data);
 void refresh_window (GtkWidget *widget, gpointer data);
 void resize_window (GtkWidget *widget, guint width, guint height, gpointer data);
 void move_window (GtkWidget *widget, guint x, guint y, gpointer data);
 void adjust_font_size (GtkWidget *widget, gpointer data, gint howmuch);
 void increase_font_size(GtkWidget *widget, gpointer data);
 void decrease_font_size (GtkWidget *widget, gpointer data);

G_END_DECLS;

#endif

