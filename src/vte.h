/*
 * Copyright (C) 2001,2002,2003 Red Hat, Inc.
 *
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

#ifndef vte_vte_h_included
#define vte_vte_h_included

#ident "$Id: vte.h,v 1.1 2004/12/11 20:45:20 kungfooguru Exp $"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <glib.h>
#include <pango/pango.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Private implementation details. */
typedef struct _VteTerminalPrivate VteTerminalPrivate;

/* The terminal widget itself. */
typedef struct _VteTerminal VteTerminal;
struct _VteTerminal {
	/*< public >*/

	/* Widget implementation stuffs. */
	GtkWidget widget;
	GtkAdjustment *adjustment;	/* Scrolling adjustment. */

	/* Metric and sizing data. */
	glong char_width, char_height;	/* dimensions of character cells */
	glong char_ascent, char_descent; /* important font metrics */
	glong row_count, column_count;	/* dimensions of the window */

	/* Titles. */
	char *window_title;
	char *icon_title;

	/*< private >*/
	VteTerminalPrivate *pvt;
};

/* The widget's class structure. */
typedef struct _VteTerminalClass VteTerminalClass;
struct _VteTerminalClass {
	/*< public > */
	/* Inherited parent class. */
	GtkWidgetClass parent_class;

	/*< protected > */
	/* Default signal handlers. */
	void (*eof)(VteTerminal* terminal);
	void (*child_exited)(VteTerminal* terminal);
	void (*emulation_changed)(VteTerminal* terminal);
	void (*encoding_changed)(VteTerminal* terminal);
	void (*char_size_changed)(VteTerminal* terminal, guint char_width, guint char_height);
	void (*window_title_changed)(VteTerminal* terminal);
	void (*icon_title_changed)(VteTerminal* terminal);
	void (*selection_changed)(VteTerminal* terminal);
	void (*contents_changed)(VteTerminal* terminal);
	void (*cursor_moved)(VteTerminal* terminal);
	void (*status_line_changed)(VteTerminal* terminal);
	void (*commit)(VteTerminal* terminal, gchar *text, guint size);

	void (*deiconify_window)(VteTerminal* terminal);
	void (*iconify_window)(VteTerminal* terminal);
	void (*raise_window)(VteTerminal* terminal);
	void (*lower_window)(VteTerminal* terminal);
	void (*refresh_window)(VteTerminal* terminal);
	void (*restore_window)(VteTerminal* terminal);
	void (*maximize_window)(VteTerminal* terminal);
	void (*resize_window)(VteTerminal* terminal, guint width, guint height);
	void (*move_window)(VteTerminal* terminal, guint x, guint y);

	void (*increase_font_size)(VteTerminal* terminal);
	void (*decrease_font_size)(VteTerminal* terminal);

	void (*text_modified)(VteTerminal* terminal);
	void (*text_inserted)(VteTerminal* terminal);
	void (*text_deleted)(VteTerminal* terminal);
	void (*text_scrolled)(VteTerminal* terminal, gint delta);

	/* Padding for future expansion. */
	void (*vte_reserved1)(void);
	void (*vte_reserved2)(void);
	void (*vte_reserved3)(void);
	void (*vte_reserved4)(void);
	void (*vte_reserved5)(void);
	void (*vte_reserved6)(void);

	/*< private > */
	/* Signals we might emit. */
	guint eof_signal;
	guint child_exited_signal;
	guint emulation_changed_signal;
	guint encoding_changed_signal;
	guint char_size_changed_signal;
	guint window_title_changed_signal;
	guint icon_title_changed_signal;
	guint selection_changed_signal;
	guint contents_changed_signal;
	guint cursor_moved_signal;
	guint status_line_changed_signal;
	guint commit_signal;

	guint deiconify_window_signal;
	guint iconify_window_signal;
	guint raise_window_signal;
	guint lower_window_signal;
	guint refresh_window_signal;
	guint restore_window_signal;
	guint maximize_window_signal;
	guint resize_window_signal;
	guint move_window_signal;

	guint increase_font_size_signal;
	guint decrease_font_size_signal;

	guint text_modified_signal;
	guint text_inserted_signal;
	guint text_deleted_signal;
	guint text_scrolled_signal;

	guint reserved1;
	guint reserved2;
	guint reserved3;
	guint reserved4;
	guint reserved5;
	guint reserved6;
};

/* Values for "what should happen when the user hits backspace/delete".  Use
 * AUTO unless the user can cause them to be overridden. */
typedef enum {
	VTE_ERASE_AUTO,
	VTE_ERASE_ASCII_BACKSPACE,
	VTE_ERASE_ASCII_DELETE,
	VTE_ERASE_DELETE_SEQUENCE
} VteTerminalEraseBinding;

/* Values for the anti alias setting */
typedef enum {
	VTE_ANTI_ALIAS_USE_DEFAULT,
	VTE_ANTI_ALIAS_FORCE_ENABLE,
	VTE_ANTI_ALIAS_FORCE_DISABLE
} VteTerminalAntiAlias;

/* The structure we return as the supplemental attributes for strings. */
struct _VteCharAttributes {
	long row, column;
	GdkColor fore, back;
	gboolean underline:1, strikethrough:1;
};
typedef struct _VteCharAttributes VteCharAttributes;

/* The name of the same structure in the 0.10 series, for API compatibility. */
struct vte_char_attributes {
	long row, column;
	GdkColor fore, back;
	gboolean underline:1, strikethrough:1;
};

/* The widget's type. */
GtkType vte_terminal_get_type(void);
GtkType vte_terminal_erase_binding_get_type(void);
GtkType vte_terminal_anti_alias_get_type(void);

#define VTE_TYPE_TERMINAL		(vte_terminal_get_type())
#define VTE_TERMINAL(obj)		(GTK_CHECK_CAST((obj),\
							VTE_TYPE_TERMINAL,\
							VteTerminal))
#define VTE_TERMINAL_CLASS(klass)	GTK_CHECK_CLASS_CAST((klass),\
							     VTE_TYPE_TERMINAL,\
							     VteTerminalClass)
#define VTE_IS_TERMINAL(obj)		GTK_CHECK_TYPE((obj),\
						       VTE_TYPE_TERMINAL)
#define VTE_IS_TERMINAL_CLASS(klass)	GTK_CHECK_CLASS_TYPE((klass),\
							     VTE_TYPE_TERMINAL)
#define VTE_TERMINAL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), VTE_TYPE_TERMINAL, VteTerminalClass))

#define VTE_TYPE_TERMINAL_ERASE_BINDING	(vte_terminal_erase_binding_get_type())
#define VTE_IS_TERMINAL_ERASE_BINDING(obj)	GTK_CHECK_TYPE((obj),\
						VTE_TYPE_TERMINAL_ERASE_BINDING)
#define VTE_TYPE_TERMINAL_ANTI_ALIAS	(vte_terminal_anti_alias_get_type())
#define VTE_IS_TERMINAL_ANTI_ALIAS(obj)		GTK_CHECK_TYPE((obj),\
						VTE_TYPE_TERMINAL_ANTI_ALIAS)

/* You can get by with just these two functions. */
GtkWidget *vte_terminal_new(void);
pid_t vte_terminal_fork_command(VteTerminal *terminal,
				const char *command, char **argv,
				char **envv, const char *directory,
				gboolean lastlog,
				gboolean utmp,
				gboolean wtmp);

/* Users of libzvt may find this useful. */
pid_t vte_terminal_forkpty(VteTerminal *terminal,
			   char **envv, const char *directory,
			   gboolean lastlog,
			   gboolean utmp,
			   gboolean wtmp);

/* Send data to the terminal to display, or to the terminal's forked command
 * to handle in some way.  If it's 'cat', they should be the same. */
void vte_terminal_feed(VteTerminal *terminal, const char *data, glong length);
void vte_terminal_feed_child(VteTerminal *terminal,
			     const char *data, glong length);

/* Copy currently-selected text to the clipboard, or from the clipboard to
 * the terminal. */
void vte_terminal_copy_clipboard(VteTerminal *terminal);
void vte_terminal_paste_clipboard(VteTerminal *terminal);
void vte_terminal_copy_primary(VteTerminal *terminal);
void vte_terminal_paste_primary(VteTerminal *terminal);

/* Set the terminal's size. */
void vte_terminal_set_size(VteTerminal *terminal,
			   glong columns, glong rows);

/* Set various one-off settings. */
void vte_terminal_set_audible_bell(VteTerminal *terminal, gboolean is_audible);
gboolean vte_terminal_get_audible_bell(VteTerminal *terminal);
void vte_terminal_set_visible_bell(VteTerminal *terminal, gboolean is_visible);
gboolean vte_terminal_get_visible_bell(VteTerminal *terminal);
void vte_terminal_set_scroll_background(VteTerminal *terminal, gboolean scroll);
void vte_terminal_set_scroll_on_output(VteTerminal *terminal, gboolean scroll);
void vte_terminal_set_scroll_on_keystroke(VteTerminal *terminal,
					  gboolean scroll);

/* Set the color scheme. */
void vte_terminal_set_color_dim(VteTerminal *terminal,
				const GdkColor *dim);
void vte_terminal_set_color_bold(VteTerminal *terminal,
				 const GdkColor *bold);
void vte_terminal_set_color_foreground(VteTerminal *terminal,
				       const GdkColor *foreground);
void vte_terminal_set_color_background(VteTerminal *terminal,
				       const GdkColor *background);
void vte_terminal_set_color_cursor(VteTerminal *terminal,
				   const GdkColor *cursor_background);
void vte_terminal_set_color_highlight(VteTerminal *terminal,
				      const GdkColor *highlight_background);
void vte_terminal_set_colors(VteTerminal *terminal,
			     const GdkColor *foreground,
			     const GdkColor *background,
			     const GdkColor *palette,
			     glong palette_size);
void vte_terminal_set_default_colors(VteTerminal *terminal);

/* Background effects. */
void vte_terminal_set_background_image(VteTerminal *terminal, GdkPixbuf *image);
void vte_terminal_set_background_image_file(VteTerminal *terminal,
					    const char *path);
void vte_terminal_set_background_tint_color(VteTerminal *terminal,
					    const GdkColor *color);
void vte_terminal_set_background_saturation(VteTerminal *terminal,
					    double saturation);
void vte_terminal_set_background_transparent(VteTerminal *terminal,
					     gboolean transparent);

/* Set whether or not the cursor blinks. */
void vte_terminal_set_cursor_blinks(VteTerminal *terminal, gboolean blink);

/* Set the number of scrollback lines, above or at an internal minimum. */
void vte_terminal_set_scrollback_lines(VteTerminal *terminal, glong lines);

/* Append the input method menu items to a given shell. */
void vte_terminal_im_append_menuitems(VteTerminal *terminal,
				      GtkMenuShell *menushell);

/* Set or retrieve the current font. */
void vte_terminal_set_font(VteTerminal *terminal,
			   const PangoFontDescription *font_desc);
void vte_terminal_set_font_full(VteTerminal *terminal,
				const PangoFontDescription *font_desc,
				VteTerminalAntiAlias antialias);
void vte_terminal_set_font_from_string(VteTerminal *terminal, const char *name);
//void vte_terminal_set_font_from_string_full(VteTerminal *terminal, const char *name, VteTerminalAntiAlias antialias);
const PangoFontDescription *vte_terminal_get_font(VteTerminal *terminal);
gboolean vte_terminal_get_using_xft(VteTerminal *terminal);
void vte_terminal_set_allow_bold(VteTerminal *terminal, gboolean allow_bold);
gboolean vte_terminal_get_allow_bold(VteTerminal *terminal);

/* Check if the terminal is the current selection owner. */
gboolean vte_terminal_get_has_selection(VteTerminal *terminal);

/* Set the list of word chars, optionally using hyphens to specify ranges
 * (to get a hyphen, place it first), and check if a character is in the
 * range. */
void vte_terminal_set_word_chars(VteTerminal *terminal, const char *spec);
gboolean vte_terminal_is_word_char(VteTerminal *terminal, gunichar c);

/* Set what happens when the user strikes backspace or delete. */
void vte_terminal_set_backspace_binding(VteTerminal *terminal,
					VteTerminalEraseBinding binding);
void vte_terminal_set_delete_binding(VteTerminal *terminal,
				     VteTerminalEraseBinding binding);

/* Manipulate the autohide setting. */
void vte_terminal_set_mouse_autohide(VteTerminal *terminal, gboolean setting);
gboolean vte_terminal_get_mouse_autohide(VteTerminal *terminal);

/* Reset the terminal, optionally clearing the tab stops and line history. */
void vte_terminal_reset(VteTerminal *terminal, gboolean full,
			gboolean clear_history);

/* Read the contents of the terminal, using a callback function to determine
 * if a particular location on the screen (0-based) is interesting enough to
 * include.  Each byte in the returned string will have a corresponding
 * VteCharAttributes structure in the passed GArray, if the array was not NULL.
 * Note that it will have one entry per byte, not per character, so indexes
 * should match up exactly. */
char *vte_terminal_get_text(VteTerminal *terminal,
			    gboolean(*is_selected)(VteTerminal *terminal,
						   glong column,
						   glong row,
						   gpointer data),
			    gpointer data,
			    GArray *attributes);
char *vte_terminal_get_text_include_trailing_spaces(VteTerminal *terminal,
						    gboolean(*is_selected)(VteTerminal *terminal,
									   glong column,
									   glong row,
									   gpointer data),
						    gpointer data,
						    GArray *attributes);
char *vte_terminal_get_text_range(VteTerminal *terminal,
				  glong start_row, glong start_col,
				  glong end_row, glong end_col,
				  gboolean(*is_selected)(VteTerminal *terminal,
							 glong column,
							 glong row,
							 gpointer data),
				  gpointer data,
				  GArray *attributes);
void vte_terminal_get_cursor_position(VteTerminal *terminal,
				      glong *column, glong *row);

/* Display string matching:  clear all matching expressions. */
void vte_terminal_match_clear_all(VteTerminal *terminal);

/* Add a matching expression, returning the tag the widget assigns to that
 * expression. */
int vte_terminal_match_add(VteTerminal *terminal, const char *match);
/* Set the cursor to be used when the pointer is over a given match. */
void vte_terminal_match_set_cursor(VteTerminal *terminal, int tag,
				   GdkCursor *cursor);
void vte_terminal_match_set_cursor_type(VteTerminal *terminal,
					int tag, GdkCursorType cursor_type);
/* Remove a matching expression by tag. */
void vte_terminal_match_remove(VteTerminal *terminal, int tag);

/* Check if a given cell on the screen contains part of a matched string.  If
 * it does, return the string, and store the match tag in the optional tag
 * argument. */
char *vte_terminal_match_check(VteTerminal *terminal,
			       glong column, glong row,
			       int *tag);

/* Set the emulation type.  Most of the time you won't need this. */
void vte_terminal_set_emulation(VteTerminal *terminal, const char *emulation);
const char *vte_terminal_get_emulation(VteTerminal *terminal);
const char *vte_terminal_get_default_emulation(VteTerminal *terminal);

/* Set the character encoding.  Most of the time you won't need this. */
void vte_terminal_set_encoding(VteTerminal *terminal, const char *codeset);
const char *vte_terminal_get_encoding(VteTerminal *terminal);

/* Get the contents of the status line. */
const char *vte_terminal_get_status_line(VteTerminal *terminal);

/* Get the padding the widget is using. */
void vte_terminal_get_padding(VteTerminal *terminal, int *xpad, int *ypad);

/* Accessors for bindings. */
GtkAdjustment *vte_terminal_get_adjustment(VteTerminal *terminal);
glong vte_terminal_get_char_width(VteTerminal *terminal);
glong vte_terminal_get_char_height(VteTerminal *terminal);
glong vte_terminal_get_char_descent(VteTerminal *terminal);
glong vte_terminal_get_char_ascent(VteTerminal *terminal);
glong vte_terminal_get_row_count(VteTerminal *terminal);
glong vte_terminal_get_column_count(VteTerminal *terminal);
const char *vte_terminal_get_window_title(VteTerminal *terminal);
const char *vte_terminal_get_icon_title(VteTerminal *terminal);

G_END_DECLS

#endif
