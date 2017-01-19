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

#ifndef TILDA_H
#define TILDA_H

#include "tilda_window.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TILDA_VERSION PACKAGE_NAME " " PACKAGE_VERSION
#define QUICK_STRCMP(a, b) (*(a)!=*(b)? \
    (int) ((unsigned char) *(a) - (unsigned char) *(b)) : \
    strcmp ((a), (b)))

struct lock_info
{
    gint pid;
    gint instance;
    gint file_descriptor;
};

typedef struct tilda_cli_options tilda_cli_options;

struct tilda_cli_options {
    gchar *background_color;
    gchar *command;
    gchar *font;
    gchar *working_dir;
#ifdef VTE_290
    gchar *image;
    gint *transparency;
#else
    gint back_alpha;
#endif
    gint lines;
    gint x_pos;
    gint y_pos;
    gboolean antialias;
    gboolean scrollbar;
    gboolean show_config;
    gboolean version;
    gboolean hidden;
};

#define GUINT16_TO_FLOAT(color) (color / (double) 0xFFFF)
#define GUINT16_FROM_FLOAT(value) ((int) (value * 0xFFFF + 0.5))

#define RGB(r,g,b) (r) / (gdouble)G_MAXUINT16, \
                     (g) / (gdouble)G_MAXUINT16, \
                     (b) / (gdouble)G_MAXUINT16, 1.

G_END_DECLS

#endif

/* Future compatibility with VTE-2.90 */
#ifndef VTE_290 
#define vte_terminal_set_colors_rgba vte_terminal_set_colors
#define vte_terminal_set_color_background_rgba vte_terminal_set_color_background
#define vte_terminal_set_color_foreground_rgba vte_terminal_set_color_foreground
#define vte_terminal_set_color_cursor_rgba vte_terminal_set_color_cursor
#define VteTerminalCursorShape VteCursorShape
#endif

