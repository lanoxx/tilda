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

#include <tilda-config.h>

#include <errno.h>

#include "debug.h"
#include "tilda.h"
#include "wizard.h"
#include "key_grabber.h"
#include "configsys.h"
#include "callback_func.h"
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

const GdkRGBA
terminal_palette_tango[TERMINAL_PALETTE_SIZE] = {
    { RGB(0x2e2e, 0x3434, 0x3636) },
    { RGB(0xcccc, 0x0000, 0x0000) },
    { RGB(0x4e4e, 0x9a9a, 0x0606) },
    { RGB(0xc4c4, 0xa0a0, 0x0000) },
    { RGB(0x3434, 0x6565, 0xa4a4) },
    { RGB(0x7575, 0x5050, 0x7b7b) },
    { RGB(0x0606, 0x9820, 0x9a9a) },
    { RGB(0xd3d3, 0xd7d7, 0xcfcf) },
    { RGB(0x5555, 0x5757, 0x5353) },
    { RGB(0xefef, 0x2929, 0x2929) },
    { RGB(0x8a8a, 0xe2e2, 0x3434) },
    { RGB(0xfcfc, 0xe9e9, 0x4f4f) },
    { RGB(0x7272, 0x9f9f, 0xcfcf) },
    { RGB(0xadad, 0x7f7f, 0xa8a8) },
    { RGB(0x3434, 0xe2e2, 0xe2e2) },
    { RGB(0xeeee, 0xeeee, 0xecec) }
};

const GdkRGBA
terminal_palette_zenburn[TERMINAL_PALETTE_SIZE] = {
    { RGB(0x2222, 0x2222, 0x2222) }, //black
    { RGB(0x8080, 0x3232, 0x3232) }, //darkred
    { RGB(0x5b5b, 0x7676, 0x2f2f) }, //darkgreen
    { RGB(0xaaaa, 0x9999, 0x4343) }, //brown
    { RGB(0x3232, 0x4c4c, 0x8080) }, //darkblue
    { RGB(0x7070, 0x6c6c, 0x9a9a) }, //darkmagenta
    { RGB(0x9292, 0xb1b1, 0x9e9e) }, //darkcyan
    { RGB(0xffff, 0xffff, 0xffff) }, //lightgrey
    { RGB(0x2222, 0x2222, 0x2222) }, //darkgrey
    { RGB(0x9898, 0x2b2b, 0x2b2b) }, //red
    { RGB(0x8989, 0xb8b8, 0x3f3f) }, //green
    { RGB(0xefef, 0xefef, 0x6060) }, //yellow
    { RGB(0x2b2b, 0x4f4f, 0x9898) }, //blue
    { RGB(0x8282, 0x6a6a, 0xb1b1) }, //magenta
    { RGB(0xa1a1, 0xcdcd, 0xcdcd) }, //cyan
    { RGB(0xdede, 0xdede, 0xdede) }, //white}
};

const GdkRGBA
terminal_palette_linux[TERMINAL_PALETTE_SIZE] = {
    { RGB(0x0000, 0x0000, 0x0000) },
    { RGB(0xaaaa, 0x0000, 0x0000) },
    { RGB(0x0000, 0xaaaa, 0x0000) },
    { RGB(0xaaaa, 0x5555, 0x0000) },
    { RGB(0x0000, 0x0000, 0xaaaa) },
    { RGB(0xaaaa, 0x0000, 0xaaaa) },
    { RGB(0x0000, 0xaaaa, 0xaaaa) },
    { RGB(0xaaaa, 0xaaaa, 0xaaaa) },
    { RGB(0x5555, 0x5555, 0x5555) },
    { RGB(0xffff, 0x5555, 0x5555) },
    { RGB(0x5555, 0xffff, 0x5555) },
    { RGB(0xffff, 0xffff, 0x5555) },
    { RGB(0x5555, 0x5555, 0xffff) },
    { RGB(0xffff, 0x5555, 0xffff) },
    { RGB(0x5555, 0xffff, 0xffff) },
    { RGB(0xffff, 0xffff, 0xffff) }
};

const GdkRGBA
terminal_palette_xterm[TERMINAL_PALETTE_SIZE] = {
    {RGB(0x0000, 0x0000, 0x0000) },
    {RGB(0xcdcb, 0x0000, 0x0000) },
    {RGB(0x0000, 0xcdcb, 0x0000) },
    {RGB(0xcdcb, 0xcdcb, 0x0000) },
    {RGB(0x1e1a, 0x908f, 0xffff) },
    {RGB(0xcdcb, 0x0000, 0xcdcb) },
    {RGB(0x0000, 0xcdcb, 0xcdcb) },
    {RGB(0xe5e2, 0xe5e2, 0xe5e2) },
    {RGB(0x4ccc, 0x4ccc, 0x4ccc) },
    {RGB(0xffff, 0x0000, 0x0000) },
    {RGB(0x0000, 0xffff, 0x0000) },
    {RGB(0xffff, 0xffff, 0x0000) },
    {RGB(0x4645, 0x8281, 0xb4ae) },
    {RGB(0xffff, 0x0000, 0xffff) },
    {RGB(0x0000, 0xffff, 0xffff) },
    {RGB(0xffff, 0xffff, 0xffff) }
};

const GdkRGBA
terminal_palette_rxvt[TERMINAL_PALETTE_SIZE] = {
    { RGB(0x0000, 0x0000, 0x0000) },
    { RGB(0xcdcd, 0x0000, 0x0000) },
    { RGB(0x0000, 0xcdcd, 0x0000) },
    { RGB(0xcdcd, 0xcdcd, 0x0000) },
    { RGB(0x0000, 0x0000, 0xcdcd) },
    { RGB(0xcdcd, 0x0000, 0xcdcd) },
    { RGB(0x0000, 0xcdcd, 0xcdcd) },
    { RGB(0xfafa, 0xebeb, 0xd7d7) },
    { RGB(0x4040, 0x4040, 0x4040) },
    { RGB(0xffff, 0x0000, 0x0000) },
    { RGB(0x0000, 0xffff, 0x0000) },
    { RGB(0xffff, 0xffff, 0x0000) },
    { RGB(0x0000, 0x0000, 0xffff) },
    { RGB(0xffff, 0x0000, 0xffff) },
    { RGB(0x0000, 0xffff, 0xffff) },
    { RGB(0xffff, 0xffff, 0xffff) }
};

const GdkRGBA
terminal_palette_solarizedL[TERMINAL_PALETTE_SIZE] = {
	{ RGB(0xeeee, 0xe8e8, 0xd5d5) },
	{ RGB(0xdcdc, 0x3232, 0x2f2f) },
	{ RGB(0x8585, 0x9999, 0x0000) },
	{ RGB(0xb5b5, 0x8989, 0x0000) },
	{ RGB(0x2626, 0x8b8b, 0xd2d2) },
	{ RGB(0xd3d3, 0x3636, 0x8282) },
	{ RGB(0x2a2a, 0xa1a1, 0x9898) },
	{ RGB(0x0707, 0x3636, 0x4242) },
	{ RGB(0xfdfd, 0xf6f6, 0xe3e3) },
	{ RGB(0xcbcb, 0x4b4b, 0x1616) },
	{ RGB(0x9393, 0xa1a1, 0xa1a1) },
	{ RGB(0x8383, 0x9494, 0x9696) },
	{ RGB(0x6565, 0x7b7b, 0x8383) },
	{ RGB(0x6c6c, 0x7171, 0xc4c4) },
	{ RGB(0x5858, 0x6e6e, 0x7575) },
	{ RGB(0x0000, 0x2b2b, 0x3636) }
};

const GdkRGBA
terminal_palette_solarizedD[TERMINAL_PALETTE_SIZE] = {
	{ RGB(0x0707, 0x3636, 0x4242) },
	{ RGB(0xdcdc, 0x3232, 0x2f2f) },
	{ RGB(0x8585, 0x9999, 0x0000) },
	{ RGB(0xb5b5, 0x8989, 0x0000) },
	{ RGB(0x2626, 0x8b8b, 0xd2d2) },
	{ RGB(0xd3d3, 0x3636, 0x8282) },
	{ RGB(0x2a2a, 0xa1a1, 0x9898) },
	{ RGB(0xeeee, 0xe8e8, 0xd5d5) },
	{ RGB(0x0000, 0x2b2b, 0x3636) },
	{ RGB(0xcbcb, 0x4b4b, 0x1616) },
	{ RGB(0x5858, 0x6e6e, 0x7575) },
	{ RGB(0x8383, 0x9494, 0x9696) },
	{ RGB(0x6565, 0x7b7b, 0x8383) },
	{ RGB(0x6c6c, 0x7171, 0xc4c4) },
	{ RGB(0x9393, 0xa1a1, 0xa1a1) },
	{ RGB(0xfdfd, 0xf6f6, 0xe3e3) }
};

typedef struct _TerminalPaletteScheme
{
  const char *name;
  const GdkRGBA *palette;
}TerminalPaletteScheme;

static TerminalPaletteScheme palette_schemes[] = {
    { N_("Custom"), NULL },
    { N_("Tango"), terminal_palette_tango },
    { N_("Linux console"), terminal_palette_linux },
    { N_("XTerm"), terminal_palette_xterm },
    { N_("Rxvt"), terminal_palette_rxvt },
    { N_("Zenburn"), terminal_palette_zenburn },
    { N_("Solarized Light"), terminal_palette_solarizedL },
    { N_("Solarized Dark"), terminal_palette_solarizedD }
};

/* For use in get_display_dimension() */
enum dimensions { HEIGHT, WIDTH };

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
static int find_centering_coordinate (tilda_window *tw, enum dimensions dimension);
static void initialize_geometry_spinners(tilda_window *tw);

#ifndef VTE_290
static void wizard_hide_deprecated_options(void);
#endif

static gint find_monitor_number(tilda_window *tw)
{
    DEBUG_FUNCTION ("find_monitor_number");

    GdkScreen *screen = gtk_widget_get_screen (tw->window);
    gint n_monitors = gdk_screen_get_n_monitors (screen);

    gchar *show_on_monitor = config_getstr("show_on_monitor");
    for(int i = 0; i < n_monitors; ++i) {
        gchar *monitor_name = gdk_screen_get_monitor_plug_name (screen, i);
        if(0 == g_strcmp0 (show_on_monitor, monitor_name)) {
            return i;
        }
    }

    return gdk_screen_get_primary_monitor (screen);
}

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

#ifndef VTE_290
    wizard_hide_deprecated_options();
#endif

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
#define FONT_BUTTON(GLADE_NAME,CFG_STR) gtk_font_button_set_font_name (GTK_FONT_BUTTON( \
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

static int percentage_dimension (int max_size, int current_size) {
    DEBUG_FUNCTION ("percentage_dimension");

    return (int) (((float) current_size) / ((float) max_size) * 100.0);
}

#define percentage_height(max_size, current_height) percentage_dimension(max_size, current_height)
#define percentage_width(max_size, current_width)   percentage_dimension(max_size, current_width)

#define pixels2percentage(max_size,pixels) percentage_dimension ((max_size), (pixels))
#define percentage2pixels(max_size,percentage) (((percentage) / 100.0) * max_size)

/**
 * Get the number of screens and load the monitor geometry for each screen,
 * then set the position of the window according to the x and y offset
 * of that monitor. This function does not actually move or resize the window
 * but only changes the value of the spin buttons. The moving and resizing
 * is then done by the callback functions of the respective widgets.
 */
static int combo_monitor_selection_changed_cb(GtkWidget* widget, tilda_window *tw) {
	// Get the monitor number on which the window is currently shown
	int last_monitor = find_monitor_number(tw);
	GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(tw->window));
	int num_monitors = gdk_screen_get_n_monitors(screen);
	GdkRectangle* rect = malloc(sizeof(GdkRectangle) * num_monitors);
	int i;
	for(i=0; i<num_monitors; i++) {
		GdkRectangle* current_rectangle = rect+i;
		gdk_screen_get_monitor_workarea(screen, i, current_rectangle);
	}

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

	//Save the new monitor value
	config_setstr("show_on_monitor", new_monitor_name);
	GdkRectangle* current_rectangle = rect + new_monitor_number;
	GdkRectangle* last_rectangle = rect + last_monitor;
	/* The dimensions of the monitor might have changed,
	 * so we need to update the spinner widgets for height,
	 * width, and their percentages as well as their ranges.
	 * Keep in mind that we use the max range of the pixel spinners
	 * to store the size of the screen. This only works well if we hide
	 * the window before updating all the spinners.
	 */
	gtk_widget_hide(tw->window);
	if(current_rectangle->width != last_rectangle->width) {
		int width_percent = SPIN_BUTTON_GET_VALUE ("spin_width_percentage");
		int new_max_width = current_rectangle->width;
		int width = percentage2pixels(new_max_width, width_percent);
		SPIN_BUTTON_SET_RANGE ("spin_width_pixels", 0, new_max_width);
		SPIN_BUTTON_SET_VALUE ("spin_width_pixels", width);

		gtk_window_resize (GTK_WINDOW(tw->window), width, config_getint("max_height"));
	}
	if(current_rectangle->height != last_rectangle->height) {
		int height_percent = SPIN_BUTTON_GET_VALUE ("spin_height_percentage");
		int new_max_height = current_rectangle->height;
		int height = percentage2pixels(new_max_height, height_percent);
		SPIN_BUTTON_SET_RANGE ("spin_height_pixels", 0, new_max_height);
		SPIN_BUTTON_SET_VALUE ("spin_height_pixels", height);

		gtk_window_resize (GTK_WINDOW(tw->window), config_getint("max_width"), height);
	}

	SPIN_BUTTON_SET_RANGE ("spin_x_position", 0, gdk_screen_width());
	SPIN_BUTTON_SET_VALUE("spin_x_position", current_rectangle->x);
	SPIN_BUTTON_SET_RANGE ("spin_y_position", 0, gdk_screen_height());
	SPIN_BUTTON_SET_VALUE("spin_y_position", current_rectangle->y);

    gtk_widget_show(tw->window);
    free(rect);
	return TRUE; //callback was handled
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
        title = get_window_title (tt->vte_term);
        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook), i);
        label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (tw->notebook), page);

        guint length = config_getint ("title_max_length");

        if(config_getbool("title_max_length_flag") && strlen(title) > length) {
            gchar *titleOffset = title + strlen(title) - length;
            gchar *shortTitle = g_strdup_printf ("...%s", titleOffset);
            gtk_label_set_text (GTK_LABEL(label), shortTitle);
            g_free(shortTitle);
        } else {
            gtk_label_set_text (GTK_LABEL(label), title);
        }

        g_free (title);
    }
}

static void set_spin_value_while_blocking_callback (GtkSpinButton *spin,
                                                    void (*callback)(GtkWidget *w, tilda_window *tw),
                                                    gint new_val,
                                                    tilda_window *tw)
{
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

static void check_enable_double_buffering_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    guint i;
    tilda_term *tt;

    config_setbool ("double_buffer", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        gtk_widget_set_double_buffered (GTK_WIDGET(tt->vte_term), status);
    }
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
#ifdef VTE_290
        vte_terminal_set_visible_bell (VTE_TERMINAL(tt->vte_term), !status);
#endif
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

static void check_enable_antialiasing_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    guint i;
    tilda_term *tt;

    config_setbool ("antialias", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        PangoFontDescription *description =
            pango_font_description_from_string (config_getstr ("font"));
        vte_terminal_set_font (VTE_TERMINAL(tt->vte_term), description);
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

static void check_allow_bold_text_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    guint i;
    tilda_term *tt;

    config_setbool ("bold", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_allow_bold (VTE_TERMINAL(tt->vte_term), status);
    }
}

static void combo_cursor_shape_changed_cb(GtkWidget *w, tilda_window *tw)
{
    guint i;
    tilda_term *tt;
    gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    if (status < 0 || status > 2) {
        DEBUG_ERROR ("Invalid Cursor Type");
        g_printerr (_("Invalid Cursor Type, reseting to default\n"));
        status = 0;
    }
    config_setint("cursor_shape", (VteTerminalCursorShape)status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_cursor_shape (VTE_TERMINAL(tt->vte_term), (VteTerminalCursorShape)status);
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

static void button_font_font_set_cb (GtkWidget *w, tilda_window *tw)
{
    const gchar *font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (w));
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

static void check_max_title_length_cb (GtkWidget *w, tilda_window *tw)
{
    DEBUG_FUNCTION ("check_max_title_length_cb");

    GtkWidget *entry = GTK_WIDGET(
        gtk_builder_get_object (xml, ("spin_title_max_length"))
    );
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
        config_setbool("title_max_length_flag", TRUE);
        gtk_editable_set_editable (GTK_EDITABLE(entry), TRUE);
    } else {
        config_setbool("title_max_length_flag", FALSE);
        gtk_editable_set_editable (GTK_EDITABLE(entry), FALSE);
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

static void combo_command_exit_changed_cb (GtkWidget *w, tilda_window *tw) {
    const gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(w));

    config_setint ("command_exit", status);
}

static void check_command_login_shell_cb (GtkWidget *w, tilda_window *tw) {
    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    config_setbool("command_login_shell", active);
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

static void entry_web_browser_changed (GtkWidget *w, tilda_window *tw) {
    const gchar *web_browser = gtk_entry_get_text (GTK_ENTRY(w));

    config_setstr ("web_browser", web_browser);
}

#if (VTE_290 || VTE_MINOR_VERSION >= 40)
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
    #if VTE_MINOR_VERSION >= 40
            vte_terminal_set_word_char_exceptions (VTE_TERMINAL (tt->vte_term), word_chars);
    #else
            vte_terminal_set_word_chars (VTE_TERMINAL(tt->vte_term), word_chars);
    #endif
    }
}
#endif

#ifdef VTE_291
static void wizard_hide_deprecated_options(void) {
    /**
     * Word chars are only unavailable in VTE 38.x
     */
    #if VTE_MINOR_VERSION < 40
        GtkWidget *frame_word_chars =
          GTK_WIDGET(gtk_builder_get_object (xml, ("frame_word_chars")));
        gtk_widget_hide (frame_word_chars);
    #endif

    GtkWidget *check_use_image_for_background =
      GTK_WIDGET (gtk_builder_get_object (xml, ("check_use_image_for_background")));
    GtkWidget *button_background_image =
        GTK_WIDGET (gtk_builder_get_object (xml, ("button_background_image")));
    GtkWidget *check_scroll_background =
      GTK_WIDGET (gtk_builder_get_object (xml, ("check_scroll_background")));
    gtk_widget_hide (check_use_image_for_background);
    gtk_widget_hide (button_background_image);
    gtk_widget_hide (check_scroll_background);
}
#endif

/*
 * Finds the coordinate that will center the tilda window in the screen.
 *
 * If you want to center the tilda window on the top or bottom of the screen,
 * pass the screen width into screen_dimension and the tilda window's width
 * into the tilda_dimension variable. The result will be the x coordinate that
 * should be used in order to have the tilda window centered on the screen.
 *
 * Centering based on y coordinate is similar, just use the screen height and
 * tilda window height.
 */
static int find_centering_coordinate (tilda_window *tw, enum dimensions dimension)
{
    DEBUG_FUNCTION ("find_centering_coordinate");

    gdouble monitor_dimension = 0;
    gdouble tilda_dimension = 0;
    gint monitor = find_monitor_number(tw);
    GdkRectangle rectangle;
    gdk_screen_get_monitor_workarea (gtk_widget_get_screen(tw->window), monitor, &rectangle);
    if (dimension == HEIGHT) {
        monitor_dimension = rectangle.height;
        tilda_dimension = config_getint("max_height");
    } else if (dimension == WIDTH) {
        monitor_dimension = rectangle.width;
        tilda_dimension = config_getint("max_width");
    }
    const gdouble screen_center = monitor_dimension / 2.0;
    const gdouble tilda_center  = tilda_dimension  / 2.0;
    gint center = (int) (screen_center - tilda_center);

    if(dimension == HEIGHT) {
        center += rectangle.y;
    } else if (dimension == WIDTH) {
        center += rectangle.x;
    }
    return center;
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

static void spin_height_percentage_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const GtkWidget *spin_height_pixels =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_height_pixels"));

    const gint h_pct = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint h_pix = (int) percentage2pixels (get_max_height(), h_pct);

    config_setint ("max_height", h_pix);
    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_height_pixels), &spin_height_pixels_value_changed_cb, h_pix, tw);
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    if (config_getbool ("centered_vertically")) {
        config_setint ("y_pos", find_centering_coordinate (tw, HEIGHT));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}


static void spin_height_pixels_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const GtkWidget *spin_height_percentage =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_height_percentage"));
    const gint h_pix = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint h_pct = pixels2percentage (get_max_height(), h_pix);

    config_setint ("max_height", h_pix);
    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_height_percentage), &spin_height_percentage_value_changed_cb, h_pct, tw);
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    if (config_getbool ("centered_vertically")) {
        config_setint ("y_pos", find_centering_coordinate (tw, HEIGHT));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_width_percentage_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const GtkWidget *spin_width_pixels =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_width_pixels"));

    const gint w_pct = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint w_pix = (int) percentage2pixels (get_max_width(), w_pct);

    config_setint ("max_width", w_pix);
    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_width_pixels), &spin_width_pixels_value_changed_cb, w_pix, tw);
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    if (config_getbool ("centered_horizontally")) {
        config_setint ("x_pos", find_centering_coordinate (tw, WIDTH));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void spin_width_pixels_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const GtkWidget *spin_width_percentage =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_width_percentage"));

    const gint w_pix = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gint w_pct = pixels2percentage (get_max_width(), w_pix);

    config_setint ("max_width", w_pix);
    set_spin_value_while_blocking_callback (GTK_SPIN_BUTTON(spin_width_percentage),
        &spin_width_percentage_value_changed_cb, w_pct, tw);
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));

    if (config_getbool ("centered_horizontally")) {
        config_setint ("x_pos", find_centering_coordinate (tw, WIDTH));
        gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    }

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
}

static void check_centered_horizontally_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_x_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_x_position"));
    const GtkWidget *spin_x_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_x_position"));

    config_setbool ("centered_horizontally", active);

    if (active)
        config_setint ("x_pos", find_centering_coordinate (tw, WIDTH));
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
    const gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *label_y_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "label_y_position"));
    const GtkWidget *spin_y_position =
        GTK_WIDGET (gtk_builder_get_object (xml, "spin_y_position"));

    config_setbool ("centered_vertically", active);

    if (active)
        config_setint ("y_pos", find_centering_coordinate (tw, HEIGHT));
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
    const gint x_pos = config_getint ("x_pos");
    const gint y_pos = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));

    config_setint ("y_pos", y_pos);
    gtk_window_move (GTK_WINDOW(tw->window), x_pos, y_pos);

    /* Always regenerate animation positions when changing x or y position!
     * Otherwise you get VERY strange things going on :) */
    generate_animation_positions (tw);
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
#ifdef VTE_290
static void spin_level_of_transparency_value_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gint status = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(w));
    const gdouble transparency_level = (status / 100.0);
    guint i;
    tilda_term *tt;

    config_setint ("transparency", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_background_saturation (VTE_TERMINAL(tt->vte_term), transparency_level);
        vte_terminal_set_opacity (VTE_TERMINAL(tt->vte_term), (1.0 - transparency_level) * 0xffff);
        vte_terminal_set_background_transparent(VTE_TERMINAL(tt->vte_term), !tw->have_argb_visual);
    }
}
#else
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

    config_setint ("back_alpha", (100 - status) * 0x290 - 65);
    for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_color_background(VTE_TERMINAL(tt->vte_term), &bg);
        }
}
#endif
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
        gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
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

#ifdef VTE_290
static void check_use_image_for_background_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    const GtkWidget *button_background_image =
        GTK_WIDGET (gtk_builder_get_object (xml, "button_background_image"));

    const gchar *image = config_getstr ("image");
    guint i;
    tilda_term *tt;


    config_setbool ("use_image", status);

    gtk_widget_set_sensitive (GTK_WIDGET(button_background_image), status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);

        if (status)
            vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), image);
        else
            vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), NULL);
    }
}

static void button_background_image_selection_changed_cb (GtkWidget *w, tilda_window *tw)
{
    const gchar *image = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(w));
    guint i;
    tilda_term *tt;

    config_setstr ("image", image);

    if (config_getbool ("use_image"))
    {
        for (i=0; i<g_list_length (tw->terms); i++) {
            tt = g_list_nth_data (tw->terms, i);
            vte_terminal_set_background_image_file (VTE_TERMINAL(tt->vte_term), image);
        }
    }
}
#endif

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
            vte_terminal_set_color_foreground_rgba (VTE_TERMINAL(tt->vte_term), &gdk_text);
            vte_terminal_set_color_background_rgba (VTE_TERMINAL(tt->vte_term), &gdk_back);
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
        vte_terminal_set_color_cursor_rgba (VTE_TERMINAL(tt->vte_term), &gdk_cursor_color);
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
        vte_terminal_set_color_foreground_rgba (VTE_TERMINAL(tt->vte_term), &gdk_text_color);
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
        vte_terminal_set_color_background_rgba (VTE_TERMINAL(tt->vte_term), &gdk_back_color);
    }
}


/**
 * This function is called if a different color scheme is selected from the combo box.
 */
static void combo_palette_scheme_changed_cb (GtkWidget *w, tilda_window *tw) {
	DEBUG_FUNCTION("combo_palette_scheme_changed_cb");
    guint i, j;
    tilda_term *tt;
    GdkRGBA fg, bg;
    GtkWidget *color_button;

    i = gtk_combo_box_get_active (GTK_COMBO_BOX(w));
    /* i = 0 means custom, in that case we do nothing */
    if (i > 0 && i < G_N_ELEMENTS (palette_schemes)) {
        color_button =
            GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_text"));
        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(color_button), &fg);
        color_button =
            GTK_WIDGET (gtk_builder_get_object (xml, "colorbutton_back"));
        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(color_button), &bg);
#ifndef VTE_290
        bg.alpha = (config_getbool("enable_transparency") ? GUINT16_TO_FLOAT(config_getint ("back_alpha")) : 1.0);
#endif

        memcpy(current_palette, palette_schemes[i].palette, sizeof(current_palette));

        /* Set terminal palette. */
        for (j=0; j<g_list_length (tw->terms); j++) {
            tt = g_list_nth_data (tw->terms, j);
            vte_terminal_set_colors_rgba (VTE_TERMINAL(tt->vte_term),
                                          &fg,
                                          &bg,
                                          current_palette,
                                          TERMINAL_PALETTE_SIZE);
        }

        for (j=0; j<TERMINAL_PALETTE_SIZE; j++) {
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
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(w), &current_palette[i]);

    /* Why saving the whole palette, not the single color that was set,
     * is if the config file doesn't exist, and we save the single color,
     * then exit, the color always becomes the first color in the config file,
     * no matter which one it actually is, and leaves other colors unset.
     * Obviously this is not what we want.
     * However, maybe there is a better solution for this issue.
     */
    for (i=0; i<TERMINAL_PALETTE_SIZE; i++)
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
        vte_terminal_set_colors_rgba (VTE_TERMINAL (tt->vte_term),
                                      &fg,
                                      &bg,
                                      current_palette,
                                      TERMINAL_PALETTE_SIZE);
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
#ifdef VTE_290
static void check_scroll_background_toggled_cb (GtkWidget *w, tilda_window *tw)
{
    const gboolean status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(w));
    guint i;
    tilda_term *tt;

    config_setbool ("scroll_background", status);

    for (i=0; i<g_list_length (tw->terms); i++) {
        tt = g_list_nth_data (tw->terms, i);
        vte_terminal_set_scroll_background (VTE_TERMINAL(tt->vte_term), status);
    }
}
#endif
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

static void initialize_combo_choose_monitor(tilda_window *tw) {
	/**
	 * First we need to initialize the "combo_choose_monitor" widget,
	 * with the numbers of each monitor attached to the system.
	 */
	GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(tw->window));
	int num_monitors = gdk_screen_get_n_monitors(screen);
	int monitor_number = find_monitor_number(tw);

	GtkComboBox* combo_choose_monitor =
			GTK_COMBO_BOX(gtk_builder_get_object(xml,"combo_choose_monitor"));
	GtkListStore* monitor_model =
			GTK_LIST_STORE(gtk_combo_box_get_model(combo_choose_monitor));
	GtkTreeIter iter;
	gint i;

    gtk_list_store_clear(monitor_model);

    for (i = 0; i < num_monitors; i++) {
        gtk_list_store_append(monitor_model, &iter);
        gchar *monitor_plug_name = gdk_screen_get_monitor_plug_name(screen, i);
        gchar *display_name = g_strdup_printf("%d    <i>(%s)</i>", i, monitor_plug_name);

        gtk_list_store_set(monitor_model, &iter,
                           0, monitor_plug_name,
                           1, i,
                           2, display_name,
                           -1);

        if(i == monitor_number) {
          gtk_combo_box_set_active_iter(combo_choose_monitor, &iter);
        }

        g_free (monitor_plug_name);
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
	GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(tw->window));
	int monitor = find_monitor_number(tw);
	GdkRectangle rectangle;
	gdk_screen_get_monitor_workarea(screen, monitor, &rectangle);
	int monitor_height = rectangle.height;
	int monitor_width = rectangle.width;

    /* Update range and value of height spinners */
    gint height = config_getint("max_height");
	SPIN_BUTTON_SET_RANGE("spin_height_percentage", 0, 100);
	SPIN_BUTTON_SET_VALUE ("spin_height_percentage", percentage_height (monitor_height, config_getint ("max_height")));
    SPIN_BUTTON_SET_RANGE("spin_height_pixels", 0, monitor_height);
	SPIN_BUTTON_SET_VALUE("spin_height_pixels", height);

    /* Update range and value of width spinners */
    gint width = config_getint("max_width");
	SPIN_BUTTON_SET_RANGE("spin_width_percentage", 0, 100);
	SPIN_BUTTON_SET_VALUE ("spin_width_percentage", percentage_width (monitor_width, config_getint ("max_width")));
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
	SPIN_BUTTON_SET_RANGE("spin_x_position", 0, gdk_screen_width());
    SPIN_BUTTON_SET_VALUE("spin_x_position", xpos); /* TODO: Consider x in rectange.x for monitor displacement */

    gint ypos = config_getint("y_pos");
    if(ypos < rectangle.y) {
        ypos = rectangle.y;
        config_setint("y_pos", ypos);
    }
    SPIN_BUTTON_SET_RANGE("spin_y_position", 0, gdk_screen_height());
    SPIN_BUTTON_SET_VALUE("spin_y_position", ypos);

    gtk_window_move(GTK_WINDOW(tw->window), xpos, ypos);

	SET_SENSITIVE_BY_CONFIG_NBOOL("spin_x_position", "centered_horizontally");
	SET_SENSITIVE_BY_CONFIG_NBOOL("label_x_position", "centered_horizontally");
	SET_SENSITIVE_BY_CONFIG_NBOOL("spin_y_position", "centered_vertically");
	SET_SENSITIVE_BY_CONFIG_NBOOL("label_y_position", "centered_vertically");
}

/* Read all state from the config system, and put it into
 * its visual representation in the wizard. */
static void set_wizard_state_from_config (tilda_window *tw) {
    GdkRGBA text_color, back_color, cursor_color;
    gint i;

    /* General Tab */
    CHECK_BUTTON ("check_display_on_all_workspaces", "pinned");
    initialize_set_as_desktop_checkbox ();
    CHECK_BUTTON ("check_always_on_top", "above");
    CHECK_BUTTON ("check_do_not_show_in_taskbar", "notaskbar");
    CHECK_BUTTON ("check_start_tilda_hidden", "hidden");
    CHECK_BUTTON ("check_show_notebook_border", "notebook_border");
    CHECK_BUTTON ("check_enable_double_buffering", "double_buffer");
    COMBO_BOX ("combo_non_focus_pull_up_behaviour", "non_focus_pull_up_behaviour");

    CHECK_BUTTON ("check_terminal_bell", "bell");
    CHECK_BUTTON ("check_cursor_blinks", "blinks");
    COMBO_BOX ("vte_cursor_shape", "cursor_shape");

    CHECK_BUTTON ("check_enable_antialiasing", "antialias");
    CHECK_BUTTON ("check_allow_bold_text", "bold");
    COMBO_BOX ("combo_tab_pos", "tab_pos");
    FONT_BUTTON ("button_font", "font");

    SPIN_BUTTON_SET_RANGE ("spin_auto_hide_time", 0, 99999);
    SPIN_BUTTON_SET_VALUE ("spin_auto_hide_time", config_getint ("auto_hide_time"));
    CHECK_BUTTON ("check_auto_hide_on_focus_lost", "auto_hide_on_focus_lost");
    CHECK_BUTTON ("check_auto_hide_on_mouse_leave", "auto_hide_on_mouse_leave");

    /* Title and Command Tab */
    TEXT_ENTRY ("entry_title", "title");
    COMBO_BOX ("combo_dynamically_set_title", "d_set_title");
    // Whether to limit the length of the title
    CHECK_BUTTON ("check_title_max_length", "title_max_length_flag");
    // The maximum length of the title
    SPIN_BUTTON_SET_RANGE ("spin_title_max_length", 0, 99999);
    SPIN_BUTTON_SET_VALUE ("spin_title_max_length", config_getint ("title_max_length"));
    SET_SENSITIVE_BY_CONFIG_BOOL ("spin_title_max_length", "title_max_length_flag");

    CHECK_BUTTON ("check_run_custom_command", "run_command");
    TEXT_ENTRY ("entry_custom_command", "command");
    CHECK_BUTTON ("check_command_login_shell", "command_login_shell");
    COMBO_BOX ("combo_command_exit", "command_exit");
    COMBO_BOX ("combo_on_last_terminal_exit", "on_last_terminal_exit");
    SET_SENSITIVE_BY_CONFIG_BOOL ("entry_custom_command","run_command");
    SET_SENSITIVE_BY_CONFIG_BOOL ("label_custom_command", "run_command");

    TEXT_ENTRY ("entry_web_browser", "web_browser");

    /* Appearance Tab */
    /* Initialize the monitor chooser combo box with the numbers of the monitor */
	initialize_combo_choose_monitor(tw);


	initialize_geometry_spinners(tw);
    CHECK_BUTTON ("check_enable_transparency", "enable_transparency");
    CHECK_BUTTON ("check_animated_pulldown", "animation");
    SPIN_BUTTON ("spin_animation_delay", "slide_sleep_usec");
    COMBO_BOX ("combo_animation_orientation", "animation_orientation");

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

    for(i = 0;i < TERMINAL_PALETTE_SIZE; i++) {
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

    #if (VTE_290 || VTE_MINOR_VERSION >= 40)
        TEXT_ENTRY ("entry_word_chars", "word_chars");
    #endif

    /* VTE-2.90 Compatibility */
#ifdef VTE_290
    SPIN_BUTTON ("spin_level_of_transparency", "transparency");
    CHECK_BUTTON ("check_scroll_background", "scroll_background");
    CHECK_BUTTON ("check_use_image_for_background", "use_image");
    SET_SENSITIVE_BY_CONFIG_BOOL ("button_background_image","use_image");

    char* filename = config_getstr ("image");
    if(filename != NULL) {
        FILE_BUTTON ("button_background_image", filename);
    }
#else
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(xml, ("spin_level_of_transparency"))), (100 - 100*GUINT16_TO_FLOAT(config_getint("back_alpha"))));
#endif

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
    CONNECT_SIGNAL ("check_enable_double_buffering","toggled",check_enable_double_buffering_toggled_cb, tw);
    CONNECT_SIGNAL ("combo_non_focus_pull_up_behaviour","changed",combo_non_focus_pull_up_behaviour_cb, tw);

    CONNECT_SIGNAL ("check_terminal_bell","toggled",check_terminal_bell_toggled_cb, tw);
    CONNECT_SIGNAL ("check_cursor_blinks","toggled",check_cursor_blinks_toggled_cb, tw);
    CONNECT_SIGNAL ("vte_cursor_shape","changed", combo_cursor_shape_changed_cb, tw);

    CONNECT_SIGNAL ("check_start_fullscreen", "toggled", check_start_fullscreen_cb, tw);

    CONNECT_SIGNAL ("check_enable_antialiasing","toggled",check_enable_antialiasing_toggled_cb, tw);
    CONNECT_SIGNAL ("check_allow_bold_text","toggled",check_allow_bold_text_toggled_cb, tw);
    CONNECT_SIGNAL ("combo_tab_pos","changed",combo_tab_pos_changed_cb, tw);
    CONNECT_SIGNAL ("button_font","font-set",button_font_font_set_cb, tw);

    CONNECT_SIGNAL ("spin_auto_hide_time","value-changed",spin_auto_hide_time_value_changed_cb, tw);
    CONNECT_SIGNAL ("check_auto_hide_on_focus_lost","toggled",check_auto_hide_on_focus_lost_toggled_cb, tw);
    CONNECT_SIGNAL ("check_auto_hide_on_mouse_leave","toggled",check_auto_hide_on_mouse_leave_toggled_cb, tw);

    CONNECT_SIGNAL ("combo_on_last_terminal_exit","changed",combo_on_last_terminal_exit_changed_cb, tw);

    /* Title and Command Tab */
    CONNECT_SIGNAL ("entry_title","changed",entry_title_changed_cb, tw);
    CONNECT_SIGNAL ("combo_dynamically_set_title","changed",combo_dynamically_set_title_changed_cb, tw);
    CONNECT_SIGNAL ("check_title_max_length","toggled",check_max_title_length_cb, tw);
    CONNECT_SIGNAL ("spin_title_max_length","value-changed", spin_max_title_length_changed_cb, tw);

    CONNECT_SIGNAL ("check_run_custom_command","toggled",check_run_custom_command_toggled_cb, tw);
    CONNECT_SIGNAL ("entry_custom_command","focus-out-event", validate_executable_command_cb, tw);
    CONNECT_SIGNAL ("combo_command_exit","changed",combo_command_exit_changed_cb, tw);
    CONNECT_SIGNAL ("check_command_login_shell", "toggled", check_command_login_shell_cb, tw);

    CONNECT_SIGNAL ("entry_web_browser","changed",entry_web_browser_changed, tw);
    CONNECT_SIGNAL ("entry_web_browser","focus-out-event", validate_executable_command_cb, tw);

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
    for(i = 0; i < TERMINAL_PALETTE_SIZE; i++)
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

#if (VTE_290 || VTE_MINOR_VERSION >= 40)
    CONNECT_SIGNAL ("entry_word_chars", "changed", entry_word_chars_changed, tw);
#endif

    /* VTE-2.90 SIGNALS */
#ifdef VTE_290
    CONNECT_SIGNAL ("check_use_image_for_background","toggled",check_use_image_for_background_toggled_cb, tw);
    CONNECT_SIGNAL ("button_background_image","selection-changed",button_background_image_selection_changed_cb, tw);
    CONNECT_SIGNAL ("check_scroll_background","toggled",check_scroll_background_toggled_cb, tw);
#endif
}

/* Initialize the palette scheme menu.
 * Add the predefined schemes to the combo box.*/
static void init_palette_scheme_menu (void)
{
    gint i;
    GtkWidget *combo_palette =
        GTK_WIDGET (gtk_builder_get_object (xml, "combo_palette_scheme"));

    i = G_N_ELEMENTS (palette_schemes);
    while (i > 0) {
        gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (combo_palette), _(palette_schemes[--i].name));
    }
}

static void update_palette_color_button(gint idx)
{
    char *s = g_strdup_printf ("colorbutton_palette_%d", idx);
    GtkWidget *color_button =
        GTK_WIDGET (gtk_builder_get_object (xml, s));

    g_free (s);

    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (color_button), &current_palette[idx]);
}
