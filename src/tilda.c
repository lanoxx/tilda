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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#ifdef HAVE_XFT2
#include <fontconfig/fontconfig.h>
#endif
#include "vte.h"
#include "config.h"
#include "tilda.h"

#define DINGUS1 "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?"
#define DINGUS2 "(((news|telnet|nttp|file|http|ftp|https)://)|(www|ftp)[-A-Za-z0-9]*\\.)[-A-Za-z0-9\\.]+(:[0-9]*)?/[-A-Za-z0-9_\\$\\.\\+\\!\\*\\(\\),;:@&=\\?/~\\#\\%]*[^]'\\.}>\\) ,\\\"]"

GtkWidget *window;
gint max_width, max_height, min_width, min_height;
char config_file[80];

void start_process(char process[])
{ 
	FILE *tmp_file;
	char tmp_name[L_tmpnam];
	char *tmp_filename;
	char tmp_string[255];
	char command[25];

	tmp_filename = tmpnam (tmp_name);
	if ((tmp_file = fopen (tmp_filename, "w+")) == NULL)
	{
		perror ("fopen tmpfile");
		exit (1);
	}
	
	strcpy (command, "ps aux > ");
	strcat (command, tmp_filename);
	
	system (command);
	
	while (!feof (tmp_file))
	{
		fgets (tmp_string, 254, tmp_file);
		
		if (strstr (tmp_string, process) != NULL)
	    {
			goto LABEL;
	    }
	}
	
	system (process);

LABEL:
	fclose (tmp_file);
	remove (tmp_filename);
}

void fix_size_settings ()
{
	gtk_window_resize ((GtkWindow *) window, max_width, max_height);
	gtk_window_get_size ((GtkWindow *) window, &max_width, &max_height);
	gtk_window_resize ((GtkWindow *) window, min_width, min_height);
	gtk_window_get_size ((GtkWindow *) window, &min_width, &min_height);
}

void pull_down ()
{
	FILE *fp;
	
	if((fp = fopen("/tmp/tilda", "w")) == NULL) 
	{
    		perror("fopen");
        	exit(1);
    	}

    	fputs("shits", fp);

    	fclose(fp);
	
	exit (0);
}

int resize (GtkWidget *window, gint w, gint h)
{
	gtk_window_resize ((GtkWindow *) window, w, h);
	
	return 0;
}	

void *wait_for_signal ()
{
	FILE *fp;
	umask(0);
	mknod ("/tmp/tilda", S_IFIFO|0666, 0);
	char c[10];
	int flag;
	gint w, h;	//, x, y;	
	
		//gtk_window_move((GtkWindow *) window, 0, -min_height);
		//resize ((GtkWidget *) window, min_width, min_height);
		//pull_down ();
		//gtk_window_get_position ((GtkWindow *) window, &x, &y);
	
		//printf ("%i %i\n", x, y);
	
	flag = 0;
	
	for (;;)
	{
		if (flag)
		{
			fp = fopen("/tmp/tilda", "r");
			fgets (c, 10, fp);
		}
	
		gtk_window_get_size ((GtkWindow *) window, &w, &h);
	    	//gtk_window_get_position ((GtkWindow *) window, &x, &y);
	
		if (h == min_height)
		{
			resize ((GtkWidget *) window, max_width, max_height);
			gtk_widget_show ((GtkWidget *) window);
			
			gtk_window_stick (GTK_WINDOW (window));
			
			gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
		}
		else if (h == max_height)
	    {	
			resize ((GtkWidget *) window, min_width, min_height);
			//gtk_window_move((GtkWindow *) window, 0, -min_height);
			
		
			gtk_widget_hide ((GtkWidget *) window);	
			
		}
		
		if (flag)
			fclose (fp);
		else
			flag=1;	
		sleep (.1);
	}
	
	return NULL;
}

static void window_title_changed(GtkWidget *widget, gpointer win)
{
	GtkWindow *window;

	g_return_if_fail(VTE_TERMINAL(widget));
	g_return_if_fail(GTK_IS_WINDOW(win));
	g_return_if_fail(VTE_TERMINAL(widget)->window_title != NULL);
	window = GTK_WINDOW(win);

	gtk_window_set_title(window, VTE_TERMINAL(widget)->window_title);
}

static void icon_title_changed(GtkWidget *widget, gpointer win)
{
	GtkWindow *window;

	g_return_if_fail(VTE_TERMINAL(widget));
	g_return_if_fail(GTK_IS_WINDOW(win));
	g_return_if_fail(VTE_TERMINAL(widget)->icon_title != NULL);
	window = GTK_WINDOW(win);

	g_message("Icon title changed to \"%s\".\n",
		  VTE_TERMINAL(widget)->icon_title);
}

static void char_size_changed(GtkWidget *widget, guint width, guint height, gpointer data)
{
	VteTerminal *terminal;
	GtkWindow *window;
	GdkGeometry geometry;
	int xpad, ypad;

	g_return_if_fail(GTK_IS_WINDOW(data));
	g_return_if_fail(VTE_IS_TERMINAL(widget));

	terminal = VTE_TERMINAL(widget);
	window = GTK_WINDOW(data);

	vte_terminal_get_padding(terminal, &xpad, &ypad);

	geometry.width_inc = terminal->char_width;
	geometry.height_inc = terminal->char_height;
	geometry.base_width = xpad;
	geometry.base_height = ypad;
	geometry.min_width = xpad + terminal->char_width * 2;
	geometry.min_height = ypad + terminal->char_height * 2;

	gtk_window_set_geometry_hints(window, widget, &geometry,
				      GDK_HINT_RESIZE_INC |
				      GDK_HINT_BASE_SIZE |
				      GDK_HINT_MIN_SIZE);
}

static void deleted_and_quit(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
	gtk_main_quit();
}

static void destroy_and_quit(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
	gtk_main_quit();
}

static void destroy_and_quit_eof(GtkWidget *widget, gpointer data)
{
	//g_print("Detected EOF.\n");
}

static void destroy_and_quit_exited(GtkWidget *widget, gpointer data)
{
	//g_print("Detected child exit.\n");
	destroy_and_quit(widget, data);
}

static void status_line_changed(GtkWidget *widget, gpointer data)
{
	g_print("Status = `%s'.\n",	vte_terminal_get_status_line(VTE_TERMINAL(widget)));
}

static int button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	VteTerminal *terminal;
	char *match;
	int tag;
	gint xpad, ypad;
	switch (event->button) {
	case 3:
		terminal = VTE_TERMINAL(widget);
		vte_terminal_get_padding(terminal, &xpad, &ypad);
		match = vte_terminal_match_check(terminal,
						 (event->x - ypad) /
						 terminal->char_width,
						 (event->y - ypad) /
						 terminal->char_height,
						 &tag);
		if (match != NULL) {
			g_print("Matched `%s' (%d).\n", match, tag);
			g_free(match);
			if (GPOINTER_TO_INT(data) != 0) {
				vte_terminal_match_remove(terminal, tag);
			}
		}
		break;
	case 1:
	case 2:
	default:
		break;
	}
	return FALSE;
}

static void iconify_window(GtkWidget *widget, gpointer data)
{
	if (GTK_IS_WIDGET(data)) {
		if ((GTK_WIDGET(data))->window) {
			gdk_window_iconify((GTK_WIDGET(data))->window);
		}
	}
}

static void deiconify_window(GtkWidget *widget, gpointer data)
{
	if (GTK_IS_WIDGET(data)) {
		if ((GTK_WIDGET(data))->window) {
			gdk_window_deiconify((GTK_WIDGET(data))->window);
		}
	}
}

static void raise_window(GtkWidget *widget, gpointer data)
{
	if (GTK_IS_WIDGET(data)) {
		if ((GTK_WIDGET(data))->window) {
			gdk_window_raise((GTK_WIDGET(data))->window);
		}
	}
}

static void lower_window(GtkWidget *widget, gpointer data)
{
	if (GTK_IS_WIDGET(data)) {
		if ((GTK_WIDGET(data))->window) {
			gdk_window_lower((GTK_WIDGET(data))->window);
		}
	}
}

static void maximize_window(GtkWidget *widget, gpointer data)
{
	if (GTK_IS_WIDGET(data)) {
		if ((GTK_WIDGET(data))->window) {
			gdk_window_maximize((GTK_WIDGET(data))->window);
		}
	}
}

static void restore_window(GtkWidget *widget, gpointer data)
{
	if (GTK_IS_WIDGET(data)) {
		if ((GTK_WIDGET(data))->window) {
			gdk_window_unmaximize((GTK_WIDGET(data))->window);
		}
	}
}

static void
refresh_window(GtkWidget *widget, gpointer data)
{
	GdkRectangle rect;
	if (GTK_IS_WIDGET(data)) {
		if ((GTK_WIDGET(data))->window) {
			rect.x = rect.y = 0;
			rect.width = (GTK_WIDGET(data))->allocation.width;
			rect.height = (GTK_WIDGET(data))->allocation.height;
			gdk_window_invalidate_rect((GTK_WIDGET(data))->window,
						   &rect, TRUE);
		}
	}
}

static void resize_window(GtkWidget *widget, guint width, guint height, gpointer data)
{
	VteTerminal *terminal;
	gint owidth, oheight, xpad, ypad;
	if ((GTK_IS_WINDOW(data)) && (width >= 2) && (height >= 2)) {
		terminal = VTE_TERMINAL(widget);
		/* Take into account border overhead. */
		gtk_window_get_size(GTK_WINDOW(data), &owidth, &oheight);
		owidth -= terminal->char_width * terminal->column_count;
		oheight -= terminal->char_height * terminal->row_count;
		/* Take into account padding, which needn't be re-added. */
		vte_terminal_get_padding(VTE_TERMINAL(widget), &xpad, &ypad);
		owidth -= xpad;
		oheight -= ypad;
		gtk_window_resize(GTK_WINDOW(data),
				  width + owidth, height + oheight);
	}
}

static void move_window(GtkWidget *widget, guint x, guint y, gpointer data)
{
	if (GTK_IS_WIDGET(data)) {
		if ((GTK_WIDGET(data))->window) {
			gdk_window_move((GTK_WIDGET(data))->window, x, y);
		}
	}
}

static void adjust_font_size(GtkWidget *widget, gpointer data, gint howmuch)
{
	VteTerminal *terminal;
	PangoFontDescription *desired;
	gint newsize;
	gint columns, rows, owidth, oheight;

	/* Read the screen dimensions in cells. */
	terminal = VTE_TERMINAL(widget);
	columns = terminal->column_count;
	rows = terminal->row_count;

	/* Take into account padding and border overhead. */
	gtk_window_get_size(GTK_WINDOW(data), &owidth, &oheight);
	owidth -= terminal->char_width * terminal->column_count;
	oheight -= terminal->char_height * terminal->row_count;

	/* Calculate the new font size. */
	desired = pango_font_description_copy(vte_terminal_get_font(terminal));
	newsize = pango_font_description_get_size(desired) / PANGO_SCALE;
	newsize += howmuch;
	pango_font_description_set_size(desired,
					CLAMP(newsize, 4, 144) * PANGO_SCALE);

	/* Change the font, then resize the window so that we have the same
	 * number of rows and columns. */
	vte_terminal_set_font(terminal, desired);
	gtk_window_resize(GTK_WINDOW(data),
			  columns * terminal->char_width + owidth,
			  rows * terminal->char_height + oheight);

	pango_font_description_free(desired);
}

static void increase_font_size(GtkWidget *widget, gpointer data)
{
	adjust_font_size(widget, data, 1);
}

static void decrease_font_size(GtkWidget *widget, gpointer data)
{
	adjust_font_size(widget, data, -1);
}

static gboolean read_and_feed(GIOChannel *source, GIOCondition condition, gpointer data)
{
	char buf[2048];
	gsize size;
	GIOStatus status;
	g_return_val_if_fail(VTE_IS_TERMINAL(data), FALSE);
	status = g_io_channel_read_chars(source, buf, sizeof(buf),
					 &size, NULL);
	if ((status == G_IO_STATUS_NORMAL) && (size > 0)) {
		vte_terminal_feed(VTE_TERMINAL(data), buf, size);
		return TRUE;
	}
	return FALSE;
}

static void disconnect_watch(GtkWidget *widget, gpointer data)
{
	g_source_remove(GPOINTER_TO_INT(data));
}

static void clipboard_get(GtkClipboard *clipboard, GtkSelectionData *selection_data,
	      guint info, gpointer owner)
{
	/* No-op. */
	return;
}

static void take_xconsole_ownership(GtkWidget *widget, gpointer data)
{
	char *name, hostname[255];
	GdkAtom atom;
	GtkClipboard *clipboard;
	GtkTargetEntry targets[] = {
		{"UTF8_STRING", 0, 0},
		{"COMPOUND_TEXT", 0, 0},
		{"TEXT", 0, 0},
		{"STRING", 0, 0},
	};

	memset(hostname, '\0', sizeof(hostname));
	gethostname(hostname, sizeof(hostname) - 1);

	name = g_strdup_printf("MIT_CONSOLE_%s", hostname);
	atom = gdk_atom_intern(name, FALSE);
#if GTK_CHECK_VERSION(2,2,0)
	clipboard = gtk_clipboard_get_for_display(gtk_widget_get_display(widget),
						  atom);
#else
	clipboard = gtk_clipboard_get(atom);
#endif
	g_free(name);

	gtk_clipboard_set_with_owner(clipboard,
				     targets,
				     G_N_ELEMENTS(targets),
				     clipboard_get,
				     (GtkClipboardClearFunc)gtk_main_quit,
				     G_OBJECT(widget));
}

/*
static void add_weak_pointer(GObject *object, GtkWidget **target)
{
	_weak_pointer(object, (gpointer*)target);
}
*/

int main(int argc, char **argv)
{
	pthread_t child; 
	int pid;
	FILE *fp;
	char *home_dir;
	char s_xbindkeys[5], s_above[5], s_notaskbar[5], s_pinned[5];
	GtkWidget *hbox, *scrollbar, *widget;
	char *env_add[] = {"FOO=BAR", "BOO=BIZ", NULL};
	const char *background = NULL;
	gboolean transparent = FALSE, audible = TRUE, blink = TRUE,
		 dingus = FALSE, geometry = TRUE, dbuffer = TRUE,
		 console = FALSE, scroll = FALSE, /*keep = FALSE,*/
		 icon_title = FALSE, shell = TRUE, highlight_set = FALSE,
		 cursor_set = FALSE;
	//VteTerminalAntiAlias antialias = VTE_ANTI_ALIAS_USE_DEFAULT;
	long lines = 100;
	//const char *message = "Launching interactive shell...\r\n";
	const char *font = NULL;
	const char *terminal = NULL;
	const char *command = NULL;
	const char *working_directory = NULL;
	char **argv2;
	int opt;
	int i, j;
	GList *args = NULL;
	GdkColor fore, back, tint, highlight, cursor;
	/*const char *usage = "Usage: %s "
			    "[ [-B image] | [-T] ] "
			    "[-C] "
			    "[-D] "
			    "[-2] "
			    "[-a] "
			    "[-b] "
			    "[-c command] "
			    "[-d] "
			    "[-f font] "
			    "[-g] "
			    "[-h] "
			    "[-i] "
			    "[-k] "
			    "[-n] "
			    "[-r] "
			    "[-s] "
			    "[-t terminaltype]\n";*/
	back.red = back.green = back.blue = 0xffff;
	fore.red = fore.green = fore.blue = 0x0000;
	highlight.red = highlight.green = highlight.blue = 0xc000;
	cursor.red = 0xffff;
	cursor.green = cursor.blue = 0x8000;
	tint.red = tint.green = tint.blue = 0;
	tint = back;
	
	/* Have to do this early. */
	if (getenv("VTE_PROFILE_MEMORY")) 
	{
		if (atol(getenv("VTE_PROFILE_MEMORY")) != 0) 
		{
			g_mem_set_vtable(glib_mem_profiler_table);
		}
	}

	/* Pull out long options for GTK+. */
	for (i = j = 1; i < argc; i++) 
	{
		if (g_ascii_strncasecmp("--", argv[i], 2) == 0) 
		{
			args = g_list_append(args, argv[i]);
			for (j = i; j < argc; j++) {
				argv[j] = argv[j + 1];
			}
			argc--;
			i--;
		}
	}
	argv2 = g_malloc0(sizeof(char*) * (g_list_length(args) + 2));
	argv2[0] = argv[0];
	for (i = 1; i <= g_list_length(args); i++) 
	{
		argv2[i] = (char*) g_list_nth(args, i - 1);
	}
	argv2[i] = NULL;
	g_assert(i < (g_list_length(args) + 2));


	/*check for -T argument, if there is one just write to the pipe and exit, this will bring down or move up the term*/
	while ((opt = getopt(argc, argv, "B:CDT2abc:df:ghkn:st:w:-")) != -1) 
	 {
     	//gboolean bail = FALSE;
        switch (opt) {
			case 'T':
				pull_down ();
				break;
			case 'C':
				if (wizard (argc, argv) == 1) { return 0; }
				break;
			case 's':
				scroll = TRUE;
				break;
			default:
				break;
		}
	}
	
	home_dir = getenv ("HOME");
	strcpy (config_file, home_dir);
	strcat (config_file, "/.tilda/config");
	
	if((fp = fopen(config_file, "r")) == NULL) 
	{
        	if (wizard (argc, argv) == 1)
			exit (0);
			
		if((fp = fopen(config_file, "r")) == NULL) 
		{  	
			perror("fopen");
        		exit(1);
		}
   	 }

	fscanf (fp, "max_height=%i\n", &max_height);
	fscanf (fp, "max_width=%i\n", &max_width);
	fscanf (fp, "min_height=%i\n", &min_height);
	fscanf (fp, "min_width=%i\n", &min_width);	 
	fscanf (fp, "notaskbar=%s\n", s_notaskbar);
	fscanf (fp, "above=%s\n", s_above);
	fscanf (fp, "pinned=%s\n", s_pinned);
	fscanf (fp, "xbindkeys=%s\n", s_xbindkeys);
	fclose (fp);

	gtk_init(&argc, &argv);

	/* Create a window to hold the scrolling shell, and hook its
	 * delete event to the quit function.. */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_resize_mode(GTK_CONTAINER(window),
				      GTK_RESIZE_IMMEDIATE);
	g_signal_connect(G_OBJECT(window), "delete_event",
			 GTK_SIGNAL_FUNC(deleted_and_quit), window);

	/* Create a box to hold everything. */
	hbox = gtk_hbox_new(0, FALSE);
	gtk_container_add(GTK_CONTAINER(window), hbox);

	/* Create the terminal widget and add it to the scrolling shell. */
	widget = vte_terminal_new();
	if (!dbuffer) 
	{
		gtk_widget_set_double_buffered(widget, dbuffer);
	}
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

	/* Connect to the "char_size_changed" signal to set geometry hints
	 * whenever the font used by the terminal is changed. */
	if (geometry) 
	{
		char_size_changed(widget, 0, 0, window);
		g_signal_connect(G_OBJECT(widget), "char-size-changed",
				 G_CALLBACK(char_size_changed), window);
	}

	/* Connect to the "window_title_changed" signal to set the main
	 * window's title. */
	g_signal_connect(G_OBJECT(widget), "window-title-changed",
			 G_CALLBACK(window_title_changed), window);
	if (icon_title) 
	{
		g_signal_connect(G_OBJECT(widget), "icon-title-changed",
				 G_CALLBACK(icon_title_changed), window);
	}

	/* Connect to the "eof" signal to quit when the session ends. */
	g_signal_connect(G_OBJECT(widget), "eof",
			 G_CALLBACK(destroy_and_quit_eof), window);
	g_signal_connect(G_OBJECT(widget), "child-exited",
			 G_CALLBACK(destroy_and_quit_exited), window);

	/* Connect to the "status-line-changed" signal. */
	g_signal_connect(G_OBJECT(widget), "status-line-changed",
			 G_CALLBACK(status_line_changed), widget);

	/* Connect to the "button-press" event. */
	g_signal_connect(G_OBJECT(widget), "button-press-event",
			 G_CALLBACK(button_pressed), widget);

	/* Connect to application request signals. */
	g_signal_connect(G_OBJECT(widget), "iconify-window",
			 G_CALLBACK(iconify_window), window);
	g_signal_connect(G_OBJECT(widget), "deiconify-window",
			 G_CALLBACK(deiconify_window), window);
	g_signal_connect(G_OBJECT(widget), "raise-window",
			 G_CALLBACK(raise_window), window);
	g_signal_connect(G_OBJECT(widget), "lower-window",
			 G_CALLBACK(lower_window), window);
	g_signal_connect(G_OBJECT(widget), "maximize-window",
			 G_CALLBACK(maximize_window), window);
	g_signal_connect(G_OBJECT(widget), "restore-window",
			 G_CALLBACK(restore_window), window);
	g_signal_connect(G_OBJECT(widget), "refresh-window",
			 G_CALLBACK(refresh_window), window);
	g_signal_connect(G_OBJECT(widget), "resize-window",
			 G_CALLBACK(resize_window), window);
	g_signal_connect(G_OBJECT(widget), "move-window",
			 G_CALLBACK(move_window), window);

	/* Connect to font tweakage. */
	g_signal_connect(G_OBJECT(widget), "increase-font-size",
			 G_CALLBACK(increase_font_size), window);
	g_signal_connect(G_OBJECT(widget), "decrease-font-size",
			 G_CALLBACK(decrease_font_size), window);

	/* Create the scrollbar for the widget. */
	scrollbar = gtk_vscrollbar_new((VTE_TERMINAL(widget))->adjustment);
	gtk_box_pack_start(GTK_BOX(hbox), scrollbar, FALSE, FALSE, 0);
	
	/* Set some defaults. */
	vte_terminal_set_audible_bell(VTE_TERMINAL(widget), audible);
	vte_terminal_set_visible_bell(VTE_TERMINAL(widget), !audible);
	vte_terminal_set_cursor_blinks(VTE_TERMINAL(widget), blink);
	vte_terminal_set_scroll_background(VTE_TERMINAL(widget), scroll);
	vte_terminal_set_scroll_on_output(VTE_TERMINAL(widget), FALSE);
	vte_terminal_set_scroll_on_keystroke(VTE_TERMINAL(widget), TRUE);
	vte_terminal_set_scrollback_lines(VTE_TERMINAL(widget), lines);
	vte_terminal_set_mouse_autohide(VTE_TERMINAL(widget), TRUE);
	if (background != NULL) 
	{
		vte_terminal_set_background_image_file(VTE_TERMINAL(widget),
						       background);
	}
	if (transparent) 
	{
		vte_terminal_set_background_transparent(VTE_TERMINAL(widget),
							TRUE);
	}
	vte_terminal_set_background_tint_color(VTE_TERMINAL(widget), &tint);
	vte_terminal_set_colors(VTE_TERMINAL(widget), &fore, &back, NULL, 0);
	if (highlight_set) 
	{
		vte_terminal_set_color_highlight(VTE_TERMINAL(widget),
						 &highlight);
	}
	if (cursor_set) 
	{
		vte_terminal_set_color_cursor(VTE_TERMINAL(widget), &cursor);
	}
	if (terminal != NULL) 
	{
		vte_terminal_set_emulation(VTE_TERMINAL(widget), terminal);
	}

	/* Set the default font. */
	if (font == NULL) 
	{
		font = "helvetica 15";
	}
	//vte_terminal_set_font_from_string_full(VTE_TERMINAL(widget), font, antialias);

	/* Match "abcdefg". */
	vte_terminal_match_add(VTE_TERMINAL(widget), "abcdefg");
	if (dingus) 
	{
		i = vte_terminal_match_add(VTE_TERMINAL(widget), DINGUS1);
		vte_terminal_match_set_cursor_type(VTE_TERMINAL(widget),
						   i, GDK_GUMBY);
		i = vte_terminal_match_add(VTE_TERMINAL(widget), DINGUS2);
		vte_terminal_match_set_cursor_type(VTE_TERMINAL(widget),
						   i, GDK_HAND1);
	}

	if (console) 
	{
		/* Open a "console" connection. */
		int consolefd = -1, yes = 1, watch;
		GIOChannel *channel;
		consolefd = open("/dev/console", O_RDONLY | O_NOCTTY);
		if (consolefd != -1) 
		{
			/* Assume failure. */
			console = FALSE;
#ifdef TIOCCONS
			if (ioctl(consolefd, TIOCCONS, &yes) != -1) 
			{
				/* Set up a listener. */
				channel = g_io_channel_unix_new(consolefd);
				watch = g_io_add_watch(channel,
						       G_IO_IN,
						       read_and_feed,
						       widget);
				g_signal_connect(G_OBJECT(widget),
						 "eof",
						 G_CALLBACK(disconnect_watch),
						 GINT_TO_POINTER(watch));
				g_signal_connect(G_OBJECT(widget),
						 "child-exited",
						 G_CALLBACK(disconnect_watch),
						 GINT_TO_POINTER(watch));
				g_signal_connect(G_OBJECT(widget),
						 "realize",
						 G_CALLBACK(take_xconsole_ownership),
						 NULL);
				/* Record success. */
				console = TRUE;
			}
#endif
		} else 
		{
			/* Bail back to normal mode. */
			g_warning("Could not open console.\n");
			close(consolefd);
			console = FALSE;
		}
	}

	if (!console) 
	{
		if (shell) 
		{
			/* Launch a shell. */
			vte_terminal_fork_command(VTE_TERMINAL(widget),
						  command, NULL, env_add,
						  working_directory,
						  TRUE, TRUE, TRUE);
			if (command == NULL) 
			{
				vte_terminal_feed_child(VTE_TERMINAL(widget),
							"pwd\n", -1);
			}
		} else 
		{
			long i;
			i = vte_terminal_forkpty(VTE_TERMINAL(widget),
						 env_add, working_directory,
						 TRUE, TRUE, TRUE);
			switch (i) {
			case -1:
				/* abnormal */
				g_warning("Error in vte_terminal_forkpty(): %s",
					  strerror(errno));
				break;
			case 0:
				/* child */
				for (i = 0; ; i++) 
				{
					switch (i % 3) 
					{
					case 0:
					case 1:
						fprintf(stdout, "%ld\n", i);
						break;
					case 2:
						fprintf(stderr, "%ld\n", i);
						break;
					}
					sleep(1);
				}
				_exit(0);
				break;
			default:
				g_print("Child PID is %ld (mine is %ld).\n",
					(long) i, (long) getpid());
				/* normal */
				break;
			}
		}
	}
	
	gtk_window_set_decorated ((GtkWindow *) window, FALSE);
	
	g_object_add_weak_pointer(G_OBJECT(widget), (gpointer*)&widget);
	g_object_add_weak_pointer(G_OBJECT(window), (gpointer*)&window);
	
	gtk_widget_set_size_request ((GtkWidget *) window, 0, 0);
	fix_size_settings ();
	gtk_window_resize ((GtkWindow *) window, min_width, min_height);
	
	if (!scroll)
	{
		gtk_widget_show ((GtkWidget *) widget);
		gtk_widget_show ((GtkWidget *) hbox);
		gtk_widget_show ((GtkWidget *) window);
	}
	else
		gtk_widget_show_all(window);
			
	if ((strcasecmp (s_xbindkeys, "true")) == 0)
		start_process ("xbindkeys");
	if ((strcasecmp (s_above, "true")) == 0)
		gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
	if ((strcasecmp (s_pinned, "true")) == 0)
		gtk_window_stick (GTK_WINDOW (window));
	if ((strcasecmp (s_notaskbar, "true")) == 0)
		gtk_window_set_skip_taskbar_hint (GTK_WINDOW(window), TRUE);
	 
	if ((pid = pthread_create (&child, NULL, &wait_for_signal, NULL)) != 0)
	{
		perror ("Fuck that thread!!!");
	}
	
	gtk_main();

	kill (pid, 9);

	pthread_join (child, NULL);    	

	return 0;
}
