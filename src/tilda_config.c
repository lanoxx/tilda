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
#include "tilda_config.h"
#include "configsys.h"
#include <stdlib.h>

/**
 * Saves the current config object without freeing or closing it.
 */
gboolean tilda_config_save(tilda_config* config) {
    //config_setstr("key", config->key);
}

/**
 * Closes the config backend but saves the current config first.
 */
gboolean tilda_config_close(tilda_config* config) {
    config_free (config->config_file);
}

tilda_config* tilda_config_load(gchar* config_file) {
    /* allocate the config object and initialize it with the config_file path */
    tilda_config* config = malloc(sizeof(tilda_config));
    config->config_file = g_strdup(config_file);

    /* Initialize the libconfuse config system */
    config_init (config_file);

    /* Load options which are overridable by command line */
    config->background_color = config_getstr ("background_color");
    config->command = config_getstr ("command");
    config->font = config_getstr ("font");
    config->image = config_getstr ("image");
    config->working_dir = config_getstr ("working_dir");

    config->lines = config_getint ("lines");
    config->transparency = config_getint ("transparency");
    config->x_pos = config_getint ("x_pos");
    config->y_pos = config_getint ("y_pos");

    config->antialias = config_getbool ("antialias");
    config->scrollbar = config_getbool ("scrollbar");
    config->hidden = config_getbool ("hidden");

    /* Load options which are not overridable by command line */
    config->key = config_getstr("key");
    config->word_chars = config_getstr("word_chars");
    config->title = config_getstr("title");
    config->web_browser = config_getstr("web_browser");

    /* Load integers */
    config->tab_pos = config_getint("tab_pos");
    config->max_width = config_getint("max_width");
    config->max_height = config_getint("max_height");
    config->min_width = config_getint("min_width");
    config->min_height = config_getint("min_height");
    config->d_set_title = config_getint("d_set_title");
    config->scheme = config_getint("scheme");
    config->slide_sleep_usec = config_getint("slide_sleep_usec");
    config->animation_orientation = config_getint("animation_orientation");
    config->palette_scheme = config_getint("palette_scheme");
    config->show_on_monitor_number = config_getint("show_on_monitor_number");
    config->on_last_terminal_exit = config_getint("on_last_terminal_exit");
    config->timer_resolution = config_getint("timer_resolution");
    config->auto_hide_time = config_getint("auto_hide_time");
    config->scrollbar_pos = config_getint("scrollbar_pos");
    config->non_focus_pull_up_behaviour = config_getint("non_focus_pull_up_behaviour");
    config->title_max_length = config_getint("title_max_length");
    config->command_exit = config_getint("command_exit");

    config->back_red = config_getint("back_red");
    config->back_green = config_getint("back_green");
    config->back_blue = config_getint("back_blue");

    config->text_red = config_getint("text_red");
    config->text_green = config_getint("text_green");
    config->text_blue = config_getint("text_blue");

    config->backspace_key = config_getint("backspace_key");
    config->delete_key = config_getint("delete_key");

    /* Load boolean values */
    config->notebook_border = config_getbool("notebook_border");
    config->auto_hide_on_mouse_leave = config_getbool("auto_hide_on_mouse_leave");
    config->auto_hide_on_focus_lost = config_getbool("auto_hide_on_focus_lost");
    config->pinned = config_getbool("pinned");
    config->set_as_desktop = config_getbool("set_as_desktop");
    config->notaskbar = config_getbool("notaskbar");
    config->above = config_getbool("above");
    config->title_max_length_flag = config_getbool("title_max_length_flag");
    config->inherit_working_dir = config_getbool("inherit_working_dir");
    config->run_command = config_getbool("run_command");
    config->bell = config_getbool("bell");
    config->blinks = config_getbool("blinks");
    config->scroll_background = config_getbool("scroll_background");
    config->scroll_on_output = config_getbool("scroll_on_output");
    config->scroll_on_key = config_getbool("scroll_on_key");
    config->bold = config_getbool("bold");
    config->double_buffer = config_getbool("double_buffer");
    config->use_image = config_getbool("use_image");
    config->enable_transparency = config_getbool("enable_transparency");
    config->scroll_history_infinite = config_getbool("scroll_history_infinite");
    config->grab_focus = config_getbool("grab_focus");
    config->animation = config_getbool("animation");
    config->centered_horizontally = config_getbool("centered_horizontally");
    config->centered_vertically = config_getbool("centered_vertically");

    /* The next section loads for each key that is statically listed in key the corresponding value
     * from the config file and stores it into a hash table of strings.
     */
    gchar* keys[] = {"quit_key", "nexttab_key", "prevtab_key",
                    "movetableft_key", "movetabright_key", "addtab_key",
                    "closetab_key", "copy_key", "paste_key", "fullscreen_key",
                    "increase_font_size_key", "decrease_font_size_key", "normalize_font_size_key",
                    "gototab_1_key", "gototab_2_key", "gototab_3_key",
                    "gototab_4_key", "gototab_5_key", "gototab_6_key",
                    "gototab_7_key", "gototab_8_key", "gototab_9_key",
                    "gototab_10_key"};

    /* The hash tables stores all the configurable hotkeys of tilda. */
    config->keys = g_hash_table_new(g_str_hash, g_str_equal);
    for(int i=0; i < G_N_ELEMENTS(keys); i++) {
        gchar* key = config_getstr(keys[i]);
        g_hash_table_insert(config->keys, keys[i], key);
    }
    /* The next section initializes the custom color palette values */
    for(int i=0; i<16; i++) {
        config->palette[i*3] = config_getnint ("palette", i*3);
        config->palette[i*3+1] = config_getnint ("palette", i*3+1);
        config->palette[i*3+2] = config_getnint ("palette", i*3+2);
    }

    return config;
};