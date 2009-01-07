/* vim: set ts=4 sts=4 sw=4 noexpandtab nowrap : */

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

#include <tilda-config.h>

#include <debug.h>
#include <configsys.h>
#include <callback_func.h>
#include <translation.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdlib.h> /* exit */


gchar *get_window_title (GtkWidget *widget)
{
    DEBUG_FUNCTION ("get_window_title");
    DEBUG_ASSERT (widget != NULL);

    const gchar *vte_title;
    gchar *window_title;
    gchar *initial;
    gchar *title;

    vte_title = vte_terminal_get_window_title (VTE_TERMINAL (widget));
    window_title = g_strdup (vte_title);
    initial = g_strdup (config_getstr ("title"));

    /* These are not needed anywhere else. If they ever are, move them to a header file */
    enum d_set_title { NOT_DISPLAYED, AFTER_INITIAL, BEFORE_INITIAL, REPLACE_INITIAL };

    switch (config_getint ("d_set_title"))
    {
        case REPLACE_INITIAL:
            title = (window_title != NULL) ? g_strdup (window_title)
                                           : g_strdup (_("Untitled"));
            break;

        case BEFORE_INITIAL:
            title = (window_title != NULL) ? g_strdup_printf ("%s - %s", window_title, initial)
                                           : g_strdup (initial);
            break;

        case AFTER_INITIAL:
            title = (window_title != NULL) ? g_strdup_printf ("%s - %s", initial, window_title)
                                           : g_strdup (initial);
            break;

        case NOT_DISPLAYED:
            title = g_strdup (initial);
            break;

        default:
            g_printerr (_("Bad value for \"d_set_title\" in config file\n"));
            title = g_strdup ("");
            break;
    }

    g_free (window_title);
    g_free (initial);

    return title;
}

