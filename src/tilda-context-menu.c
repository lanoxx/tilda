#include "tilda-context-menu.h"

#include "debug.h"
#include "wizard.h"
#include "tilda-url-spawner.h"

#include <vte/vte.h>
#include <glib/gi18n.h>

typedef struct {
    tilda_window * tw;
    tilda_term * tt;
    char * match;
    TildaMatchRegistryEntry * match_entry;
} TildaContextMenu;

static void
tilda_context_menu_free (TildaContextMenu * context_menu);

static void
menu_add_tab_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_add_tab_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_window_add_tab (TILDA_WINDOW (user_data));
}

static void
menu_close_tab_cb (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_close_tab_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_window_close_current_tab (TILDA_WINDOW (user_data));
}

static void
menu_copy_cb (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_copy_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_term *tt = TILDA_TERM (user_data);

    vte_terminal_copy_clipboard_format (VTE_TERMINAL (tt->vte_term), VTE_FORMAT_TEXT);
}

static void
menu_paste_cb (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_paste_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_term *tt = TILDA_TERM (user_data);

    vte_terminal_paste_clipboard (VTE_TERMINAL (tt->vte_term));
}

static void
menu_copy_match_cb (GSimpleAction * action,
                    GVariant      * parameter,
                    gpointer        user_data)
{
    DEBUG_FUNCTION ("menu_copy_match_cb");
    DEBUG_ASSERT (user_data != NULL);

    char * match = user_data;

    GtkClipboard * clipBoard = gtk_clipboard_get_default (gdk_display_get_default ());

    gtk_clipboard_set_text (clipBoard, match, -1);
}

static void
menu_open_match_cb (GSimpleAction * action,
                    GVariant      * parameter,
                    gpointer        user_data)
{
    DEBUG_FUNCTION("menu_open_match_cb");

    TildaContextMenu *context_menu = user_data;
    GtkWindow * parent;

    parent = GTK_WINDOW (context_menu->tw->window);

    tilda_url_spawner_spawn_browser_for_match (parent,
                                               context_menu->match,
                                               context_menu->match_entry);
}

static void
menu_fullscreen_cb (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_fullscreen_cb");
    DEBUG_ASSERT (user_data != NULL);

    toggle_fullscreen_cb (TILDA_WINDOW (user_data));
}

static void
menu_searchbar_cb (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_fullscreen_cb");
    DEBUG_ASSERT (user_data != NULL);

    tilda_window_toggle_searchbar (TILDA_WINDOW (user_data));
}

static void
menu_preferences_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_config_cb");
    DEBUG_ASSERT (user_data != NULL);

    /* Show the config wizard */
    wizard (TILDA_WINDOW (user_data));
}

static void
menu_quit_cb (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
    DEBUG_FUNCTION ("menu_quit_cb");

    tilda_window_confirm_quit (TILDA_WINDOW (user_data));
}

static void on_popup_hide (GtkWidget *widget, TildaContextMenu * context_menu)
{
    DEBUG_FUNCTION ("on_popup_hide");
    DEBUG_ASSERT (widget != NULL);
    DEBUG_ASSERT (context_menu != NULL);
    DEBUG_ASSERT (context_menu->tw != NULL);

    tilda_window * tw = context_menu->tw;

    tw->disable_auto_hide = FALSE;
}

static void
on_selection_done (GtkWidget *widget, TildaContextMenu * context_menu)
{
    DEBUG_FUNCTION ("on_selection_done");

    gtk_menu_detach (GTK_MENU (widget));

    tilda_context_menu_free (context_menu);
}

static GMenuModel *
create_menu_model (TildaContextMenu * context_menu)
{
    GMenu * menu = g_menu_new();

    // tab section

    GMenu *tab_section = g_menu_new ();
    g_menu_append (tab_section, _("_New Tab"), "window.new-tab");
    g_menu_append (tab_section, _("_Close Tab"), "window.close-tab");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (tab_section));

    // copy section

    GMenu *match_section = g_menu_new ();
    g_menu_append (match_section, _("_Copy"), "window.copy");
    g_menu_append (match_section, _("_Paste"), "window.paste");

    if (context_menu->match_entry != NULL)
    {
        char * copy_label;
        char * open_label;

        copy_label = tilda_match_registry_entry_get_copy_label (
                context_menu->match_entry);
        g_menu_append (match_section, copy_label, "window.copy-match");

        open_label = tilda_match_registry_entry_get_action_label (
                context_menu->match_entry);

        if (open_label != NULL) {
            g_menu_append (match_section, open_label, "window.open-match");
        }

        g_free (open_label);
        g_free (copy_label);
    }

    g_menu_append_section (menu, NULL, G_MENU_MODEL (match_section));

    // toggle section

    GMenu *toggle_section = g_menu_new ();
    g_menu_append (toggle_section, _("_Toggle Fullscreen"), "window.fullscreen");
    g_menu_append (toggle_section, _("_Toggle Searchbar"), "window.searchbar");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (toggle_section));

    // preferences section

    GMenu *preferences_section = g_menu_new ();
    g_menu_append (preferences_section, _("_Preferences"), "window.preferences");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (preferences_section));

    // quit section

    GMenu *quit_section = g_menu_new ();
    g_menu_append (quit_section, _("_Quit"), "window.quit");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (quit_section));

    return G_MENU_MODEL (menu);
}

GtkWidget *
tilda_context_menu_popup (tilda_window *tw, tilda_term *tt, const char * match, TildaMatchRegistryEntry * entry)
{
    DEBUG_FUNCTION ("tilda_context_menu_popup");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tt != NULL);

    TildaContextMenu * context_menu = g_new0 (TildaContextMenu, 1);
    context_menu->tw = tw;
    context_menu->tt = tt;
    context_menu->match = g_strdup (match);
    context_menu->match_entry = entry;

    GtkBuilder *builder = gtk_builder_new ();

    gtk_builder_add_from_resource (builder, "/org/tilda/menu.ui", NULL);

    /* Create the action group */
    GSimpleActionGroup *action_group = g_simple_action_group_new ();

    /* We need different lists of entries because the
     * actions have different scope, some concern the
     * tilda_window and others concern the current terminal, so
     * when we add them to the action_group we need to pass different
     * user_data (tw or tt).
     *
     * Note: Using designated initializers here, allows us to skip the remaining fields which are NULL anyway and
     * also gets rid of missing field initializer warnings.
     */
    GActionEntry entries_for_tilda_window[] = {
            { .name="new-tab", menu_add_tab_cb },
            { .name="close-tab", menu_close_tab_cb },
            { .name="fullscreen", menu_fullscreen_cb },
            { .name="searchbar", menu_searchbar_cb },
            { .name="preferences", menu_preferences_cb },
            { .name="quit", menu_quit_cb }
    };

    GActionEntry entries_for_tilda_terminal[] = {
            { .name="copy", menu_copy_cb},
            { .name="paste", menu_paste_cb}
    };

    GActionEntry entries_for_match_copy [] = {
            {.name="copy-match", menu_copy_match_cb}
    };

    GActionEntry entries_for_match_open [] = {
            {.name="open-match", menu_open_match_cb}
    };

    g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                     entries_for_tilda_window, G_N_ELEMENTS (entries_for_tilda_window), tw);
    g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                     entries_for_tilda_terminal, G_N_ELEMENTS (entries_for_tilda_terminal), tt);

    g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                     entries_for_match_copy, G_N_ELEMENTS (entries_for_match_copy), context_menu->match);

    g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                     entries_for_match_open, G_N_ELEMENTS (entries_for_match_open), context_menu);

    gtk_widget_insert_action_group (tw->window, "window", G_ACTION_GROUP (action_group));

    GMenuModel *menu_model = create_menu_model (context_menu);
    GtkWidget *menu = gtk_menu_new_from_model (menu_model);

    gtk_menu_attach_to_widget (GTK_MENU (menu), tw->window, NULL);

    gtk_menu_set_accel_group (GTK_MENU (menu), tw->accel_group);
    gtk_menu_set_accel_path (GTK_MENU (menu), "<tilda>/context");

    /* Disable auto hide */
    tw->disable_auto_hide = TRUE;
    g_signal_connect (G_OBJECT (menu), "unmap", G_CALLBACK (on_popup_hide), context_menu);
    g_signal_connect (G_OBJECT (menu), "selection-done", G_CALLBACK (on_selection_done), context_menu);

    g_object_unref (action_group);
    g_object_unref (builder);

    return menu;
}

static void
tilda_context_menu_free (TildaContextMenu * context_menu)
{
    g_free (context_menu->match);
    g_free (context_menu);
}
