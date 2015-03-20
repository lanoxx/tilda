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
#ifndef _TILDA_TILDA_CONFIG_H_
#define _TILDA_TILDA_CONFIG_H_

#include <gtk/gtk.h>

typedef struct tilda_config_ tilda_config;

struct tilda_config_ {
    /* Saves the location of the config file */
    gchar* config_file;

    /* Options overridable by command line */
    gchar *background_color;
    gchar *command;
    gchar *font;
    gchar *image;
    gchar *working_dir;

    gint lines;
    gint transparency;
    gint x_pos;
    gint y_pos;

    gboolean antialias;
    gboolean scrollbar;
    gboolean hidden;

    /* Options not overridable by commandline */

    /*The key used to pull down the tilda window. */
    gchar const *key;
    gchar *word_chars;
    gchar *title;
    gchar *web_browser;

    gint tab_pos;
    gint max_width;
    gint max_height;
    gint min_width;
    gint min_height;
    gint d_set_title;
    gint scheme;
    gint slide_sleep_usec;
    gint animation_orientation;
    gint palette_scheme;
    gint show_on_monitor_number;
    gint on_last_terminal_exit;
    gint timer_resolution;
    gint auto_hide_time;
    gint scrollbar_pos;
    gint non_focus_pull_up_behaviour;
    gint title_max_length;
    gint command_exit;

    gint back_red;
    gint back_green;
    gint back_blue;

    gint text_red;
    gint text_green;
    gint text_blue;

    gint backspace_key;
    gint delete_key;

    /* stores 16*3 == 48 values of a custom color palette */
    gint palette[48];

    gboolean notebook_border;
    gboolean auto_hide_on_mouse_leave;
    gboolean auto_hide_on_focus_lost;
    gboolean pinned;
    gboolean set_as_desktop;
    gboolean notaskbar;
    gboolean above;
    gboolean title_max_length_flag;
    gboolean inherit_working_dir;
    gboolean run_command;
    gboolean bell;
    gboolean blinks;
    gboolean scroll_background;
    gboolean scroll_on_output;
    gboolean scroll_on_key;
    gboolean bold;
    gboolean double_buffer;
    gboolean use_image;
    gboolean enable_transparency;
    gboolean scroll_history_infinite;
    gboolean grab_focus;
    gboolean animation;
    gboolean centered_horizontally;
    gboolean centered_vertically;

    /* Stores a list of user bindable hotkeys which are configurable in the tilda wizard */
    GHashTable *keys;
};

/**
* This saves the config object to the config file. This function must only be called, if
* the global config objects has previously been initialized by calling tilda_config_load()
*/
gboolean       tilda_config_save(tilda_config* config);
gboolean       tilda_config_close(tilda_config* config);

tilda_config*  tilda_config_load(gchar* config_file);

#endif //_TILDA_TILDA_CONFIG_H_
