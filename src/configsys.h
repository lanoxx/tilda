/* vim: set ts=8 sts=8 sw=8 noexpandtab textwidth=112: */
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

#ifndef CONFIGSYS_H
#define CONFIGSYS_H

#include <glib.h>
#include <gtk/gtk.h>

#define DEFAULT_WORD_CHARS  "-A-Za-z0-9,./?%&#:_"

/**
 * Computes the size of pixels relative to max_pixels and
 * returns them as a ratio with a range of 0 to 1.
 */
#define pixels_absolute_to_ratio(max_pixels, pixels) \
    (((gdouble) pixels) / ((gdouble) max_pixels))

/**
 * Computes the absolute number of pixels given a ratio with a range
 * of 0 to 1 and a size.
 */
#define pixels_ratio_to_absolute(ratio, size) \
    (gint) ((ratio) * (size))

/**
 * Macro to convert a long value to a double by scaling it
 * with G_MAXINT.
 */
#define GLONG_TO_DOUBLE(value) ((value) / (double) G_MAXINT)

/**
 * Macro to convert a double to a long value by scaling it with G_MAXINT.
 */
#define GLONG_FROM_DOUBLE(value) ((long) ((value) * G_MAXINT))

/* Initialize and free the config system's private variables */
gint config_init (const gchar *config_file);
gint config_free (const gchar *config_file);

/* Write to disk (generally discouraged) */
gint config_write (const gchar *config_file);

/* Set values in the config system */
gint config_setint     (const gchar *key, const glong val);
gint config_setstr     (const gchar *key, const gchar *val);
gint config_setbool    (const gchar *key, const gboolean val);
gint config_setnint    (const gchar *key, const glong val, const guint idx);
gint config_setdouble  (const gchar *key, const gdouble val);
gint config_setndouble (const gchar *key, const gdouble vat, const guint idx);

/* Get values from the config system */
glong    config_getint     (const gchar *key);
gchar*   config_getstr     (const gchar *key);
gboolean config_getbool    (const gchar *key);
glong    config_getnint    (const gchar *key, const guint idx);
gdouble  config_getdouble  (const gchar *key);
gdouble  config_getndouble (const gchar *key, const guint idx);

/**
 * This function uses the configured relative ratio of the window size and
 * applies it to the current workarea size to compute the desired absolute
 * size in pixels.
 *
 * @param rectangle A GdkRectangle with the configured window size in pixes.
 */
void config_get_configured_window_size (GdkRectangle *rectangle);

#endif /* CONFIGSYS_H */

