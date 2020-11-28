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

#include "tilda-match-registry.h"
#include "tilda-regex.h"

/**
 * The TildaMatchRegistry acts as a central class for all types of matches that
 * the terminal can detect. When the TildaMatchRegistry is used, users can
 * choose to activate all patterns or only selected patterns.
 *
 * For each pattern, the registry handles conversion of a match to a proper URI
 * (if the matched pattern supports this), and it can return action labels to
 * copy and open the match. If a match is not an URI (such as a number match)
 * then it can only be copied but not opened.
 */

#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

typedef char * (* MatchToUriFunc) (const char * match);

typedef struct {
    const char *pattern;
    TildaMatchRegistryFlavor flavor;
    const char *copy_action_label;
    const char *open_action_label;
    MatchToUriFunc match_to_uri;
    gboolean is_uri;
} PatternItem;

static char * default_match_to_uri (const char * match);
static char * http_match_to_uri (const char * match);
static char * email_match_to_uri (const char * match);

static const PatternItem hyperlink_pattern = {
    NULL, TILDA_MATCH_FLAVOR_URL, "_Copy Link", "_Open Link", default_match_to_uri, TRUE
};

static const PatternItem pattern_items[] = {
        { REGEX_URL_AS_IS, TILDA_MATCH_FLAVOR_URL,             "_Copy Link",   "_Open Link",  default_match_to_uri, TRUE },
        { REGEX_URL_HTTP,  TILDA_MATCH_FLAVOR_DEFAULT_TO_HTTP, "_Copy Link",   "_Open Link",  http_match_to_uri, TRUE },
        { REGEX_URL_FILE,  TILDA_MATCH_FLAVOR_FILE,            "_Copy File",   "_Open File",  default_match_to_uri, TRUE },
        { REGEX_EMAIL,     TILDA_MATCH_FLAVOR_EMAIL,           "_Copy Email",  "_Send Email", email_match_to_uri, TRUE },
        { REGEX_NUMBER,    TILDA_MATCH_FLAVOR_NUMBER,          "_Copy Number", NULL,          NULL, FALSE }
};

struct TildaMatchRegistry_ {
    GHashTable *entries;
    TildaMatchRegistryEntry * hyperlink_entry;
};

struct TildaMatchRegistryEntry_ {
    const PatternItem * pattern;
    VteRegex * regex;
};

static void
tilda_match_registry_add (TildaMatchRegistry * registry,
                          const PatternItem *pattern_item,
                          VteRegex * regex,
                          int tag);

static void registry_entry_free (gpointer data)
{
    TildaMatchRegistryEntry * entry = data;

    if (entry->regex != NULL) {
        vte_regex_unref (entry->regex);
    }

    g_slice_free (TildaMatchRegistryEntry, data);
}

TildaMatchRegistry *
tilda_match_registry_new ()
{
    TildaMatchRegistry * registry = g_new0 (TildaMatchRegistry, 1);

    registry->entries = g_hash_table_new_full(NULL,
                                              NULL,
                                              NULL,
                                              registry_entry_free);

    TildaMatchRegistryEntry * entry = g_slice_new0 (TildaMatchRegistryEntry);

    entry->pattern = &hyperlink_pattern;

    registry->hyperlink_entry = entry;

    return registry;
}

static void
tilda_match_registry_add (TildaMatchRegistry * registry,
                          const PatternItem *pattern_item,
                          VteRegex * regex,
                          int tag)
{
    TildaMatchRegistryEntry * entry = g_slice_new0 (TildaMatchRegistryEntry);

    entry->pattern = pattern_item;
    entry->regex = regex;

    g_hash_table_insert (registry->entries, GINT_TO_POINTER (tag), entry);
}

/**
 * tilda_match_registry_for_each:
 * @registry: An instance of a TildaMatchRegistry.
 * @callback: A function to call for each item in the registry.
 * @user_data: user data to pass to the function.
 */
/**
 * TildaMatchHookFunc:
 * @regex: The VteRegex begin enumerated.
 * @user_data: The user data passed to tilda_match_registry_for_each()
 *
 * Declares a type of function that can be passed to
 * tilda_match_registry_for_each() to perform an action on each item
 * in the registry.
 *
 * Returns: A tag to be stored with the regex or TILDA_MATCH_REGISTRY_IGNORE
 * if the regex should not be registered.
 */
void tilda_match_registry_for_each (TildaMatchRegistry * registry,
                                    TildaMatchHookFunc callback,
                                    gpointer user_data)
{
    guint32 flags = PCRE2_CASELESS | PCRE2_MULTILINE;

    for (guint i = 0; i < G_N_ELEMENTS (pattern_items); i++)
    {
        GError * error = NULL;
        const PatternItem * pattern_item = pattern_items + i;

        VteRegex * regex = vte_regex_new_for_match (pattern_item->pattern,
                                                    -1,
                                                    flags,
                                                    &error);

        if (error) {
            g_critical ("Could not register match pattern for flavor %d: %s",
                        pattern_item->flavor,
                        error->message);
            g_error_free (error);
        }

        gint tag = callback (regex, pattern_item->flavor, user_data);

        if (tag == TILDA_MATCH_REGISTRY_IGNORE) {
            continue;
        }

        tilda_match_registry_add (registry, pattern_item, regex, tag);
    }
}

TildaMatchRegistryEntry *
tilda_match_registry_lookup_by_tag (TildaMatchRegistry * registry,
                                    gint tag)
{
    TildaMatchRegistryEntry * entry;

    if (tag < 0) {
        return NULL;
    }

    entry = g_hash_table_lookup(registry->entries, GINT_TO_POINTER(tag));

    return entry;
}

TildaMatchRegistryEntry *
tilda_match_registry_get_hyperlink_entry (TildaMatchRegistry * registry)
{
    return registry->hyperlink_entry;
}

char *
tilda_match_registry_entry_get_action_label (TildaMatchRegistryEntry * entry)
{
    g_return_val_if_fail(entry != NULL, NULL);
    g_return_val_if_fail(entry->pattern != NULL, NULL);

    const char * open_action_label = entry->pattern->open_action_label;

    if (open_action_label == NULL) {
        return NULL;
    }

    return g_strdup (entry->pattern->open_action_label);
}

char *
tilda_match_registry_entry_get_copy_label (TildaMatchRegistryEntry * entry)
{
    g_return_val_if_fail(entry != NULL, NULL);
    g_return_val_if_fail(entry->pattern != NULL, NULL);

    return g_strdup(entry->pattern->copy_action_label);
}

TildaMatchRegistryFlavor
tilda_match_registry_entry_get_flavor (TildaMatchRegistryEntry * entry)
{
    g_return_val_if_fail(entry != NULL, TILDA_MATCH_FLAVOR_LAST);
    g_return_val_if_fail(entry->pattern != NULL, TILDA_MATCH_FLAVOR_LAST);

    return entry->pattern->flavor;
}

gboolean
tilda_match_registry_flavor_is_uri (TildaMatchRegistryEntry * entry)
{
    g_return_val_if_fail(entry != NULL, FALSE);
    g_return_val_if_fail(entry->pattern != NULL, FALSE);

    return entry->pattern->is_uri;
}

char *
tilda_match_registry_entry_get_uri_from_match (TildaMatchRegistryEntry * entry, const char * match)
{
    g_return_val_if_fail(entry != NULL, NULL);
    g_return_val_if_fail(entry->pattern != NULL, NULL);
    g_return_val_if_fail(match != NULL, NULL);

    if (entry->pattern->match_to_uri == NULL) {
        return NULL;
    }

    return entry->pattern->match_to_uri (match);
}

static char *
email_match_to_uri (const char * match)
{
    char * uri;
    const char * mailto_prefix = "mailto:";

    if (g_ascii_strncasecmp(mailto_prefix, match, strlen(mailto_prefix) != 0)) {
        uri = g_strdup_printf ("%s%s", mailto_prefix, match);
    } else {
        uri = g_strdup (match);
    }

    return uri;
}

static char *
default_match_to_uri (const char * match)
{
    return g_strdup(match);
}

static char *
http_match_to_uri (const char * match)
{
    return g_strdup_printf("http://%s", match);
}

void tilda_match_registry_free (TildaMatchRegistry * registry)
{
    g_hash_table_destroy(registry->entries);

    registry_entry_free(registry->hyperlink_entry);

    g_free (registry);
}
