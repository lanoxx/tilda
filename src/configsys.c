/* vim: set ts=4 sts=4 sw=4 expandtab textwidth=112: */
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

#define _POSIX_SOURCE /* feature test macro for fileno */
#define _XOPEN_SOURCE /* feature test macro for fsync */

#include "config.h"
#include "debug.h"

#include <confuse.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h> /* atoi */
#include <unistd.h> /* fsync */

#include "configsys.h"
#include <vte/vte.h>

static cfg_t *tc;

/* CONFIGURATION OPTIONS
 * In this array we set the default configuration options for the
 * configuration file.
 */
static cfg_opt_t config_opts[] = {

    /* strings */
    CFG_STR("tilda_config_version", PACKAGE_VERSION, CFGF_NONE),
    CFG_STR("command", "", CFGF_NONE),
    CFG_STR("font", "Monospace 11", CFGF_NONE),
    CFG_STR("key", NULL, CFGF_NONE),
    CFG_STR("addtab_key", "<Shift><Control>t", CFGF_NONE),
    CFG_STR("fullscreen_key", "F11", CFGF_NONE),
    CFG_STR("toggle_transparency_key", "F12", CFGF_NONE),
    CFG_STR("toggle_searchbar_key", "<Shift><Control>f", CFGF_NONE),
    CFG_STR("closetab_key", "<Shift><Control>w", CFGF_NONE),
    CFG_STR("nexttab_key", "<Control>Page_Down", CFGF_NONE),
    CFG_STR("prevtab_key", "<Control>Page_Up", CFGF_NONE),
    CFG_STR("movetableft_key", "<Shift><Control>Page_Up", CFGF_NONE),
    CFG_STR("movetabright_key", "<Shift><Control>Page_Down", CFGF_NONE),
    CFG_STR("gototab_1_key", "<Alt>1", CFGF_NONE),
    CFG_STR("gototab_2_key", "<Alt>2", CFGF_NONE),
    CFG_STR("gototab_3_key", "<Alt>3", CFGF_NONE),
    CFG_STR("gototab_4_key", "<Alt>4", CFGF_NONE),
    CFG_STR("gototab_5_key", "<Alt>5", CFGF_NONE),
    CFG_STR("gototab_6_key", "<Alt>6", CFGF_NONE),
    CFG_STR("gototab_7_key", "<Alt>7", CFGF_NONE),
    CFG_STR("gototab_8_key", "<Alt>8", CFGF_NONE),
    CFG_STR("gototab_9_key", "<Alt>9", CFGF_NONE),
    CFG_STR("gototab_10_key", "<Alt>0", CFGF_NONE),
    CFG_STR("copy_key", "<Shift><Control>c", CFGF_NONE),
    CFG_STR("paste_key", "<Shift><Control>v", CFGF_NONE),
    CFG_STR("quit_key", "<Shift><Control>q", CFGF_NONE),
    CFG_STR("title", "Tilda", CFGF_NONE),
    CFG_STR("background_color", "white", CFGF_NONE),
    CFG_STR("working_dir", NULL, CFGF_NONE),
    CFG_STR("web_browser", "xdg-open", CFGF_NONE),
    CFG_STR("increase_font_size_key", "<Control>equal", CFGF_NONE),
    CFG_STR("decrease_font_size_key", "<Control>minus", CFGF_NONE),
    CFG_STR("normalize_font_size_key", "<Control>0", CFGF_NONE),
    CFG_STR("show_on_monitor", "", CFGF_NONE),
    CFG_STR("word_chars", DEFAULT_WORD_CHARS, CFGF_NONE),

    /* ints */
    CFG_INT("lines", 5000, CFGF_NONE),
    CFG_INT("x_pos", 0, CFGF_NONE),
    CFG_INT("y_pos", 0, CFGF_NONE),
    CFG_INT("tab_pos", 0, CFGF_NONE),
    CFG_BOOL("expand_tabs", FALSE, CFGF_NONE),
    CFG_BOOL("show_single_tab", FALSE, CFGF_NONE),
    CFG_INT("backspace_key", 0, CFGF_NONE),
    CFG_INT("delete_key", 1, CFGF_NONE),
    CFG_INT("d_set_title", 3, CFGF_NONE),
    CFG_INT("command_exit", 2, CFGF_NONE),
    /* Timeout in milliseconds to spawn a shell or command */
    CFG_INT("command_timeout_ms", 3000, CFGF_NONE),
    CFG_INT("scheme", 3, CFGF_NONE),
    CFG_INT("slide_sleep_usec", 20000, CFGF_NONE),
    CFG_INT("animation_orientation", 0, CFGF_NONE),
    CFG_INT("timer_resolution", 200, CFGF_NONE),
    CFG_INT("auto_hide_time", 2000, CFGF_NONE),
    CFG_INT("on_last_terminal_exit", 0, CFGF_NONE),
    CFG_BOOL("prompt_on_exit", TRUE, CFGF_NONE),
    CFG_INT("palette_scheme", 1, CFGF_NONE),
    CFG_INT("non_focus_pull_up_behaviour", 0, CFGF_NONE),
    CFG_INT("cursor_shape", 0, CFGF_NONE),

    /* The length of a tab title */
    CFG_INT("title_max_length", 25, CFGF_NONE),

    /* int list */
    CFG_INT_LIST("palette", "{\
        0x2e2e, 0x3434, 0x3636,\
        0xcccc, 0x0000, 0x0000,\
        0x4e4e, 0x9a9a, 0x0606,\
        0xc4c4, 0xa0a0, 0x0000,\
        0x3434, 0x6565, 0xa4a4,\
        0x7575, 0x5050, 0x7b7b,\
        0x0606, 0x9820, 0x9a9a,\
        0xd3d3, 0xd7d7, 0xcfcf,\
        0x5555, 0x5757, 0x5353,\
        0xefef, 0x2929, 0x2929,\
        0x8a8a, 0xe2e2, 0x3434,\
        0xfcfc, 0xe9e9, 0x4f4f,\
        0x7272, 0x9f9f, 0xcfcf,\
        0xadad, 0x7f7f, 0xa8a8,\
        0x3434, 0xe2e2, 0xe2e2,\
        0xeeee, 0xeeee, 0xecec}",
        CFGF_NONE),

    /* guint16 */
    CFG_INT("scrollbar_pos", 2, CFGF_NONE),
    CFG_INT("back_red", 0x0000, CFGF_NONE),
    CFG_INT("back_green", 0x0000, CFGF_NONE),
    CFG_INT("back_blue", 0x0000, CFGF_NONE),
    CFG_INT("text_red", 0xffff, CFGF_NONE),
    CFG_INT("text_green", 0xffff, CFGF_NONE),
    CFG_INT("text_blue", 0xffff, CFGF_NONE),
    CFG_INT("cursor_red", 0xffff, CFGF_NONE),
    CFG_INT("cursor_green", 0xffff, CFGF_NONE),
    CFG_INT("cursor_blue", 0xffff, CFGF_NONE),

    /* floats, libconfuse has a bug with floats on non english systems,
     * see: https://github.com/martinh/libconfuse/issues/119, so we
     * need to emulate floats by scaling values to  a long value. */
    CFG_INT ("width_percentage", G_MAXINT, CFGF_NONE),
    CFG_INT ("height_percentage", G_MAXINT, CFGF_NONE),

    /* booleans */
    CFG_BOOL("scroll_history_infinite", FALSE, CFGF_NONE),
    CFG_BOOL("scroll_on_output", FALSE, CFGF_NONE),
    CFG_BOOL("notebook_border", FALSE, CFGF_NONE),

    CFG_BOOL("scrollbar", FALSE, CFGF_NONE),
    CFG_BOOL("grab_focus", TRUE, CFGF_NONE),
    CFG_BOOL("above", TRUE, CFGF_NONE),
    CFG_BOOL("notaskbar", TRUE, CFGF_NONE),
    CFG_BOOL("blinks", TRUE, CFGF_NONE),
    CFG_BOOL("scroll_on_key", TRUE, CFGF_NONE),
    CFG_BOOL("bell", FALSE, CFGF_NONE),
    CFG_BOOL("run_command", FALSE, CFGF_NONE),
    CFG_BOOL("pinned", TRUE, CFGF_NONE),
    CFG_BOOL("animation", FALSE, CFGF_NONE),
    CFG_BOOL("hidden", FALSE, CFGF_NONE),
    CFG_BOOL("set_as_desktop", FALSE, CFGF_NONE),
    CFG_BOOL("centered_horizontally", FALSE, CFGF_NONE),
    CFG_BOOL("centered_vertically", FALSE, CFGF_NONE),
    CFG_BOOL("enable_transparency", FALSE, CFGF_NONE),
    CFG_BOOL("auto_hide_on_focus_lost", FALSE, CFGF_NONE),
    CFG_BOOL("auto_hide_on_mouse_leave", FALSE, CFGF_NONE),
    /* Whether and how we limit the length of a tab title */
    CFG_INT("title_behaviour", 2, CFGF_NONE),
    /* Whether to set a new tab's working dir to the current tab's */
    CFG_BOOL("inherit_working_dir", TRUE, CFGF_NONE),
    CFG_BOOL("command_login_shell", FALSE, CFGF_NONE),
    CFG_BOOL("start_fullscreen", FALSE, CFGF_NONE),
    /* Whether closing a tab shows a confirmation dialog. */
    CFG_BOOL("confirm_close_tab", TRUE, CFGF_NONE),

    CFG_INT("back_alpha", 0xffff, CFGF_NONE),

    /* Whether to show the full tab title as a tooltip */
    CFG_BOOL("show_title_tooltip", FALSE, CFGF_NONE),

    /* Whether match activation with mouse click requires CTRL to be pressed */
    CFG_BOOL("control_activates_match", TRUE, CFGF_NONE),

    /* Whether to enable regular expressions to match
     * certain types of tokens: */
    CFG_BOOL("match_web_uris", TRUE, CFGF_NONE),
    CFG_BOOL("match_file_uris", TRUE, CFGF_NONE),
    CFG_BOOL("match_email_addresses", TRUE, CFGF_NONE),
    CFG_BOOL("match_numbers", TRUE, CFGF_NONE),

    /* if set to TRUE, tilda will fall back to open
     * URIs with the 'web_browser' option. */
    CFG_BOOL("use_custom_web_browser", FALSE, CFGF_NONE),

    /**
     * Deprecated tilda options. These options be commented out in the
     * configuration file and will not be initialized with default values
     * if the option is missing in the config file.
     **/
    CFG_INT("max_width", 0, CFGF_NODEFAULT),
    CFG_INT("max_height", 0, CFGF_NODEFAULT),

    CFG_STR("image", NULL, CFGF_NODEFAULT),

    CFG_INT("show_on_monitor_number", 0, CFGF_NODEFAULT),
    CFG_INT("transparency", 0, CFGF_NODEFAULT),

    CFG_BOOL("bold", TRUE, CFGF_NODEFAULT),
    CFG_BOOL("title_max_length_flag", FALSE, CFGF_NODEFAULT),
    CFG_BOOL("antialias", TRUE, CFGF_NODEFAULT),
    CFG_BOOL("double_buffer", FALSE, CFGF_NODEFAULT),
    CFG_BOOL("scroll_background", FALSE, CFGF_NODEFAULT),
    CFG_BOOL("use_image", FALSE, CFGF_NODEFAULT),

    CFG_INT("min_width", 0, CFGF_NODEFAULT),
    CFG_INT("min_height", 0, CFGF_NODEFAULT),
    /* End deprecated tilda options */

    CFG_END()
};

/* Define these here, so that we can enable a non-threadsafe version
 * without changing the code below. */
#ifndef NO_THREADSAFE
    static GMutex mutex;
    #define config_mutex_lock() g_mutex_lock (&mutex)
    #define config_mutex_unlock() g_mutex_unlock (&mutex)
#else
    #define config_mutex_lock()
    #define config_mutex_unlock()
#endif

#define CONFIG1_OLDER -1
#define CONFIGS_SAME   0
#define CONFIG1_NEWER  1

static gboolean compare_config_versions (const gchar *config1, const gchar *config2) G_GNUC_UNUSED;

static void invoke_deprecation_function(const gchar *const *deprecated_config_options,
                                        guint size);

static void remove_deprecated_config_options(const gchar *const *deprecated_config_options, guint size);

/* Note: set config_file to NULL to just free the
 * data structures, and not write out the state to
 * a file. */
gint config_free (const gchar *config_file)
{
    gint ret = 0;

    if (config_file != NULL)
        ret = config_write (config_file);

    cfg_free (tc);

    return ret;
}

gint config_setint (const gchar *key, const glong val)
{
    config_mutex_lock ();
    cfg_setint (tc, key, val);
    config_mutex_unlock ();

    return 0;
}

gint config_setnint(const gchar *key, const glong val, const guint idx)
{
    config_mutex_lock ();
    cfg_setnint (tc, key, val, idx);
    config_mutex_unlock ();

    return 0;
}

gint config_setdouble (const gchar *key, const gdouble val) {
    config_mutex_lock ();
    cfg_setfloat (tc, key, val);
    config_mutex_unlock ();

    return 0;
}

gint config_setndouble (const gchar *key, const gdouble val, const guint idx) {
    config_mutex_lock ();
    cfg_setnfloat (tc, key, val, idx);
    config_mutex_unlock ();

    return 0;
}

gint config_setstr (const gchar *key, const gchar *val)
{
    config_mutex_lock ();
    cfg_setstr (tc, key, val);
    config_mutex_unlock ();

    return 0;
}

gint config_setbool(const gchar *key, const gboolean val)
{
    config_mutex_lock ();
    cfg_setbool (tc, key, val);
    config_mutex_unlock ();

    return 0;
}

glong config_getint (const gchar *key)
{
    glong temp;

    config_mutex_lock ();
    temp = cfg_getint (tc, key);
    config_mutex_unlock ();

    return temp;
}

glong config_getnint(const gchar *key, const guint idx)
{
    glong temp;

    config_mutex_lock ();
    temp = cfg_getnint (tc, key, idx);
    config_mutex_unlock ();

    return temp;
}

gdouble config_getdouble (const gchar* key) {
    gdouble temp;

    config_mutex_lock ();
    temp = cfg_getfloat (tc, key);
    config_mutex_unlock ();

    return temp;
}

gdouble config_getndouble (const gchar* key, const guint idx) {
    gdouble temp;

    config_mutex_lock ();
    temp = cfg_getnfloat (tc, key, idx);
    config_mutex_unlock ();

    return temp;
}

gchar* config_getstr (const gchar *key)
{
    gchar *temp;

    config_mutex_lock ();
    temp = cfg_getstr (tc, key);
    config_mutex_unlock ();

    return temp;
}

gboolean config_getbool(const gchar *key)
{
    gboolean temp;

    config_mutex_lock ();
    temp = cfg_getbool (tc, key);
    config_mutex_unlock ();

    return temp;
}

/* This will write out the current state of the config file to the disk.
 * It's use is generally discouraged, since config_free() will also write
 * out the configuration to disk. */
gint config_write (const gchar *config_file)
{
    DEBUG_FUNCTION ("config_write");
    DEBUG_ASSERT (config_file != NULL);

    gint ret = 0;
    FILE *fp;

    char *temp_config_file = g_strdup_printf ("%s.tmp", config_file);
    fp = fopen(temp_config_file, "w");

    if (fp != NULL)
    {
        config_mutex_lock ();
        cfg_print (tc, fp);
        config_mutex_unlock ();

        if (fsync (fileno(fp)))
        {
            // Error occurred during sync
            TILDA_PERROR ();
            DEBUG_ERROR ("Unable to sync file");

            g_printerr (_("Unable to sync the config file to disk\n"));
            ret = 2;
        }

        if (fclose (fp))
        {
            // An error occurred
            TILDA_PERROR ();
            DEBUG_ERROR ("Unable to close config file");

            g_printerr (_("Unable to close the config file\n"));
            ret = 3;
        }
        if (rename(temp_config_file, config_file)) {
            TILDA_PERROR ();
            DEBUG_ERROR ("Unable to rename temporary config file to final config file.");
        }
    }
    else
    {
        TILDA_PERROR ();
        DEBUG_ERROR ("Unable to write config file");

        g_printerr (_("Unable to write the config file to %s\n"), config_file);
        ret = 4;
    }

    return ret;
}

/**
 * Start up the configuration system, using the configuration file given
 * to get the current values. If the configuration file given does not exist,
 * go ahead and write out the default config to the file.
 */
gint config_init (const gchar *config_file)
{
    DEBUG_FUNCTION ("config_init");

    gint ret = 0;

    // Can we use a more descriptive name than tc?
    tc = cfg_init (config_opts, 0);

    if (g_file_test (config_file,
        G_FILE_TEST_IS_REGULAR))
    {
        /* Read in the existing configuration options */
        ret = cfg_parse (tc, config_file);

        if (ret == CFG_PARSE_ERROR) {
            DEBUG_ERROR ("Problem parsing config");
            return ret;
        } else if (ret != CFG_SUCCESS) {
            DEBUG_ERROR ("Problem parsing config.");
            return ret;
        }
    }

    /* Deprecate old config settings.
     * This is a lame work around until we get a permanent solution to
     * libconfuse lacking for this functionality
     */
    const gchar *deprecated_tilda_config_options[] = {"show_on_monitor_number",
                                                      "bold",
                                                      "title_max_length_flag",
                                                      "double_buffer",
                                                      "antialias",
                                                      "image",
                                                      "transparency",
                                                      "scroll_background",
                                                      "use_image",
                                                      "min_width",
                                                      "min_height",
                                                      "max_width",
                                                      "max_height"
    };

    invoke_deprecation_function (deprecated_tilda_config_options,
                                 G_N_ELEMENTS(deprecated_tilda_config_options));

    remove_deprecated_config_options(deprecated_tilda_config_options,
                                     G_N_ELEMENTS(deprecated_tilda_config_options));

    #ifndef NO_THREADSAFE
        g_mutex_init(&mutex);
    #endif

    return ret;
}

static GdkMonitor *config_get_configured_monitor ()
{
    gint x_pos = (gint) config_getint ("x_pos");
    gint y_pos = (gint) config_getint ("y_pos");

    GdkDisplay *display = gdk_display_get_default ();

    return gdk_display_get_monitor_at_point (display,
                                             x_pos,
                                             y_pos);
}

void config_get_configured_window_size (GdkRectangle *rectangle)
{
    gdouble relative_width = GLONG_TO_DOUBLE (config_getint ("width_percentage"));
    gdouble relative_height = GLONG_TO_DOUBLE (config_getint ("height_percentage"));

    GdkMonitor *monitor = config_get_configured_monitor ();

    GdkRectangle workarea;
    gdk_monitor_get_workarea (monitor, &workarea);

    rectangle->width = pixels_ratio_to_absolute (relative_width, workarea.width);
    rectangle->height = pixels_ratio_to_absolute (relative_height, workarea.height);
}

static void config_get_configured_percentage (gdouble *width_percentage,
                                              gdouble *height_percentage)
{
    glong windowWidth = config_getint ("max_width");
    glong windowHeight = config_getint ("max_height");

    GdkMonitor *monitor = config_get_configured_monitor ();

    GdkRectangle workarea;
    gdk_monitor_get_workarea (monitor, &workarea);

    if (width_percentage) {
        *width_percentage = pixels_absolute_to_ratio (workarea.width, windowWidth);
    }

    if (height_percentage) {
        *height_percentage = pixels_absolute_to_ratio (workarea.height, windowHeight);
    }
}

static void print_migration_info (const gchar *old_option_name,
                                  const gchar *new_option_name)
{
    g_print ("Migrated deprecated value in option '%s' to '%s'.\n",
             old_option_name, new_option_name);
}

void invoke_deprecation_function (const gchar *const *deprecated_config_options,
                                  guint size)
{
    for (guint i = 0; i < size; i++)
    {
        const char *const option_name = deprecated_config_options[i];

        /* This will still return the option even if its
         * commented out in the config file, so we perform the extra check
         * using `cfg_opt_size` below to determine if the option has a valid
         * value. We do this to ensure that we only execute the migration
         * code once. */
        cfg_opt_t *option = cfg_getopt (tc, option_name);

        if (option == NULL || cfg_opt_size (option) == 0) {
            continue;
        }

        gdouble width_percentage;
        gdouble height_percentage;

        config_get_configured_percentage (&width_percentage,
                                          &height_percentage);

        if (strncmp(option_name, "max_width", sizeof("max_width")) == 0)
        {
            print_migration_info (option_name, "width_percentage");
            config_setint ("width_percentage", GLONG_FROM_DOUBLE (width_percentage));
        }
        if (strncmp(option_name, "max_height", sizeof("max_height")) == 0)
        {
            print_migration_info (option_name, "height_percentage");
            config_setint ("height_percentage", GLONG_FROM_DOUBLE (height_percentage));
        }
    }
}

void remove_deprecated_config_options(const gchar *const *deprecated_config_options, guint size) {
    cfg_opt_t *opt;
    for (guint i =0; i < size; i++) {
        opt = cfg_getopt(tc, deprecated_config_options[i]);
        if (opt->nvalues != 0) {
            g_info("'%s' is no longer a valid config option in the current version of Tilda and has been removed from the config file.", deprecated_config_options[i]);
            cfg_free_value(opt);
        }
    }
}

/*
 * Compares two config versions together.
 *
 * Returns -1 if config1 is older than config2 (UPDATE REQUIRED)
 * Returns  0 if config1 is equal to   config2 (NORMAL USAGE)
 * Returns  1 if config1 is newer than config2 (DISABLE WRITE)
 */
static gboolean compare_config_versions (const gchar *config1, const gchar *config2)
{
    DEBUG_FUNCTION ("compare_config_versions");
    DEBUG_ASSERT (config1 != NULL);
    DEBUG_ASSERT (config2 != NULL);

    /*
     * 1) Split apart both strings using the .'s
     * 2) Compare the major-major version
     * 3) Compare the major version
     * 4) Compare the minor version
     */

    gchar **config1_tokens;
    gchar **config2_tokens;
    gint  config1_version[3];
    gint  config2_version[3];
    gint  i;

    config1_tokens = g_strsplit (config1, ".", 3);
    config2_tokens = g_strsplit (config2, ".", 3);

    for (i=0; i<3; i++)
    {
        config1_version[i] = atoi (config1_tokens[i]);
        config2_version[i] = atoi (config2_tokens[i]);
    }

    g_strfreev (config1_tokens);
    g_strfreev (config2_tokens);

    /* We're done splitting things, so compare now */
    for (i=0; i<3; i++)
    {
        if (config1_version[i] > config2_version[i])
            return CONFIG1_NEWER;

        if (config1_version[i] < config2_version[i])
            return CONFIG1_OLDER;
    }

    return CONFIGS_SAME;
}
