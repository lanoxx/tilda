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

#ifndef TILDA_MATCH_REGISTRY_H
#define TILDA_MATCH_REGISTRY_H

#include <glib.h>
#include <vte/vte.h>

typedef enum {
    TILDA_MATCH_FLAVOR_URL,
    TILDA_MATCH_FLAVOR_DEFAULT_TO_HTTP,
    TILDA_MATCH_FLAVOR_FILE,
    TILDA_MATCH_FLAVOR_EMAIL,
    TILDA_MATCH_FLAVOR_NUMBER,
    TILDA_MATCH_FLAVOR_LAST
} TildaMatchRegistryFlavor;

typedef struct TildaMatchRegistry_ TildaMatchRegistry;

typedef struct TildaMatchRegistryEntry_ TildaMatchRegistryEntry;

typedef int (*TildaMatchHookFunc)(VteRegex * regex, TildaMatchRegistryFlavor flavor, gpointer user_data);

#define TILDA_MATCH_REGISTRY_IGNORE -1

TildaMatchRegistry *
tilda_match_registry_new (void);

void
tilda_match_registry_for_each (TildaMatchRegistry * registry,
                               TildaMatchHookFunc callback,
                               gpointer user_data);

TildaMatchRegistryEntry *
tilda_match_registry_lookup_by_tag (TildaMatchRegistry * registry,
                                    gint tag);

TildaMatchRegistryEntry *
tilda_match_registry_get_hyperlink_entry (TildaMatchRegistry * registry);

char *
tilda_match_registry_entry_get_action_label (TildaMatchRegistryEntry * entry);

char *
tilda_match_registry_entry_get_copy_label (TildaMatchRegistryEntry * entry);

TildaMatchRegistryFlavor
tilda_match_registry_entry_get_flavor (TildaMatchRegistryEntry * entry);

gboolean
tilda_match_registry_flavor_is_uri (TildaMatchRegistryEntry * entry);

char *
tilda_match_registry_entry_get_uri_from_match (TildaMatchRegistryEntry * entry,
                                               const char * match);

void
tilda_match_registry_free (TildaMatchRegistry * registry);

#endif
