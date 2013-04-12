/* vim: set ts=8 sts=8 sw=8 noexpandtab textwidth=112: */

#ifndef CONFIGSYS_H
#define CONFIGSYS_H

#include <glib.h>

#define DEFAULT_WORD_CHARS  "-A-Za-z0-9,./?%&#:_"

/* Initialize and free the config system's private variables */
gint config_init (const gchar *config_file);
gint config_free (const gchar *config_file);

/* Write to disk (generally discouraged) */
gint config_write (const gchar *config_file);

/* Set values in the config system */
gint config_setint     (const gchar *key, const gint val);
gint config_setstr     (const gchar *key, const gchar *val);
gint config_setbool    (const gchar *key, const gboolean val);
gint config_setnint    (const gchar *key, const gint val, const guint idx);
gint config_setdouble  (const gchar *key, const gdouble val);
gint config_setndouble (const gchar *key, const gdouble vat, const guint idx);

/* Get values from the config system */
gint     config_getint     (const gchar *key);
gchar*   config_getstr     (const gchar *key);
gboolean config_getbool    (const gchar *key);
glong    config_getnint    (const gchar *key, const guint idx);
gdouble  config_getdouble  (const gchar *key);
gdouble  config_getndouble (const gchar *key, const guint idx);

#endif /* CONFIGSYS_H */

