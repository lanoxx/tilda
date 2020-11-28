/* vim: set ts=4 sts=4 sw=4 expandtab nowrap: */
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

#include "config.h"

#include <errno.h>

#include "debug.h"
#include "tilda.h"
#include "wizard.h"
#include "key_grabber.h"
#include "configsys.h"
#include "screen-size.h"
#include "tilda-palettes.h"
#include "tilda_window.h"
#include "tilda-keybinding.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <vte/vte.h> /* VTE_* constants, mostly */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

/* INT_MAX */
#include <limits.h>

struct TildaWizard_
{
    tilda_window            *tw;
    TildaKeybindingTreeView *keybinding;
    GtkBuilder              *builder;
};

typedef struct TildaWizard_ TildaWizard;

/* This will hold the GtkBuilder representation of the .ui file.
 * We keep this global so that we can look up any element from any routine.
 *
 * Note that for GtkBuilder to autoconnect signals, the functions that it hooks
 * to must NOT be marked static. I decided against the autoconnect just to
 * keep things "in the code" so that they can be grepped for easily. */
static GtkBuilder *xml = NULL;

/* Prototypes for use in the wizard() */
static void set_wizard_state_from_config (tilda_window *tw);
static void connect_wizard_signals (TildaWizard *wizard);
static void init_palette_scheme_menu (void);
static void update_palette_color_button(gint idx);
static void initialize_geometry_spinners(tilda_window *tw);

/* Show the wizard. This will show the wizard, then exit immediately. */
gint wizard (tilda_window *tw)
{
    DEBUG_FUNCTION ("wizard");
    DEBUG_ASSERT (tw != NULL);

    gchar *window_title;

    /* Make sure that there isn't already a wizard showing */
    if (tw->wizard_window) {
        gtk_window_present (GTK_WINDOW (tw->wizard_window));
        return 0;
    }

    TildaWizard *wizard = g_malloc (sizeof (TildaWizard));

    GError* error = NULL;
    xml = gtk_builder_new ();

#if ENABLE_NLS
    gtk_builder_set_translation_domain (xml, PACKAGE);
#endif

    if(!gtk_builder_add_from_resource (xml, "/org/tilda/tilda.ui", &error)) {
        g_prefix_error(&error, "Error:");
        return EXIT_FAILURE;
    }

    if (!xml) {
        g_warning ("problem while loading the tilda.ui file");
        return 2;
    }

    wizard->builder = xml;
    wizard->tw = tw;

    tw->wizard_window = GTK_WIDGET (
        gtk_builder_get_object (xml, "wizard_window")
    );

    /* GtkDialog windows need to have a transient parent or a warning will be logged. */
    gtk_window_set_transient_for (GTK_WINDOW(tw->wizard_window), GTK_WINDOW(tw->window));

    init_palette_scheme_menu ();

    /* Copy the current program state into the wizard */
    set_wizard_state_from_config (tw);

    wizard->keybinding = tilda_keybinding_init (wizard->builder);

    /* Connect all signal handlers. We do this after copying the state into
     * the wizard so that all of the handlers don't get called as we copy in
     * the values. This function manually connects the required signals for
     * all the widgets */
    connect_wizard_signals (wizard);

    /* Unbind the current keybinding. I'm aware that this opens up an opportunity to
     * let "someone else" grab the key, but it also saves us some trouble, and makes
     * validation easier. */
    tilda_keygrabber_unbind (config_getstr ("key"));

    /* Adding widget title for CSS selection */
    gtk_widget_set_name (GTK_WIDGET(tw->wizard_window), "Wizard");

    /* Set the icon for the wizard window to our tilda icon. */
    gchar* filename = g_build_filename (DATADIR, "pixmaps", "tilda.png", NULL);
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    g_free(filename);
    gtk_window_set_icon(GTK_WINDOW(tw->wizard_window), pixbuf);

    window_title = g_strdup_printf (_("Tilda %d Config"), tw->instance);
    gtk_window_set_title (GTK_WINDOW(tw->wizard_window), window_title);
    gtk_window_set_type_hint (GTK_WINDOW(tw->wizard_window), GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_widget_show_all (tw->wizard_window);

    /* This is needed to ensure that the wizard appears above of the terminal window */
    gtk_window_present(GTK_WINDOW(tw->wizard_window));

    g_free (window_title);

    /* Disable auto hide */
    tw->disable_auto_hide = TRUE;

    return 0;
}

# define GET_BUTTON_LABEL(BUTTON) ( gtk_button_get_label( GTK_BUTTON( \
    gtk_builder_get_object (xml, BUTTON))))

/* Gets called just after the wizard is closed. This should clean up after
 * the wizard, and do anything that couldn't be done immediately during the
 * wizard's lifetime. */
static void wizard_close_dialog (TildaWizard *wizard)
{
    DEBUG_FUNCTION ("wizard_close_dialog");

    tilda_window *tw = wizard->tw;

    const GtkWidget *entry_custom_command =
        GTK_WIDGET (gtk_builder_get_object(xml, "entry_custom_command"));
    const GtkWidget *wizard_window = tw->wizard_window;
    const gchar *command = gtk_entry_get_text (GTK_ENTRY(entry_custom_command));

    if (!tilda_keybinding_save (wizard->keybinding, tw)) {
        return;
    }

    tilda_keybinding_apply (wizard->keybinding);

    tilda_keybinding_free (wizard->keybinding);

    wizard->keybinding = NULL;

    /* TODO: validate this?? */
    config_setstr ("command", command);

    /* Free the GtkBuilder data structure */
    g_object_unref (G_OBJECT(xml));
    xml = NULL;

    /* Remove the wizard */
    gtk_widget_destroy (GTK_WIDGET(wizard_window));
    tw->wizard_window = NULL;

    /* Write the config, because it probably changed. This saves us in case
     * of an XKill (or crash) later ... */
    config_write (tw->config_file);

    /* Enables auto hide */
    tw->disable_auto_hide = FALSE;

    wizard->tw = NULL;
    wizard->builder = NULL;
    g_free (wizard);
}

/******************************************************************************/
/*                       Wizard set-up functions                              */
/******************************************************************************/

/* The following macro definitions are used to make the process of loading the
 * configuration values a lot easier. Each macro takes the name of a GtkBuilder object
 * and the name of a configuration option and sets the value for that widget to the value
 * that is stored in this configuration option. The macros are mainly used in the function
 * set_wizard_state_from_config() but they are used in some callback functions as well, so
 * we need to define before the callback functions.
 */

/** Setters */
#define CHECK_BUTTON(GLADE_NAME,CFG_BOOL) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getbool ((CFG_BOOL)))
#define COMBO_BOX(GLADE_NAME,CFG_INT) gtk_combo_box_set_active (GTK_COMBO_BOX( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getint ((CFG_INT)))
#define FONT_BUTTON(GLADE_NAME,CFG_STR) gtk_font_chooser_set_font (GTK_FONT_CHOOSER( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define TEXT_ENTRY(GLADE_NAME,CFG_STR) gtk_entry_set_text (GTK_ENTRY( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define BUTTON_LABEL_FROM_CFG(GLADE_NAME,CFG_STR) gtk_button_set_label (GTK_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getstr ((CFG_STR)))
#define SPIN_BUTTON(GLADE_NAME,CFG_INT) gtk_spin_button_set_value (GTK_SPIN_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), config_getint ((CFG_INT)))
#define SPIN_BUTTON_SET_RANGE(GLADE_NAME,LOWER,UPPER) gtk_spin_button_set_range (GTK_SPIN_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), (LOWER), (UPPER))
#define SPIN_BUTTON_SET_VALUE(GLADE_NAME,VALUE) gtk_spin_button_set_value (GTK_SPIN_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), (VALUE))
#define FILE_BUTTON(GLADE_NAME, FILENAME) gtk_file_chooser_set_filename (GTK_FILE_CHOOSER( \
    gtk_builder_get_object (xml, (GLADE_NAME))), FILENAME)
#define COLOR_CHOOSER(GLADE_NAME,COLOR) gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER( \
    gtk_builder_get_object (xml, (GLADE_NAME))), (COLOR))
#define SET_SENSITIVE_BY_CONFIG_BOOL(GLADE_NAME,CFG_BOOL) gtk_widget_set_sensitive ( \
    GTK_WIDGET (gtk_builder_get_object (xml, (GLADE_NAME))), config_getbool ((CFG_BOOL)))
#define SET_SENSITIVE_BY_CONFIG_NBOOL(GLADE_NAME,CFG_BOOL) gtk_widget_set_sensitive ( \
    GTK_WIDGET (gtk_builder_get_object (xml, (GLADE_NAME))), !config_getbool ((CFG_BOOL)))

/** Getters */
#define SPIN_BUTTON_GET_VALUE(GLADE_NAME) gtk_spin_button_get_value (GTK_SPIN_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))))
#define SPIN_BUTTON_GET_RANGE(GLADE_NAME,MIN_POINTER,MAX_POINTER) gtk_spin_button_get_range (GTK_SPIN_BUTTON( \
    gtk_builder_get_object (xml, (GLADE_NAME))), MIN_POINTER, MAX_POINTER)

/******************************************************************************/
/*      Utility functions to get the current monitors height and width        */
/******************************************************************************/
static int get_max_height() {
    gdouble height_min;
    gdouble height_max;
    SPIN_BUTTON_GET_RANGE("spin_height_pixels", &height_min, &height_max);
    return (int) height_max;
}

static int get_max_width() {
    gdouble width_min;
    gdouble width_max;
    SPIN_BUTTON_GET_RANGE("spin_width_pixels", &width_min, &width_max);
    return (int) width_max;
}

/******************************************************************************/
/*               ALL static callback helpers are below                        */
/******************************************************************************/

/**
 * Get the number of screens and load the monitor geometry for each screen,
 * then set the position of the window according to the x and y offset
 * of that monitor. This function does not actually move or resize the window
 * but only changes the value of the spin buttons. The moving and resizing
 * is then done by the callback functions of the respective widgets.
 */
static int
combo_monitor_selection_changed_cb (GtkWidget* widget, tilda_window *tw)
{
    DEBUG_FUNCTION ("combo_monitor_selection_changed_cb");

    GdkDisplay *display          = gdk_display_get_default ();
    GdkMonitor *original_monitor = tilda_window_find_monitor_number (tw);
    GdkMonitor *new_monitor;

    GdkRectangle original_monitor_rectangle;
    GdkRectangle selected_monitor_rectangle;

    gdk_monitor_get_workarea (original_monitor, &original_monitor_rectangle);

    GtkTreeIter active_iter;

    GtkComboBox* combo_choose_monitor = GTK_COMBO_BOX(widget);

    if(!gtk_combo_box_get_active_iter(combo_choose_monitor, &active_iter))
    {
        return FALSE;
    }

    gchar* new_monitor_name = NULL;
    gint new_monitor_number;

    gtk_tree_model_get(gtk_combo_box_get_model(combo_choose_monitor), &active_iter,
                       0, &new_monitor_name,
                       1, &new_monitor_number,
                       -1);

    new_monitor = gdk_display_get_monitor (display, new_monitor_number);
    gdk_monitor_get_workarea (new_monitor, &selected_monitor_rectangle);

    //Save the new monitor value
    config_setstr("show_on_monitor", new_monitor_name);

    /* The dimensions of the monitor might have changed,
     * so we need to update the spinner widgets for height,
     * width, and their percentages as well as their ranges.
     * Keep in mind that we use the max range of the pixel spinners
     * to store the size of the screen.
     */
    GdkRectangle rectangle;
    config_get_configured_window_size (&rectangle);

    if(selected_monitor_rectangle.width != original_monitor_rectangle.width)
    {
        gint new_max_width = selected_monitor_rectangle.width;

        SPIN_BUTTON_SET_RANGE ("spin_width_pixels", 0, new_max_width);
        SPIN_BUTTON_SET_VALUE ("spin_width_pixels", rectangle.width);

        gtk_window_resize (GTK_WINDOW(tw->window),
                           rectangle.width,
                           rectangle.height);
    }

    if(selected_monitor_rectangle.height != original_monitor_rectangle.height)
    {
        int new_max_height = selected_monitor_rectangle.height;

        SPIN_BUTTON_SET_RANGE ("spin_height_pixels", 0, new_max_height);
        SPIN_BUTTON_SET_VALUE ("spin_height_pixels", rectangle.height);

        gtk_window_resize (GTK_WINDOW(tw->window),
                           rectangle.width,
                           rectangle.height);
    }

    gint screen_width, screen_height;
    screen_size_get_dimensions (&screen_width, &screen_height);
    SPIN_BUTTON_SET_RANGE ("spin_x_position", 0, screen_width);
    SPIN_BUTTON_SET_VALUE("spin_x_position", selected_monitor_rectangle.x);
    SPIN_BUTTON_SET_RANGE ("spin_y_position", 0, screen_height);
    SPIN_BUTTON_SET_VALUE("spin_y_position", selected_monitor_rectangle.y);

    tilda_window_update_window_position (tw);

    return GDK_EVENT_STOP;
}

static void window_title_change_all (tilda_window *tw)
{
    DEBUG_FUNCTION ("window_title_change_all");

    GtkWidget *page;
    GtkWidget *label;
    tilda_term *tt;
    gchar *title;
    gint i, size, list_count;

    size = gtk_notebook_get_n_pages (GTK_NOTEBOOK(tw->notebook));
    list_count = size-1;

    for (i=0;i<size;i++,list_count--)
    {
        tt = g_list_nth (tw->terms, list_count)->data;
        title = tilda_terminal_get_title (tt);
        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook), i);
        label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (tw->notebook), page);

        guint length = config_getint ("title_max_length");
        guint title_behaviour = config_getint("title_behaviour");
        if(title_behaviour && strlen(title) > length) {
            gchar *shortTitle = NULL;
            if(title_behaviour == 1) {
                shortTitle = g_strdup_printf ("%.*s...", length, title);
            }
            else {
                gchar *titleOffset = title + strlen(title) - length;
                shortTitle = g_strdup_printf ("...%s", titleOffset);
            }
            gtk_label_set_text (GTK_LABEL(label), shortTitle);
            g_free(shortTitle);
        } else {
            gtk_label_set_text (GTK_LABEL(label), title);
        }
        if(config_getbool("show_title_tooltip"))
          gtk_widget_set_tooltip_text(label, title);
        else
          gtk_widget_set_tooltip_text(label, "");

        g_free (title);
    }
}

static void set_spin_value_while_blocking_callback (GtkSpinButton *spin,
                                                    void (*callback)(GtkWidget *w, tilda_window *tw),
                                                    gdouble new_val,
                                                    tilda_window *tw)
{
    DEBUG_FUNCTION ("set_spin_value_while_blocking_callback");

    g_signal_handlers_block_by_func (spin, G_CALLBACK(*callback), tw);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin), new_val);
    g_signal_handlers_unblock_by_func (spin, G_CALLBACK(*callback), tw);
}

/******************************************************************************/
/*                       ALL Callbacks are below                              */
/******************************************************************************/

static void wizard_button_close_clicked_cb (GtkButton   *button,
                                            TildaWizard *wizard)
{
    /* Call the clean-up function */
    wizard_close_dialog (wizard);
}

static void wizard_window_delete_event_cb (GtkWidget   *widget,
                                           GdkEvent    *event,
                                           TildaWizard *wizard)
{
    /* Call the clean-up function */
    wizard_close_dialog (wizard);
}

static void check_display_on_all_workspaces_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("pinned", status);

    if (status)
        gtk_window_stick (GTK_WINDOW (tw->window));
    else
        gtk_window_unstick (GTK_WINDOW (tw->window));
}

static void check_set_as_desktop_toggled_cb (GtkWidget *widget, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
    GtkWidget *check_display_on_all_workspaces = GTK_WIDGET (gtk_builder_get_object (xml, "check_display_on_all_workspaces"));
    config_setbool ("set_as_desktop", status);

    g_signal_handlers_block_by_func (check_display_on_all_workspaces, check_display_on_all_workspaces_toggled_cb, NULL);
    gboolean status_display_on_all_workspaces = config_getbool ("pinned");
    if (status) {

        gtk_widget_set_sensitive (check_display_on_all_workspaces, FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_display_on_all_workspaces), TRUE);
        gtk_window_stick (GTK_WINDOW (tw->window));
    } else {
        gtk_widget_set_sensitive (check_display_on_all_workspaces, TRUE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_display_on_all_workspaces), status_display_on_all_workspaces);
        gtk_window_unstick (GTK_WINDOW (tw->window));
    }
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), status);
    g_signal_handlers_unblock_by_func (check_display_on_all_workspaces, check_display_on_all_workspaces_toggled_cb, NULL);
}

static void check_do_not_show_in_taskbar_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("notaskbar", status);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tw->window), status);
}

static void check_show_notebook_border_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("notebook_border", status);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook), status);
}

static void check_always_on_top_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("above", status);
    gtk_window_set_keep_above (GTK_WINDOW (tw->window), status);
}


static void check_start_tilda_hidden_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("hidden", status);
}

static void check_terminal_bell_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    guint i;
    tilda_term *tt;

    config_setbool ("bell", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_audible_bell (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void check_cursor_blinks_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    guint i;
    tilda_term *tt;

    config_setbool ("blinks", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_cursor_blink_mode (VTE_TERMINAL(tt->vte_term),
                (status)?VTE_CURSOR_BLINK_ON:VTE_CURSOR_BLINK_OFF);
    }
}

static void spin_auto_hide_time_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint auto_hide_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));
    config_setint ("auto_hide_time", auto_hide_time);
    tw->auto_hide_max_time = auto_hide_time;
}

static void check_auto_hide_on_focus_lost_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("auto_hide_on_focus_lost", status);
    tw->auto_hide_on_focus_lost = status;
}

static void check_auto_hide_on_mouse_leave_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("auto_hide_on_mouse_leave", status);
    tw->auto_hide_on_mouse_leave = status;
}

static void combo_cursor_shape_changed_cb(GtkWidget *w, tilda_window *tw)
{
    guint i;
    tilda_term *tt;
    gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    if (status < 0 || status > 2) {
        DEBUG_ERROR ("Invalid Cursor Type");
        g_printerr (_("Invalid Cursor Type, resetting to default\n"));
        status = 0;
    }
    config_setint("cursor_shape", (VteCursorShape) status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_cursor_shape (VTE_TERMINAL(tt->vte_term),
                                       (VteCursorShape) status);
    }
}

static void combo_non_focus_pull_up_behaviour_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    if (status < 0 || status > 1) {
        DEBUG_ERROR ("Non-focus pull up behaviour invalid");
        g_printerr (_("Invalid non-focus pull up behaviour, ignoring\n"));
        return;
    }

    if(1 == status) {
        tw->hide_non_focused = TRUE;
    }
    else {
        tw->hide_non_focused = FALSE;
    }

    config_setint ("non_focus_pull_up_behaviour", status);
}

static void button_font_font_set_cb (GtkWidget *w, tilda_window *tw)
{
    const gchar *font = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (w));
    guint i;
    tilda_term *tt;

    config_setstr ("font", font);
    PangoFontDescription *description = pango_font_description_from_string (font);
    tw->unscaled_font_size = pango_font_description_get_size(description);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_font (VTE_TERMINAL(tt->vte_term), description);
        tilda_term_adjust_font_scale(tt, tw->current_scale_factor);
    }
}

static void entry_title_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gchar *title = gtk_entry_get_text (GTK_ENTRY(w));

    config_setstr ("title", title);
    window_title_change_all (tw);
}

static void combo_dynamically_set_title_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("d_set_title", status);
    window_title_change_all (tw);
}

static void combo_title_behaviour_changed_cb (GtkWidget *w, tilda_window *tw)
{
    DEBUG_FUNCTION ("combo_title_behaviour_changed_cb");

    GtkWidget *entry = GTK_WIDGET(
        gtk_builder_get_object (xml, ("spin_title_max_length"))
    );
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    config_setint("title_behaviour", status);
    if (status > 0) {
        gtk_widget_set_sensitive (entry, TRUE);
    } else {
        gtk_widget_set_sensitive (entry, FALSE);
    }
}

static void spin_max_title_length_changed_cb (GtkWidget *w, tilda_window *tw)
{
    DEBUG_FUNCTION ("spin_max_title_length_changed_cb");

    int length = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w));

    config_setint ("title_max_length", length);
    window_title_change_all (tw);
}

static void check_run_custom_command_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    GtkWidget *label_custom_command;
    GtkWidget *entry_custom_command;
    GtkWidget *check_command_login_shell;

    config_setbool ("run_command", status);

    label_custom_command =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_custom_command"));
    entry_custom_command =
        GTK_WIDGET (gtk_builder_get_object (xml, "entry_custom_command"));
    check_command_login_shell =
        GTK_WIDGET (gtk_builder_get_object (xml, "check_command_login_shell"));

    gtk_widget_set_sensitive (label_custom_command, status);
    gtk_widget_set_sensitive (entry_custom_command, status);
    gtk_widget_set_sensitive (check_command_login_shell, !status);

    if(!status) {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry_custom_command),
                GTK_ENTRY_ICON_SECONDARY,
                NULL);
    }

    gtk_widget_grab_focus(entry_custom_command);
}


/**
 * Check that the value entered into the GtkEntry is a valid command
 * which is accessible from the users PATH environment variable.
 * If the command is not found on the users PATH then a small error
 * icon is displayed on the end of the entry.
 * This function can only be registered to Widgets of (sub)type GtkEntry
 */
static void validate_executable_command_cb (GtkWidget *w,
                                            G_GNUC_UNUSED GdkEvent *event,
                                            G_GNUC_UNUSED tilda_window *tw)
{
    g_return_if_fail(w != NULL && GTK_IS_ENTRY(w));
    const char* command = gtk_entry_get_text (GTK_ENTRY(w));
    /* Check that the command exists */
    int argc = 0;
    gchar** argv = NULL;
    GError *error = NULL;
    gboolean success = g_shell_parse_argv(command, &argc, &argv, &error);
    char *command_filename = NULL;
    if(success && argc > 0) {
        command_filename = g_find_program_in_path(argv[0]);
    }
    g_strfreev(argv);

    if (command_filename == NULL && gtk_widget_is_sensitive(w)) {
        //wrong command
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(w),
                GTK_ENTRY_ICON_SECONDARY, "dialog-error");
        gtk_entry_set_icon_tooltip_text(GTK_ENTRY(w),
                GTK_ENTRY_ICON_SECONDARY,
                "The command you have entered is not a valid command.\n"
                        "Make sure that the specified executable is in your PATH environment variable."
        );
    } else {
        gtk_entry_set_icon_from_icon_name(GTK_ENTRY(w),
                GTK_ENTRY_ICON_SECONDARY,
                NULL);
        free(command_filename);
    }
}

static void check_confirm_close_tab_cb (GtkWidget *w,
                                        G_GNUC_UNUSED tilda_window *tw)
{
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool ("confirm_close_tab", active);
}

static void combo_command_exit_changed_cb (GtkWidget *w, tilda_window *tw) {
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("command_exit", status);
}

static void check_command_login_shell_cb (GtkWidget *w, tilda_window *tw) {
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("command_login_shell", active);
}

static void check_control_activates_match_cb (GtkWidget *w, tilda_window *tw) {
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("control_activates_match", active);
}

static void update_custom_web_browser_sensitivity () {
    gboolean match_web_uris = config_getbool("match_web_uris");
    gboolean use_custom_web_browser = config_getbool("use_custom_web_browser");
    gboolean sensitive = FALSE;

    GtkWidget * check_custom_web_browser;
    GtkWidget * label_web_browser;
    GtkWidget * entry_web_browser;

    check_custom_web_browser =
            GTK_WIDGET (gtk_builder_get_object (xml, "check_custom_web_browser"));
    label_web_browser =
            GTK_WIDGET (gtk_builder_get_object (xml, "label_web_browser"));
    entry_web_browser =
            GTK_WIDGET (gtk_builder_get_object (xml, "entry_web_browser"));

    gtk_widget_set_sensitive(check_custom_web_browser, match_web_uris);

    sensitive = match_web_uris && use_custom_web_browser;

    gtk_widget_set_sensitive (label_web_browser, sensitive);
    gtk_widget_set_sensitive (entry_web_browser, sensitive);
}

static void check_match_web_uris_cb (GtkWidget *w, tilda_window *tw) {
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("match_web_uris", active);

    update_custom_web_browser_sensitivity ();

    for (guint i=0; i < g_list_length (tw->terms); i++) {
        tilda_term * tt = g_list_nth_data (tw->terms, i);
        tilda_terminal_update_matches(tt);
    }
}

static void check_custom_web_browser_cb (GtkWidget *w, tilda_window *tw) {
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("use_custom_web_browser", active);

    update_custom_web_browser_sensitivity ();
}

static void check_match_file_uris_cb (GtkWidget *w, tilda_window *tw) {
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("match_file_uris", active);

    for (guint i=0; i < g_list_length (tw->terms); i++) {
        tilda_term * tt = g_list_nth_data (tw->terms, i);
        tilda_terminal_update_matches(tt);
    }
}

static void check_match_email_addresses_cb (GtkWidget *w, tilda_window *tw) {
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("match_email_addresses", active);

    for (guint i=0; i < g_list_length (tw->terms); i++) {
        tilda_term * tt = g_list_nth_data (tw->terms, i);
        tilda_terminal_update_matches(tt);
    }
}

static void check_match_numbers_cb (GtkWidget *w, tilda_window *tw) {
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("match_numbers", active);

    for (guint i=0; i < g_list_length (tw->terms); i++) {
        tilda_term * tt = g_list_nth_data (tw->terms, i);
        tilda_terminal_update_matches(tt);
    }
}

static void check_start_fullscreen_cb(GtkWidget *w, tilda_window *tw) {
    const gboolean start_fullscreen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("start_fullscreen", start_fullscreen);
}

static void combo_on_last_terminal_exit_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("on_last_terminal_exit", status);
}

static void check_prompt_on_exit_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean prompt_on_exit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("prompt_on_exit", prompt_on_exit);
}

static void check_show_title_tooltip_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean show_title_tooltip = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("show_title_tooltip", show_title_tooltip);
    window_title_change_all (tw);
}

static void entry_web_browser_changed (GtkWidget *w, tilda_window *tw) {
    const gchar *web_browser = gtk_entry_get_text (GTK_ENTRY(w));

    config_setstr ("web_browser", web_browser);
}

static void entry_word_chars_changed (GtkWidget *w, tilda_window *tw)
{
    guint i;
    tilda_term *tt;
    const gchar *word_chars = gtk_entry_get_text (GTK_ENTRY(w));

    /* restore to default value if user clears this setting */
    if (NULL == word_chars || '\0' == word_chars[0])
        word_chars = DEFAULT_WORD_CHARS;

    config_setstr ("word_chars", word_chars);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_word_char_exceptions (VTE_TERMINAL (tt->vte_term), word_chars);
    }
}

/*
 * Prototypes for the next 4 functions.
 */
//Both height functions depend on each other
static void spin_height_percentage_value_changed_cb (GtkWidget *w, tilda_window *tw);
static void spin_height_pixels_value_changed_cb (GtkWidget *w, tilda_window *tw);
//Both width functions depend on each other
static void spin_width_percentage_value_changed_cb (GtkWidget *w, tilda_window *tw);
static void spin_width_pixels_value_changed_cb (GtkWidget *w, tilda_window *tw);

static void initialize_scrollback_settings(void);
static void initialize_set_as_desktop_checkbox (void);

static void spin_height_percentage_value_changed_cb (GtkWidget *spin_height_percentage,
                                                     tilda_window *tw)
{
    DEBUG_FUNCTION ("spin_height_percentage_value_changed_cb");

    const GtkWidget *spin_height_pixels =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_height_pixels"));

    const gdouble height_percentage = gtk_spin_button_get_value (GTK_SPIN_BUTTON(spin_height_percentage)) / 100;
    const gint height_pixels = pixels_ratio_to_absolute (get_max_height(), height_percentage);

    config_setint ("height_percentage", GLONG_FROM_DOUBLE (height_percentage));

    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_height_pixels),
                                            &spin_height_pixels_value_changed_cb,
                                            height_pixels, tw);

    GdkRectangle rectangle;
    config_get_configured_window_size (&rectangle);

    gtk_window_resize (GTK_WINDOW(tw->window), rectangle.width, height_pixels);

    if (config_getbool ("centered_vertically"))
    {
        config_setint ("y_pos", tilda_window_find_centering_coordinate (tw, HEIGHT));

        gtk_window_move (GTK_WINDOW(tw->window),
                         config_getint ("x_pos"),
                         config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}


static void spin_height_pixels_value_changed_cb (GtkWidget *spin_height_pixels,
                                                 tilda_window *tw)
{
    DEBUG_FUNCTION ("spin_height_pixels_value_changed_cb");

    const GtkWidget *spin_height_percentage =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_height_percentage"));
    const gint height_pixels = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_height_pixels));
    const gdouble height_percentage = pixels_absolute_to_ratio (get_max_height(), height_pixels);

    config_setint ("height_percentage", GLONG_FROM_DOUBLE (height_percentage));

    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_height_percentage),
                                            &spin_height_percentage_value_changed_cb,
                                            height_percentage * 100, tw);

    GdkRectangle rectangle;
    config_get_configured_window_size (&rectangle);

    gtk_window_resize (GTK_WINDOW(tw->window), rectangle.width, height_pixels);

    if (config_getbool ("centered_vertically"))
    {
        config_setint ("y_pos", tilda_window_find_centering_coordinate (tw, HEIGHT));

        gtk_window_move (GTK_WINDOW(tw->window),
                         config_getint ("x_pos"),
                         config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_width_percentage_value_changed_cb (GtkWidget *spin_width_percentage,
                                                    tilda_window *tw)
{
    DEBUG_FUNCTION ("spin_width_percentage_value_changed_cb");

    const GtkWidget *spin_width_pixels =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_width_pixels"));

    const gdouble width_percentage = gtk_spin_button_get_value (GTK_SPIN_BUTTON(spin_width_percentage)) / 100;
    const gint width_pixels = pixels_ratio_to_absolute (get_max_width(), width_percentage);

    config_setint ("width_percentage", GLONG_FROM_DOUBLE(width_percentage));

    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_width_pixels),
                                            &spin_width_pixels_value_changed_cb,
                                            width_pixels, tw);

    GdkRectangle rectangle;
    config_get_configured_window_size (&rectangle);

    gtk_window_resize (GTK_WINDOW(tw->window), width_pixels, rectangle.height);

    if (config_getbool ("centered_horizontally"))
    {
        config_setint ("x_pos", tilda_window_find_centering_coordinate (tw, WIDTH));

        gtk_window_move (GTK_WINDOW(tw->window),
                         config_getint ("x_pos"),
                         config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_width_pixels_value_changed_cb (GtkWidget *spin_width_pixels, tilda_window *tw)
{
    DEBUG_FUNCTION ("spin_width_pixels_value_changed_cb");

    const GtkWidget *spin_width_percentage =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_width_percentage"));

    const gint width_pixels = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_width_pixels));
    const gdouble width_percentage = pixels_absolute_to_ratio (get_max_width(), width_pixels);

    config_setint ("width_percentage", GLONG_FROM_DOUBLE(width_percentage));

    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_width_percentage),
                                            &spin_width_percentage_value_changed_cb,
                                            width_percentage * 100, tw);

    GdkRectangle rectangle;
    config_get_configured_window_size (&rectangle);

    gtk_window_resize (GTK_WINDOW(tw->window), width_pixels, rectangle.height);

    if (config_getbool ("centered_horizontally"))
    {
        config_setint ("x_pos", tilda_window_find_centering_coordinate (tw, WIDTH));

        gtk_window_move (GTK_WINDOW(tw->window),
                         config_getint ("x_pos"),
                         config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void check_centered_horizontally_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    DEBUG_FUNCTION ("check_centered_horizontally_toggled_cb");

    const gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_x_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_x_position"));
    const GtkWidget *spin_x_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_x_position"));

    config_setbool ("centered_horizontally", active);

    if (active)
        config_setint ("x_pos", tilda_window_find_centering_coordinate (tw, WIDTH));
    else
        config_setint ("x_pos", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_x_position)));

    gtk_widget_set_sensitive (GTK_WIDGET(label_x_position), !active);
    gtk_widget_set_sensitive (GTK_WIDGET(spin_x_position), !active);

    gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_x_position_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    DEBUG_FUNCTION ("spin_x_position_value_changed_cb");

    const gint x_pos = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint y_pos = config_getint ("y_pos");

    config_setint ("x_pos", x_pos);
    gtk_window_move (GTK_WINDOW(tw->window), x_pos, y_pos);

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void check_centered_vertically_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    DEBUG_FUNCTION ("check_centered_vertically_toggled_cb");

    const gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_y_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_y_position"));
    const GtkWidget *spin_y_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_y_position"));

    config_setbool ("centered_vertically", active);

    if (active)
        config_setint ("y_pos", tilda_window_find_centering_coordinate (tw, HEIGHT));
    else
        config_setint ("y_pos", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin_y_position)));

    gtk_widget_set_sensitive (GTK_WIDGET(label_y_position), !active);
    gtk_widget_set_sensitive (GTK_WIDGET(spin_y_position), !active);

    gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_y_position_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    DEBUG_FUNCTION ("spin_y_position_value_changed_cb");

    const gint x_pos = config_getint ("x_pos");
    const gint y_pos = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));

    config_setint ("y_pos", y_pos);
    gtk_window_move (GTK_WINDOW(tw->window), x_pos, y_pos);

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void combo_tab_pos_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    const GtkPositionType positions[] = {
                             GTK_POS_TOP,
                             GTK_POS_BOTTOM,
                             GTK_POS_LEFT,
                             GTK_POS_RIGHT };

    if (status < 0 || status > 4) {
        DEBUG_ERROR ("Notebook tab position invalid");
        g_printerr (_("Invalid tab position setting, ignoring\n"));
        return;
    }

    config_setint ("tab_pos", status);

    if(NB_HIDDEN == status) {
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK(tw->notebook), FALSE);
    }
    else {
        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK (tw->notebook)) > 1) {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(tw->notebook), TRUE);
        }
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK(tw->notebook), positions[status]);
    }
}

static void check_expand_tabs_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("expand_tabs", status);

    int page = 0;
    GtkWidget *child = NULL;
    while((child = gtk_notebook_get_nth_page(GTK_NOTEBOOK (tw->notebook),
                                             page++)))
    {
        gtk_container_child_set (GTK_CONTAINER(tw->notebook),
            child,
            "tab-expand", status,
            "tab-fill", status,
            NULL);
    }
}

static void check_show_single_tab_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("show_single_tab", status);

    /* Only need to do something if the current number of tabs is 1 */
    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) == 1)
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), status);
}

static void check_enable_transparency_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    const GtkWidget *label_level_of_transparency =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_level_of_transparency"));
    const GtkWidget *spin_level_of_transparency =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_level_of_transparency"));

    gtk_widget_set_sensitive (GTK_WIDGET(label_level_of_transparency), status);
    gtk_widget_set_sensitive (GTK_WIDGET(spin_level_of_transparency), status);

    tilda_window_toggle_transparency(tw);
}

static void spin_level_of_transparency_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    guint i;
    tilda_term *tt;
    GdkRGBA bg;

    bg.red   =    GUINT16_TO_FLOAT(config_getint ("back_red"));
    bg.green =    GUINT16_TO_FLOAT(config_getint ("back_green"));
    bg.blue  =    GUINT16_TO_FLOAT(config_getint ("back_blue"));
    bg.alpha =    1.0 - (status / 100.0);

    config_setint ("back_alpha", GUINT16_FROM_FLOAT (bg.alpha));
    for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_color_background(VTE_TERMINAL(tt->vte_term), &bg);
        }
}

static void spin_animation_delay_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));

    config_setint ("slide_sleep_usec", status);
}

static void combo_animation_orientation_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("animation_orientation", status);
    generate_animation_positions (tw);
}

static void check_animated_pulldown_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_animation_delay =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_animation_delay"));
    const GtkWidget *spin_animation_delay =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_animation_delay"));
    const GtkWidget *label_animation_orientation =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_animation_orientation"));
    const GtkWidget *combo_animation_orientation =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_animation_orientation"));

    gtk_widget_set_sensitive (GTK_WIDGET(label_animation_delay), status);
    gtk_widget_set_sensitive (GTK_WIDGET(spin_animation_delay), status);
    gtk_widget_set_sensitive (GTK_WIDGET(label_animation_orientation), status);
    gtk_widget_set_sensitive (GTK_WIDGET(combo_animation_orientation), status);

    config_setbool ("animation", status);

    /* If we just disabled animation, we have to reset the window size to the normal
     * size, since the animations change the size of the window, and pull() does nothing more
     * than show and place the window. */
    if (!status)
    {
        GdkRectangle rectangle;
        config_get_configured_window_size (&rectangle);

        guint width = rectangle.width;
        guint height = rectangle.height;

        gtk_window_resize (GTK_WINDOW(tw->window), width, height);
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Avoids a nasty looking glitch if you switch on animation while the window is
     * hidden. It will briefly show at full size, then shrink to the first animation
     * position. From there it works fine. */
    if (status && tw->current_state == STATE_UP)
    {
        /* I don't know why, but width=0, height=0 doesn't work. Width=1, height=1 works
         * exactly as expected, so I'm leaving it that way. */
        gtk_window_resize (GTK_WINDOW(tw->window), 1, 1);
    }
}

static void combo_colorschemes_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint scheme = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    const GtkWidget *colorbutton_text =
        GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_text"));
    const GtkWidget *colorbutton_back =
        GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_back"));
    GdkRGBA gdk_text, gdk_back;

    tilda_term *tt;
    guint i;
    gboolean nochange = FALSE;

    config_setint ("scheme", scheme);

    gdk_text.alpha = 1.0;
    gdk_back.alpha = 1.0;

    switch (scheme) {
        /* Green on black */
        case 1:
            gdk_text.red = gdk_text.blue = 0.0;
            gdk_text.green = 1.0;
            gdk_back.red = gdk_back.green = gdk_back.blue = 0.0;
            break;
        /* Black on white */
        case 2:
            gdk_text.red = gdk_text.green = gdk_text.blue = 0.0;
            gdk_back.red = gdk_back.green = gdk_back.blue = 1.0;
            break;
        /* White on black */
        case 3:
            gdk_text.red = gdk_text.green = gdk_text.blue = 1.0;
            gdk_back.red = gdk_back.green = gdk_back.blue = 0.0;
            break;
        /* Zenburn */
        case 4:
            gdk_text.red = 0.86;
            gdk_text.green = gdk_text.blue = 0.64;
            gdk_back.red = gdk_back.green = gdk_back.blue = 0.25;
            break;
        /* Solarized Light */
        case 5:
            gdk_text.red = 0.4;
            gdk_text.green = 0.48;
            gdk_text.blue = 0.51;
            gdk_back.red = 0.99;
            gdk_back.green = 0.96;
            gdk_back.blue = 0.89;
            break;
        /* Solarized Dark */
        case 6:
            gdk_text.red = 0.51;
            gdk_text.green = 0.58;
            gdk_text.blue = 0.59;
            gdk_back.red = 0.0;
            gdk_back.green = 0.17;
            gdk_back.blue = 0.21;
            break;
        /* Snazzy */
        case 7:
            gdk_text.red = 0.94;
            gdk_text.green = 0.94;
            gdk_text.blue = 0.92;
            gdk_back.red = 0.16;
            gdk_back.green = 0.16;
            gdk_back.blue = 0.21;
            break;
        /* Custom */
        default:
            nochange = TRUE;
            break;
    }

    /* If we switched to "Custom", then don't do anything. Let the user continue
     * from the current color choice. */
    if (!nochange) {
        config_setint ("back_red",   GUINT16_FROM_FLOAT(gdk_back.red));
        config_setint ("back_green", GUINT16_FROM_FLOAT(gdk_back.green));
        config_setint ("back_blue",  GUINT16_FROM_FLOAT(gdk_back.blue));
        config_setint ("text_red",   GUINT16_FROM_FLOAT(gdk_text.red));
        config_setint ("text_green", GUINT16_FROM_FLOAT(gdk_text.green));
        config_setint ("text_blue",  GUINT16_FROM_FLOAT(gdk_text.blue));

        gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER(colorbutton_text), &gdk_text);
        gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER(colorbutton_back), &gdk_back);

        for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_color_foreground (VTE_TERMINAL(tt->vte_term),
                                               &gdk_text);
            vte_terminal_set_color_background (VTE_TERMINAL(tt->vte_term),
                                               &gdk_back);
        }

        tilda_window_refresh_transparency(tw);
    }
}
static void colorbutton_cursor_color_set_cb (GtkWidget *w, tilda_window *tw)
{
    const GtkWidget *combo_colorschemes =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_colorschemes"));

    guint i;
    tilda_term *tt;
    GdkRGBA gdk_cursor_color;

    /* The user just changed colors manually, so set the scheme to "Custom" */
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_colorschemes), 0);
    config_setint ("scheme", 0);

    /* Now get the color that was set, save it, then set it */
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(w), &gdk_cursor_color);
    config_setint ("cursor_red", GUINT16_FROM_FLOAT(gdk_cursor_color.red));
    config_setint ("cursor_green", GUINT16_FROM_FLOAT(gdk_cursor_color.green));
    config_setint ("cursor_blue", GUINT16_FROM_FLOAT(gdk_cursor_color.blue));

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_color_cursor (VTE_TERMINAL(tt->vte_term),
                                       &gdk_cursor_color);
    }
}

static void colorbutton_text_color_set_cb (GtkWidget *w, tilda_window *tw)
{
    const GtkWidget *combo_colorschemes =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_colorschemes"));

    guint i;
    tilda_term *tt;
    GdkRGBA gdk_text_color;

    /* The user just changed colors manually, so set the scheme to "Custom" */
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_colorschemes), 0);
    config_setint ("scheme", 0);

    /* Now get the color that was set, save it, then set it */
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(w), &gdk_text_color);
    config_setint ("text_red",   GUINT16_FROM_FLOAT(gdk_text_color.red));
    config_setint ("text_green", GUINT16_FROM_FLOAT(gdk_text_color.green));
    config_setint ("text_blue",  GUINT16_FROM_FLOAT(gdk_text_color.blue));

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_color_foreground (VTE_TERMINAL(tt->vte_term),
                                           &gdk_text_color);
    }
}

static void colorbutton_back_color_set_cb (GtkWidget *w, tilda_window *tw)
{
    const GtkWidget *combo_colorschemes =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_colorschemes"));
    guint i;
    tilda_term *tt;
    GdkRGBA gdk_back_color;

    /* The user just changed colors manually, so set the scheme to "Custom" */
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_colorschemes), 0);
    config_setint ("scheme", 0);

    /* Now get the color that was set, save it, then set it */
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(w), &gdk_back_color);
    config_setint ("back_red",   GUINT16_FROM_FLOAT(gdk_back_color.red));
    config_setint ("back_green", GUINT16_FROM_FLOAT(gdk_back_color.green));
    config_setint ("back_blue",  GUINT16_FROM_FLOAT(gdk_back_color.blue));

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_color_background (VTE_TERMINAL(tt->vte_term),
                                           &gdk_back_color);
        vte_terminal_set_color_cursor_foreground (VTE_TERMINAL(tt->vte_term), 
                                                  &gdk_back_color);
    }
}


/**
 * This function is called if a different color scheme is selected from the combo box.
 */
static void combo_palette_scheme_changed_cb (GtkWidget *w, tilda_window *tw) {
    DEBUG_FUNCTION("combo_palette_scheme_changed_cb");
    gint i;
    guint j;
    tilda_term *tt;
    GdkRGBA fg, bg;
    GtkWidget *color_button;

    i = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    /* i = 0 means custom, in that case we do nothing */
    TildaColorScheme *tildaPaletteSchemes = tilda_palettes_get_palette_schemes ();

    if (i > 0 && i < tilda_palettes_get_n_palette_schemes ()) {

        const GdkRGBA *current_palette = tildaPaletteSchemes[i].palette;

        color_button =
            GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_text"));
        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(color_button), &fg);
        color_button =
            GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_back"));
        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(color_button), &bg);

        bg.alpha = (config_getbool("enable_transparency")
                    ? GUINT16_TO_FLOAT(config_getint ("back_alpha")) : 1.0);

        tilda_palettes_set_current_palette (current_palette);

        /* Set terminal palette. */
        for (j=0; j<g_list_length (tw->terms); j++) {
            tt = g_list_nth_data (tw->terms, j);
            vte_terminal_set_colors (VTE_TERMINAL(tt->vte_term),
                                     &fg,
                                     &bg,
                                     current_palette,
                                     TILDA_COLOR_PALETTE_SIZE);
        }

        for (j=0; j<TILDA_COLOR_PALETTE_SIZE; j++) {
            update_palette_color_button(j);

            /* Set palette in the config. */
            config_setnint ("palette", GUINT16_FROM_FLOAT(current_palette[j].red),   j*3);
            config_setnint ("palette", GUINT16_FROM_FLOAT(current_palette[j].green), j*3+1);
            config_setnint ("palette", GUINT16_FROM_FLOAT(current_palette[j].blue),  j*3+2);
        }
    }

    /* Set palette scheme in the config*/
    config_setint ("palette_scheme", i);
}

/**
 *  This function is called, if the user has changed a single color. The function
 *  handles this color change, and sets the color schema to "Custom".
 */
static void colorbutton_palette_n_set_cb (GtkWidget *w, tilda_window *tw)
{
    DEBUG_FUNCTION("colorbutton_palette_n_set_cb");
    const GtkWidget *combo_palette_scheme =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_palette_scheme"));
    const gchar* name = gtk_buildable_get_name(GTK_BUILDABLE(w));
    guint i;
    tilda_term *tt;
    GtkWidget *color_button;
    GdkRGBA fg, bg;

    /* The user just changed the palette manually, so we set the palette scheme to "Custom".
     * "Custom" is the 0th element in the palette_schemes.
     */
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_palette_scheme), 0);
    config_setint ("palette_scheme", 0);

    /* We need the index part of the name string, which looks like this:
     * "colorbutton_palette_12", so we set the button_index_str to the
     * position of the first digit and then convert it into an integer.
     */
    const char* button_index_str = &name[sizeof("colorbutton_palette_")-1];
    i = atoi(button_index_str);

    /* Now get the color that was set, save it. */
    GdkRGBA *current_palette = tilda_palettes_get_current_palette ();

    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(w), &current_palette[i]);

    /* Why saving the whole palette, not the single color that was set,
     * is if the config file doesn't exist, and we save the single color,
     * then exit, the color always becomes the first color in the config file,
     * no matter which one it actually is, and leaves other colors unset.
     * Obviously this is not what we want.
     * However, maybe there is a better solution for this issue.
     */
    for (i=0; i<TILDA_COLOR_PALETTE_SIZE; i++)
    {
        config_setnint ("palette", GUINT16_FROM_FLOAT(current_palette[i].red),   i*3);
        config_setnint ("palette", GUINT16_FROM_FLOAT(current_palette[i].green), i*3+1);
        config_setnint ("palette", GUINT16_FROM_FLOAT(current_palette[i].blue),  i*3+2);
    }

    /* Set terminal palette. */
    color_button =
        GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_text"));
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(color_button), &fg);
    color_button =
        GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_back"));
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(color_button), &bg);

    for (i=0; i<g_list_length (tw->terms); i++)
    {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_colors (VTE_TERMINAL (tt->vte_term),
                                 &fg,
                                 &bg,
                                 current_palette,
                                 TILDA_COLOR_PALETTE_SIZE);
    }
}

static void combo_scrollbar_position_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    guint i;
    tilda_term *tt;

    config_setint ("scrollbar_pos", status);

    for (i=0; i<g_list_length (tw->terms); i++)
    {
        tt = g_list_nth_data (tw->terms, i);
        tilda_term_set_scrollbar_position (tt, status);
    }
}

static void spin_scrollback_amount_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));

    guint i;
    tilda_term *tt;

    config_setint ("lines", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scrollback_lines (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void check_infinite_scrollback_toggled_cb(GtkWidget *w, tilda_window *tw)
{
    // if status is false then scrollback is infinite, otherwise the spinner is active
    const gboolean hasScrollbackLimit = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));

    config_setbool ("scroll_history_infinite", !hasScrollbackLimit);

    GtkWidget *spinner = (GtkWidget *) gtk_builder_get_object(xml, "spin_scrollback_amount");
    gint scrollback_lines = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinner));
    GtkWidget *label = (GtkWidget *) gtk_builder_get_object(xml, "label_scrollback_lines");

    gtk_widget_set_sensitive(spinner, hasScrollbackLimit);
    gtk_widget_set_sensitive(label, hasScrollbackLimit);

    guint i;
    tilda_term *tt;

    if(!hasScrollbackLimit) {
        scrollback_lines = -1;
    }

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scrollback_lines(VTE_TERMINAL(tt->vte_term), scrollback_lines);
    }
}

static void check_scroll_on_output_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    guint i;
    tilda_term *tt;

    config_setbool ("scroll_on_output", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scroll_on_output (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void check_scroll_on_keystroke_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    guint i;
    tilda_term *tt;

    config_setbool ("scroll_on_key", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void combo_backspace_binding_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    const gint keys[] = { VTE_ERASE_ASCII_DELETE,
                          VTE_ERASE_DELETE_SEQUENCE,
                          VTE_ERASE_ASCII_BACKSPACE,
                          VTE_ERASE_AUTO };
    guint i;
    tilda_term *tt;

    config_setint ("backspace_key", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_backspace_binding (VTE_TERMINAL(tt->vte_term), keys[status]);
    }
}

static void combo_delete_binding_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    const gint keys[] = { VTE_ERASE_ASCII_DELETE,
                          VTE_ERASE_DELETE_SEQUENCE,
                          VTE_ERASE_ASCII_BACKSPACE,
                          VTE_ERASE_AUTO };
    guint i;
    tilda_term *tt;

    config_setint ("delete_key", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_delete_binding (VTE_TERMINAL(tt->vte_term), keys[status]);
    }
}

static void button_reset_compatibility_options_clicked_cb (tilda_window *tw)
{
    const GtkWidget *combo_backspace_binding =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_backspace_binding"));
    const GtkWidget *combo_delete_binding =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_delete_binding"));

    config_setint ("backspace_key", 0);
    config_setint ("delete_key", 1);

    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_backspace_binding), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_delete_binding), 1);
}

static void
initialize_combo_choose_monitor(tilda_window *tw)
{
    DEBUG_FUNCTION ("initialize_combo_choose_monitor");

    /**
     * First we need to initialize the "combo_choose_monitor" widget,
     * with the numbers of each monitor attached to the system.
     */
    GdkDisplay *display = gdk_display_get_default ();
    int num_monitors = gdk_display_get_n_monitors (display);
    GdkMonitor *active_monitor = tilda_window_find_monitor_number(tw);

    GtkComboBox* combo_choose_monitor =
            GTK_COMBO_BOX(gtk_builder_get_object(xml,"combo_choose_monitor"));
    GtkListStore* monitor_model =
            GTK_LIST_STORE(gtk_combo_box_get_model(combo_choose_monitor));
    GtkTreeIter iter;
    gint i;

    gtk_list_store_clear(monitor_model);

    for (i = 0; i < num_monitors; i++) {
        GdkMonitor *monitor;

        gtk_list_store_append(monitor_model, &iter);
        monitor = gdk_display_get_monitor (display, i);

        const gchar *monitor_model_name = gdk_monitor_get_model (monitor);
        gchar *display_name = g_strdup_printf ("%d    <i>(%s)</i>", i, monitor_model_name);

        gtk_list_store_set(monitor_model, &iter,
                           0, monitor_model_name,
                           1, i,
                           2, display_name,
                           -1);

        if(monitor == active_monitor) {
          gtk_combo_box_set_active_iter(combo_choose_monitor, &iter);
        }

        g_free (display_name);
    }
}

/**
 * Here the geometry options in the appearance tab get initialized. The options are:
 * height and width (both absolute and in percentage), x and y positions as well as the
 * check boxes for centering.
 * Because we might have multiple monitors we first need to get the monitor that
 * is currently selected to show the window and then load its geometry information.
 */
static void initialize_geometry_spinners(tilda_window *tw) {
    DEBUG_FUNCTION ("initialize_geometry_spinners");

    GdkMonitor *monitor = tilda_window_find_monitor_number(tw);
    GdkRectangle rectangle;

    gdk_monitor_get_workarea (monitor, &rectangle);
    int monitor_height = rectangle.height;
    int monitor_width = rectangle.width;

    /* Update range and value of height spinners */
    GdkRectangle tilda_rectangle;
    config_get_configured_window_size (&tilda_rectangle);

    gint width = tilda_rectangle.width;
    gint height = tilda_rectangle.height;

    gdouble height_percentage =
            GLONG_TO_DOUBLE (config_getint("height_percentage")) * 100;

    gdouble width_percentage =
            GLONG_TO_DOUBLE (config_getint("width_percentage")) * 100;

    SPIN_BUTTON_SET_RANGE("spin_height_percentage", 0, 100);
    SPIN_BUTTON_SET_VALUE ("spin_height_percentage", height_percentage);
    SPIN_BUTTON_SET_RANGE("spin_height_pixels", 0, monitor_height);
    SPIN_BUTTON_SET_VALUE("spin_height_pixels", height);

    /* Update range and value of width spinners */
    SPIN_BUTTON_SET_RANGE("spin_width_percentage", 0, 100);
    SPIN_BUTTON_SET_VALUE ("spin_width_percentage", width_percentage);
    SPIN_BUTTON_SET_RANGE("spin_width_pixels", 0, monitor_width);
    SPIN_BUTTON_SET_VALUE("spin_width_pixels", width);

    CHECK_BUTTON("check_centered_horizontally", "centered_horizontally");
    CHECK_BUTTON("check_centered_vertically", "centered_vertically");
    CHECK_BUTTON("check_start_fullscreen", "start_fullscreen");

    gint xpos = config_getint("x_pos");
    if(xpos < rectangle.x) {
        xpos = rectangle.x;
        config_setint("x_pos", xpos);
    }

    gint screen_width, screen_height;
    screen_size_get_dimensions (&screen_width, &screen_height);
    SPIN_BUTTON_SET_RANGE("spin_x_position", 0, screen_width);
    SPIN_BUTTON_SET_VALUE("spin_x_position", xpos); /* TODO: Consider x in rectange.x for monitor displacement */

    gint ypos = config_getint("y_pos");
    if(ypos < rectangle.y) {
        ypos = rectangle.y;
        config_setint("y_pos", ypos);
    }
    SPIN_BUTTON_SET_RANGE("spin_y_position", 0, screen_height);
    SPIN_BUTTON_SET_VALUE("spin_y_position", ypos);

    SET_SENSITIVE_BY_CONFIG_NBOOL("spin_x_position", "centered_horizontally");
    SET_SENSITIVE_BY_CONFIG_NBOOL("label_x_position", "centered_horizontally");
    SET_SENSITIVE_BY_CONFIG_NBOOL("spin_y_position", "centered_vertically");
    SET_SENSITIVE_BY_CONFIG_NBOOL("label_y_position", "centered_vertically");
}

/* Read all state from the config system, and put it into
 * its visual representation in the wizard. */
static void set_wizard_state_from_config (tilda_window *tw) {
    GdkRGBA text_color, back_color, cursor_color;
    guint i;
    GdkRGBA *current_palette;

    /* General Tab */
    CHECK_BUTTON ("check_display_on_all_workspaces", "pinned");
    initialize_set_as_desktop_checkbox ();
    CHECK_BUTTON ("check_always_on_top", "above");
    CHECK_BUTTON ("check_do_not_show_in_taskbar", "notaskbar");
    CHECK_BUTTON ("check_start_tilda_hidden", "hidden");
    CHECK_BUTTON ("check_show_notebook_border", "notebook_border");
    COMBO_BOX ("combo_non_focus_pull_up_behaviour", "non_focus_pull_up_behaviour");

    CHECK_BUTTON ("check_terminal_bell", "bell");
    CHECK_BUTTON ("check_cursor_blinks", "blinks");
    COMBO_BOX ("vte_cursor_shape", "cursor_shape");

    FONT_BUTTON ("button_font", "font");

    SPIN_BUTTON_SET_RANGE ("spin_auto_hide_time", 0, 99999);
    SPIN_BUTTON_SET_VALUE ("spin_auto_hide_time", config_getint ("auto_hide_time"));
    CHECK_BUTTON ("check_auto_hide_on_focus_lost", "auto_hide_on_focus_lost");
    CHECK_BUTTON ("check_auto_hide_on_mouse_leave", "auto_hide_on_mouse_leave");

    /* Title and Command Tab */
    TEXT_ENTRY ("entry_title", "title");
    COMBO_BOX ("combo_dynamically_set_title", "d_set_title");
    // Whether to limit the length of the title
    COMBO_BOX ("combo_title_behaviour", "title_behaviour");
    // The maximum length of the title
    SPIN_BUTTON_SET_RANGE ("spin_title_max_length", 0, 99999);
    SPIN_BUTTON_SET_VALUE ("spin_title_max_length", config_getint ("title_max_length"));

    CHECK_BUTTON ("check_run_custom_command", "run_command");
    TEXT_ENTRY ("entry_custom_command", "command");
    CHECK_BUTTON ("check_command_login_shell", "command_login_shell");

    CHECK_BUTTON ("check_control_activates_match", "control_activates_match");

    CHECK_BUTTON ("check_match_web_uris", "match_web_uris");
    CHECK_BUTTON ("check_custom_web_browser", "use_custom_web_browser");

    CHECK_BUTTON ("check_match_file_uris", "match_file_uris");
    CHECK_BUTTON ("check_match_email_addresses", "match_email_addresses");
    CHECK_BUTTON ("check_match_numbers", "match_numbers");

    COMBO_BOX ("combo_command_exit", "command_exit");
    COMBO_BOX ("combo_on_last_terminal_exit", "on_last_terminal_exit");
    CHECK_BUTTON ("check_prompt_on_exit", "prompt_on_exit");
    SET_SENSITIVE_BY_CONFIG_BOOL ("entry_custom_command","run_command");
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_custom_command", "run_command");

    TEXT_ENTRY ("entry_web_browser", "web_browser");

    update_custom_web_browser_sensitivity ();

    CHECK_BUTTON ("check_confirm_close_tab", "confirm_close_tab");

    /* Appearance Tab */
    /* Initialize the monitor chooser combo box with the numbers of the monitor */
    initialize_combo_choose_monitor(tw);


    initialize_geometry_spinners(tw);
    CHECK_BUTTON ("check_enable_transparency", "enable_transparency");
    CHECK_BUTTON ("check_animated_pulldown", "animation");
    SPIN_BUTTON ("spin_animation_delay", "slide_sleep_usec");
    COMBO_BOX ("combo_animation_orientation", "animation_orientation");

    COMBO_BOX ("combo_tab_pos", "tab_pos");
    CHECK_BUTTON ("check_expand_tabs", "expand_tabs");
    CHECK_BUTTON ("check_show_single_tab", "show_single_tab");
    CHECK_BUTTON ("check_show_title_tooltip", "show_title_tooltip");

    SET_SENSITIVE_BY_CONFIG_BOOL ("label_level_of_transparency","enable_transparency");
    SET_SENSITIVE_BY_CONFIG_BOOL ("spin_level_of_transparency","enable_transparency");
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_animation_delay","animation");
    SET_SENSITIVE_BY_CONFIG_BOOL ("spin_animation_delay","animation");
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_animation_orientation","animation");
    SET_SENSITIVE_BY_CONFIG_BOOL ("combo_animation_orientation","animation");

    /* Colors Tab */
    COMBO_BOX ("combo_colorschemes", "scheme");
    text_color.red =   GUINT16_TO_FLOAT(config_getint ("text_red"));
    text_color.green = GUINT16_TO_FLOAT(config_getint ("text_green"));
    text_color.blue =  GUINT16_TO_FLOAT(config_getint ("text_blue"));
    text_color.alpha = 1.0;
    COLOR_CHOOSER ("colorbutton_text", &text_color);
    back_color.red =   GUINT16_TO_FLOAT(config_getint ("back_red"));
    back_color.green = GUINT16_TO_FLOAT(config_getint ("back_green"));
    back_color.blue =  GUINT16_TO_FLOAT(config_getint ("back_blue"));
    back_color.alpha = 1.0;
    COLOR_CHOOSER ("colorbutton_back", &back_color);
    cursor_color.red = GUINT16_TO_FLOAT(config_getint ("cursor_red"));
    cursor_color.green = GUINT16_TO_FLOAT(config_getint ("cursor_green"));
    cursor_color.blue = GUINT16_TO_FLOAT(config_getint ("cursor_blue"));
    cursor_color.alpha = 1.0;
    COLOR_CHOOSER ("colorbutton_cursor", &cursor_color);

    COMBO_BOX ("combo_palette_scheme", "palette_scheme");

    current_palette = tilda_palettes_get_current_palette ();

    for(i = 0;i < TILDA_COLOR_PALETTE_SIZE; i++) {
        current_palette[i].red   = GUINT16_TO_FLOAT (config_getnint ("palette", i*3));
        current_palette[i].green = GUINT16_TO_FLOAT (config_getnint ("palette", i*3+1));
        current_palette[i].blue  = GUINT16_TO_FLOAT (config_getnint ("palette", i*3+2));
        current_palette[i].alpha = 1.0;

        update_palette_color_button(i);
    }

    /* Scrolling Tab */
    initialize_scrollback_settings();

    /* Compatibility Tab */
    COMBO_BOX ("combo_backspace_binding", "backspace_key");
    COMBO_BOX ("combo_delete_binding", "delete_key");

    TEXT_ENTRY ("entry_word_chars", "word_chars");

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(xml, ("spin_level_of_transparency"))),
                              (100 - 100*GUINT16_TO_FLOAT(config_getint("back_alpha"))));
}

static void initialize_scrollback_settings(void) {
    COMBO_BOX ("combo_scrollbar_position", "scrollbar_pos");
    SPIN_BUTTON ("spin_scrollback_amount", "lines");

    /* For historical reasons the config value is named "scrollback_history_infinite", but we have changed the
     * UI semantics such that the checkbox is activated to limit the scrollback and deactivated to use an infinite
     * buffer. Therefore we need to negate the value from the config here. */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (xml, "check_infinite_scrollback")),
                                  !config_getbool ("scroll_history_infinite"));
    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (xml, ("label_scrollback_lines"))),
                              !config_getbool ("scroll_history_infinite"));
    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (xml, ("spin_scrollback_amount"))),
                              !config_getbool ("scroll_history_infinite"));

    CHECK_BUTTON ("check_scroll_on_output", "scroll_on_output");
    CHECK_BUTTON ("check_scroll_on_keystroke", "scroll_on_key");
}

static void initialize_set_as_desktop_checkbox (void) {
    CHECK_BUTTON ("check_set_as_desktop", "set_as_desktop");

    GtkWidget *check_set_as_desktop =            GTK_WIDGET(gtk_builder_get_object (xml, "check_set_as_desktop"));
    GtkWidget *check_display_on_all_workspaces = GTK_WIDGET(gtk_builder_get_object (xml, "check_display_on_all_workspaces"));

    gboolean status = config_getbool("set_as_desktop");
    gboolean status_display_on_all_workspaces = config_getbool ("pinned");
    if (status) {
        gtk_widget_set_sensitive (check_display_on_all_workspaces, FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_display_on_all_workspaces), TRUE);
    } else {
        gtk_widget_set_sensitive (check_display_on_all_workspaces, TRUE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_display_on_all_workspaces), status_display_on_all_workspaces);
    }
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_set_as_desktop), status);
}

#define CONNECT_SIGNAL(GLADE_WIDGET,SIGNAL_NAME,SIGNAL_HANDLER,DATA) g_signal_connect ( \
    gtk_builder_get_object (xml, (GLADE_WIDGET)), (SIGNAL_NAME), G_CALLBACK((SIGNAL_HANDLER)), DATA)

/* Connect all signals in the wizard. This should be done after setting all
 * values, that way all of the signal handlers don't get called */
static void connect_wizard_signals (TildaWizard *wizard)
{
    gint i;
    tilda_window *tw = wizard->tw;

    /* General Tab */
    CONNECT_SIGNAL ("check_display_on_all_workspaces","toggled",check_display_on_all_workspaces_toggled_cb, tw);
    CONNECT_SIGNAL ("check_set_as_desktop","toggled",check_set_as_desktop_toggled_cb, tw);
    CONNECT_SIGNAL ("check_do_not_show_in_taskbar","toggled",check_do_not_show_in_taskbar_toggled_cb, tw);
    CONNECT_SIGNAL ("check_show_notebook_border","toggled",check_show_notebook_border_toggled_cb, tw);
    CONNECT_SIGNAL ("check_always_on_top","toggled",check_always_on_top_toggled_cb, tw);
    CONNECT_SIGNAL ("check_start_tilda_hidden","toggled",check_start_tilda_hidden_toggled_cb, tw);
    CONNECT_SIGNAL ("combo_non_focus_pull_up_behaviour","changed",combo_non_focus_pull_up_behaviour_cb, tw);

    CONNECT_SIGNAL ("check_terminal_bell","toggled",check_terminal_bell_toggled_cb, tw);
    CONNECT_SIGNAL ("check_cursor_blinks","toggled",check_cursor_blinks_toggled_cb, tw);
    CONNECT_SIGNAL ("vte_cursor_shape","changed", combo_cursor_shape_changed_cb, tw);

    CONNECT_SIGNAL ("check_start_fullscreen", "toggled", check_start_fullscreen_cb, tw);

    CONNECT_SIGNAL ("button_font","font-set",button_font_font_set_cb, tw);

    CONNECT_SIGNAL ("spin_auto_hide_time","value-changed",spin_auto_hide_time_value_changed_cb, tw);
    CONNECT_SIGNAL ("check_auto_hide_on_focus_lost","toggled",check_auto_hide_on_focus_lost_toggled_cb, tw);
    CONNECT_SIGNAL ("check_auto_hide_on_mouse_leave","toggled",check_auto_hide_on_mouse_leave_toggled_cb, tw);

    CONNECT_SIGNAL ("combo_on_last_terminal_exit","changed",combo_on_last_terminal_exit_changed_cb, tw);
    CONNECT_SIGNAL ("check_prompt_on_exit","toggled",check_prompt_on_exit_toggled_cb, tw);

    /* Title and Command Tab */
    CONNECT_SIGNAL ("entry_title","changed",entry_title_changed_cb, tw);
    CONNECT_SIGNAL ("combo_dynamically_set_title","changed",combo_dynamically_set_title_changed_cb, tw);
    CONNECT_SIGNAL ("combo_title_behaviour","changed",combo_title_behaviour_changed_cb, tw);
    CONNECT_SIGNAL ("spin_title_max_length","value-changed", spin_max_title_length_changed_cb, tw);

    CONNECT_SIGNAL ("check_run_custom_command","toggled",check_run_custom_command_toggled_cb, tw);
    CONNECT_SIGNAL ("entry_custom_command","focus-out-event", validate_executable_command_cb, tw);
    CONNECT_SIGNAL ("combo_command_exit","changed",combo_command_exit_changed_cb, tw);
    CONNECT_SIGNAL ("check_command_login_shell", "toggled", check_command_login_shell_cb, tw);

    CONNECT_SIGNAL ("check_control_activates_match", "toggled", check_control_activates_match_cb, tw);

    CONNECT_SIGNAL ("check_match_web_uris", "toggled", check_match_web_uris_cb, tw);
    CONNECT_SIGNAL ("check_custom_web_browser", "toggled", check_custom_web_browser_cb, tw);

    CONNECT_SIGNAL ("check_match_file_uris", "toggled", check_match_file_uris_cb, tw);
    CONNECT_SIGNAL ("check_match_email_addresses", "toggled", check_match_email_addresses_cb, tw);
    CONNECT_SIGNAL ("check_match_numbers", "toggled", check_match_numbers_cb, tw);

    CONNECT_SIGNAL ("entry_web_browser","changed",entry_web_browser_changed, tw);
    CONNECT_SIGNAL ("entry_web_browser","focus-out-event", validate_executable_command_cb, tw);

    CONNECT_SIGNAL ("check_confirm_close_tab", "toggled", check_confirm_close_tab_cb, tw);

    /* Appearance Tab */
    CONNECT_SIGNAL ("combo_choose_monitor", "changed", combo_monitor_selection_changed_cb, tw);
    CONNECT_SIGNAL ("spin_height_percentage","value-changed",spin_height_percentage_value_changed_cb, tw);
    CONNECT_SIGNAL ("spin_height_pixels","value-changed",spin_height_pixels_value_changed_cb, tw);
    CONNECT_SIGNAL ("spin_width_percentage","value-changed",spin_width_percentage_value_changed_cb, tw);
    CONNECT_SIGNAL ("spin_width_pixels","value-changed",spin_width_pixels_value_changed_cb, tw);

    CONNECT_SIGNAL ("check_centered_horizontally","toggled",check_centered_horizontally_toggled_cb, tw);
    CONNECT_SIGNAL ("check_centered_vertically","toggled",check_centered_vertically_toggled_cb, tw);
    CONNECT_SIGNAL ("spin_x_position","value-changed",spin_x_position_value_changed_cb, tw);
    CONNECT_SIGNAL ("spin_y_position","value-changed",spin_y_position_value_changed_cb, tw);

    CONNECT_SIGNAL ("combo_tab_pos","changed",combo_tab_pos_changed_cb, tw);
    CONNECT_SIGNAL ("check_expand_tabs","toggled",check_expand_tabs_toggled_cb, tw);
    CONNECT_SIGNAL ("check_show_single_tab","toggled",check_show_single_tab_toggled_cb, tw);
    CONNECT_SIGNAL ("check_show_title_tooltip","toggled",check_show_title_tooltip_toggled_cb, tw);

    CONNECT_SIGNAL ("check_enable_transparency","toggled",check_enable_transparency_toggled_cb, tw);
    CONNECT_SIGNAL ("check_animated_pulldown","toggled",check_animated_pulldown_toggled_cb, tw);
    CONNECT_SIGNAL ("spin_level_of_transparency","value-changed",spin_level_of_transparency_value_changed_cb, tw);
    CONNECT_SIGNAL ("spin_animation_delay","value-changed",spin_animation_delay_value_changed_cb, tw);
    CONNECT_SIGNAL ("combo_animation_orientation","changed",combo_animation_orientation_changed_cb, tw);

    /* Colors Tab */
    CONNECT_SIGNAL ("combo_colorschemes","changed",combo_colorschemes_changed_cb, tw);
    CONNECT_SIGNAL ("colorbutton_text","color-set",colorbutton_text_color_set_cb, tw);
    CONNECT_SIGNAL ("colorbutton_back","color-set",colorbutton_back_color_set_cb, tw);
    CONNECT_SIGNAL ("colorbutton_cursor","color-set",colorbutton_cursor_color_set_cb, tw);
    CONNECT_SIGNAL ("combo_palette_scheme","changed",combo_palette_scheme_changed_cb, tw);
    for(i = 0; i < TILDA_COLOR_PALETTE_SIZE; i++)
    {
        char *s = g_strdup_printf ("colorbutton_palette_%d", i);
        CONNECT_SIGNAL (s,"color-set",colorbutton_palette_n_set_cb, tw);
        g_free (s);
    }

    /* Scrolling Tab */
    CONNECT_SIGNAL ("combo_scrollbar_position","changed",combo_scrollbar_position_changed_cb, tw);
    CONNECT_SIGNAL ("spin_scrollback_amount","value-changed",spin_scrollback_amount_value_changed_cb, tw);
    CONNECT_SIGNAL ("check_infinite_scrollback", "toggled", check_infinite_scrollback_toggled_cb, tw);
    CONNECT_SIGNAL ("check_scroll_on_output","toggled",check_scroll_on_output_toggled_cb, tw);
    CONNECT_SIGNAL ("check_scroll_on_keystroke","toggled",check_scroll_on_keystroke_toggled_cb, tw);

    /* Compatibility Tab */
    CONNECT_SIGNAL ("combo_backspace_binding","changed",combo_backspace_binding_changed_cb, tw);
    CONNECT_SIGNAL ("combo_delete_binding","changed",combo_delete_binding_changed_cb, tw);
    CONNECT_SIGNAL ("button_reset_compatibility_options","clicked",button_reset_compatibility_options_clicked_cb, tw);

    /* Close Button */
    CONNECT_SIGNAL ("button_wizard_close","clicked", wizard_button_close_clicked_cb, wizard);
    CONNECT_SIGNAL ("wizard_window","delete_event", wizard_window_delete_event_cb, wizard);

    CONNECT_SIGNAL ("entry_word_chars", "changed", entry_word_chars_changed, tw);
}

/* Initialize the palette scheme menu.
 * Add the predefined schemes to the combo box.*/
static void init_palette_scheme_menu (void)
{
    gint i;
    TildaColorScheme *paletteSchemes;
    GtkWidget *combo_palette;

    combo_palette = GTK_WIDGET (gtk_builder_get_object (xml,
                                                        "combo_palette_scheme"));

    i = tilda_palettes_get_n_palette_schemes ();
    paletteSchemes = tilda_palettes_get_palette_schemes ();

    while (i > 0) {
        --i;
        const char *palette_name = paletteSchemes[i].name;
        gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (combo_palette),
                                         _(palette_name));
    }
}

static void update_palette_color_button(gint idx)
{
    GdkRGBA * current_palette;
    char *s = g_strdup_printf ("colorbutton_palette_%d", idx);
    GtkWidget *color_button =
        GTK_WIDGET (gtk_builder_get_object (xml, s));

    g_free (s);

    current_palette = tilda_palettes_get_current_palette ();

    const GdkRGBA *color = tilda_palettes_get_palette_color (current_palette, idx);
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (color_button),
                                color);
}
