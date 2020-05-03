#include "tilda-context-menu.h"

#include "debug.h"
#include "wizard.h"
#include <vte/vte.h>

typedef struct {
    tilda_window * tw;
    tilda_term * tt;
    char * match;
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
menu_copy_link_cb (GSimpleAction * action,
                   GVariant      * parameter,
                   gpointer        user_data)
{
    DEBUG_FUNCTION ("menu_copy_link_cb");
    DEBUG_ASSERT (user_data != NULL);

    char * link = user_data;

    GtkClipboard * clipBoard = gtk_clipboard_get_default (gdk_display_get_default ());

    gtk_clipboard_set_text (clipBoard, link, -1);
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

void
tilda_context_menu_popup (tilda_window *tw, tilda_term *tt, GdkEvent * event)
{
    DEBUG_FUNCTION ("popup_menu");
    DEBUG_ASSERT (tw != NULL);
    DEBUG_ASSERT (tt != NULL);

    TildaContextMenu * context_menu = g_new0 (TildaContextMenu, 1);
    context_menu->tw = tw;
    context_menu->tt = tt;

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

    GActionEntry entries_for_regex [] = {
            {.name="copy-link", menu_copy_link_cb}
    };

    g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                     entries_for_tilda_window, G_N_ELEMENTS (entries_for_tilda_window), tw);
    g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                     entries_for_tilda_terminal, G_N_ELEMENTS (entries_for_tilda_terminal), tt);

    char * match = vte_terminal_match_check_event (VTE_TERMINAL (tt->vte_term), event, NULL);
    context_menu->match = g_strdup (match);

    g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                     entries_for_regex, G_N_ELEMENTS (entries_for_regex), context_menu->match);

    GAction *copyAction;

    copyAction = g_action_map_lookup_action (G_ACTION_MAP (action_group), "copy-link");

    if (context_menu->match == NULL) {
        g_simple_action_set_enabled (G_SIMPLE_ACTION (copyAction), FALSE);
    }

    gtk_widget_insert_action_group (tw->window, "window", G_ACTION_GROUP (action_group));

    GMenuModel *menu_model = G_MENU_MODEL (gtk_builder_get_object (builder, "menu"));
    GtkWidget *menu = gtk_menu_new_from_model (menu_model);

    gtk_menu_attach_to_widget (GTK_MENU (menu), tw->window, NULL);

    gtk_menu_set_accel_group (GTK_MENU (menu), tw->accel_group);
    gtk_menu_set_accel_path (GTK_MENU (menu), "<tilda>/context");

    /* Disable auto hide */
    tw->disable_auto_hide = TRUE;
    g_signal_connect (G_OBJECT (menu), "unmap", G_CALLBACK (on_popup_hide), context_menu);
    g_signal_connect (G_OBJECT (menu), "selection-done", G_CALLBACK (on_selection_done), context_menu);

    gtk_menu_popup_at_pointer (GTK_MENU (menu), event);

    g_object_unref (action_group);
    g_object_unref (builder);
}

static void
tilda_context_menu_free (TildaContextMenu * context_menu)
{
    g_free (context_menu->match);
    g_free (context_menu);
}
