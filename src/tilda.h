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

#include <gtk/gtk.h>

int wizard (int argc, char **argv);
int write_key_bindings (char wm[], char key[]);
void popup (char *message, char *b1_message, char *b2_message, void (*func1)(),void (*func2)());
void redo_wizard (GtkWidget *widget, gpointer data);
void add_anyway (GtkWidget *widget, gpointer data);
