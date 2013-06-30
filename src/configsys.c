/* vim: set ts=4 sts=4 sw=4 expandtab textwidth=112: */

#define _POSIX_SOURCE /* feature test macro for fileno */
#define _XOPEN_SOURCE /* feature test macro for fsync */

#include <tilda-config.h>
#include <debug.h>

#include <confuse.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h> /* atoi */
#include <unistd.h> /* fsync */

#include <configsys.h>

static cfg_t *tc;

/* CONFIGURATION OPTIONS
 * In this array we set the default configuration options for the
 * configuration file.
 */
static cfg_opt_t config_opts[] = {

    /* strings */
    CFG_STR("tilda_config_version", PACKAGE_VERSION, CFGF_NONE),
    CFG_STR("image", NULL, CFGF_NONE),
    CFG_STR("command", "", CFGF_NONE),
    CFG_STR("font", "Monospace 11", CFGF_NONE),
    CFG_STR("key", NULL, CFGF_NONE),
    CFG_STR("addtab_key", "<Shift><Control>t", CFGF_NONE),
    CFG_STR("fullscreen_key", "F11", CFGF_NONE),
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
    CFG_STR("web_browser", "x-www-browser", CFGF_NONE),
    CFG_STR("word_chars", DEFAULT_WORD_CHARS, CFGF_NONE),

    /* ints */
    CFG_INT("lines", 100, CFGF_NONE),
    CFG_INT("max_width", 600, CFGF_NONE),
    CFG_INT("max_height", 150, CFGF_NONE),
    CFG_INT("min_width", 1, CFGF_NONE),
    CFG_INT("min_height", 1, CFGF_NONE),
    CFG_INT("transparency", 0, CFGF_NONE),
    CFG_INT("x_pos", 0, CFGF_NONE),
    CFG_INT("y_pos", 0, CFGF_NONE),
    CFG_INT("tab_pos", 0, CFGF_NONE),
    CFG_INT("backspace_key", 0, CFGF_NONE),
    CFG_INT("delete_key", 1, CFGF_NONE),
    CFG_INT("d_set_title", 3, CFGF_NONE),
    CFG_INT("command_exit", 2, CFGF_NONE),
    CFG_INT("scheme", 3, CFGF_NONE),
    CFG_INT("slide_sleep_usec", 20000, CFGF_NONE),
    CFG_INT("animation_orientation", 0, CFGF_NONE),
    CFG_INT("timer_resolution", 200, CFGF_NONE),
    CFG_INT("auto_hide_time", 2000, CFGF_NONE),
    CFG_INT("on_last_terminal_exit", 0, CFGF_NONE),
    CFG_INT("palette_scheme", 0, CFGF_NONE),
    /* The default monitor number is 0 */
    CFG_INT("show_on_monitor_number", 0, CFGF_NONE),

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

    /* booleans */
    CFG_BOOL("scroll_background", TRUE, CFGF_NONE),
    CFG_BOOL("scroll_on_output", FALSE, CFGF_NONE),
    CFG_BOOL("notebook_border", FALSE, CFGF_NONE),
    CFG_BOOL("antialias", TRUE, CFGF_NONE),
    CFG_BOOL("scrollbar", FALSE, CFGF_NONE),
    CFG_BOOL("use_image", FALSE, CFGF_NONE),
    CFG_BOOL("grab_focus", TRUE, CFGF_NONE),
    CFG_BOOL("above", TRUE, CFGF_NONE),
    CFG_BOOL("notaskbar", TRUE, CFGF_NONE),
    CFG_BOOL("bold", TRUE, CFGF_NONE),
    CFG_BOOL("blinks", TRUE, CFGF_NONE),
    CFG_BOOL("scroll_on_key", TRUE, CFGF_NONE),
    CFG_BOOL("bell", FALSE, CFGF_NONE),
    CFG_BOOL("run_command", FALSE, CFGF_NONE),
    CFG_BOOL("pinned", TRUE, CFGF_NONE),
    CFG_BOOL("animation", FALSE, CFGF_NONE),
    CFG_BOOL("hidden", FALSE, CFGF_NONE),
    CFG_BOOL("centered_horizontally", FALSE, CFGF_NONE),
    CFG_BOOL("centered_vertically", FALSE, CFGF_NONE),
    CFG_BOOL("enable_transparency", FALSE, CFGF_NONE),
    CFG_BOOL("double_buffer", FALSE, CFGF_NONE),
    CFG_BOOL("auto_hide_on_focus_lost", FALSE, CFGF_NONE),
    CFG_BOOL("auto_hide_on_mouse_leave", FALSE, CFGF_NONE),
    CFG_END()
};

/* Define these here, so that we can enable a non-threadsafe version
 * without changing the code below. */
#ifndef NO_THREADSAFE
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	#define config_mutex_lock() g_static_mutex_lock (&mutex)
	#define config_mutex_unlock() g_static_mutex_unlock (&mutex)
#else
	#define config_mutex_lock()
	#define config_mutex_unlock()
#endif

#define CONFIG1_OLDER -1
#define CONFIGS_SAME   0
#define CONFIG1_NEWER  1

static gboolean compare_config_versions (const gchar *config1, const gchar *config2) G_GNUC_UNUSED;



/**
 * Start up the configuration system, using the configuration file given
 * to get the current values. If the configuration file given does not exist,
 * go ahead and write out the default config to the file.
 */
gint config_init (const gchar *config_file)
{
	gint ret = 0;

	tc = cfg_init (config_opts, 0);

	if (g_file_test (config_file,
        G_FILE_TEST_IS_REGULAR))
    {
		/* Read in the existing configuration options */
		ret = cfg_parse (tc, config_file);

		if (ret == CFG_PARSE_ERROR) {
			DEBUG_ERROR ("Problem parsing config");
			g_printerr (_("Problem when opening config file\n"));
			return 1;
		} else if (ret == CFG_PARSE_ERROR) {
			DEBUG_ERROR ("Problem parsing config");
			g_printerr (_("Problem while parsing config file\n"));
        } else if (ret != CFG_SUCCESS) {
            DEBUG_ERROR ("Problem parsing config.");
			g_printerr (_("An unexpected error occured while "
                "parsing the config file\n"));
        }
	}

	return 0;
}

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

gint config_setint (const gchar *key, const gint val)
{
	config_mutex_lock ();
	cfg_setint (tc, key, val);
	config_mutex_unlock ();

	return 0;
}

gint config_setnint(const gchar *key, const gint val, const guint idx)
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

gint config_getint (const gchar *key)
{
	gint temp;

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

    fp = fopen(config_file, "w");

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
