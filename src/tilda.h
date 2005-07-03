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

#ifndef TILDA_H
#define TILDA_H

/* These are from OpenBSD. They are safe string handling functions.
 * The source was: ftp://ftp.openbsd.org//pub/OpenBSD/src/lib/libc/string/strlcat.c
 *                 ftp://ftp.openbsd.org//pub/OpenBSD/src/lib/libc/string/strlcpy.c
 *
 * Here is a little guide on usage: http://www.courtesan.com/todd/papers/strlcpy.html
 *
 * In short, the syntax is just like strncpy() and strncat().
 */
 
#include <gtk/gtk.h>
#include "tilda_window.h"

G_BEGIN_DECLS;

#define TILDA_VERSION "Tilda 0.07 devel"
#define DEFAULT_LINES 100
#define QUICK_STRCMP(a, b) (*(a)!=*(b)? \
    (int) ((unsigned char) *(a) - (unsigned char) *(b)) : \
    strcmp ((a), (b)))

gboolean image_set_clo, antialias_set_clo, scroll_set_clo;
gint old_max_height, old_max_width;

// commandline arg values
gint x_pos_arg; 
gint y_pos_arg;
gchar s_font_arg[64];
gdouble TRANS_LEVEL_arg;

static const char *usage = "Usage: %s "
    "[-B image] "
    "[-T] "
    "[-C] "
    "[-b [white][black] ] "
    "[-f font] "
    "[-a]"
    "[-h] "
    "[-s] "
    "[-w directory] "
    "[-c command] "
    "[-t level] "
    "[-x position] "
    "[-y position] "
    "[-l lines]\n\n"
    "-B image : set background image\n"
    "-T : Sorry this no longer does anything\n"
    "-C : bring up tilda configuration wizard\n"
    "-b [white][black] : set the background color either white or black\n"
    "-f font : set the font to the following string, ie \"monospace 11\"\n"
    "-a : use antialias fonts\n"
    "-h : show this message\n"
    "-s : use scrollbar\n"
    "-w directory : switch working directory\n"
    "-c command : run command\n"
    "-t level: set transparent to true and set the level of transparency to level, 0-100\n"
    "-x postion: sets the number of pixels from the top left corner to move tilda over\n"
    "-y postion: sets the number of pixels from the top left corner to move tilda down\n"
    "-l lines : set number of scrollback lines\n";
    
void clean_up (tilda_window *tw);
void clean_up_no_args ();

G_END_DECLS;

#endif
