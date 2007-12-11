// vim: set ts=8 sts=8 sw=8 noexpandtab textwidth=112:

#include <tilda-config.h>
#include <debug.h>

#include <confuse.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h> /* atoi */
#include <unistd.h> /* fsync */

#include <configsys.h>
#include <translation.h>
#include <key_converter.h>


static cfg_t *tc;

/* CONFIGURATION OPTIONS */
static cfg_opt_t config_opts[] = {

    /* strings */
    CFG_STR("tilda_config_version", PACKAGE_VERSION, CFGF_NONE),
    CFG_STR("image", NULL, CFGF_NONE),
    CFG_STR("command", "", CFGF_NONE),
    CFG_STR("font", "Monospace 13", CFGF_NONE),
    CFG_STR("key", NULL, CFGF_NONE),
    CFG_STR("title", "Tilda", CFGF_NONE),
    CFG_STR("background_color", "white", CFGF_NONE),
    CFG_STR("working_dir", NULL, CFGF_NONE),
    CFG_STR("web_browser", "firefox", CFGF_NONE),

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
    CFG_INT("slide_sleep_usec", 15000, CFGF_NONE),
    CFG_INT("animation_orientation", 0, CFGF_NONE),

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
    CFG_END()
};

static gboolean config_writing_disabled = FALSE;

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

static void try_to_update_config_file (const gchar *config_file);
static gboolean compare_config_versions (const gchar *config1, const gchar *config2);



/**
 * Start up the configuration system, using the configuration file given
 * to get the current values. If the configuration file given does not exist,
 * go ahead and write out the default config to the file.
 */
gint config_init (const gchar *config_file)
{
	gint ret = 0;

	tc = cfg_init (config_opts, 0);

	/* I know that this is racy, but I can't think of a good
	 * way to fix it ... */
	if (g_file_test (config_file, G_FILE_TEST_IS_REGULAR)) {
		/* Read the file, and try to upgrade it */
		ret = cfg_parse (tc, config_file);

		if (ret == CFG_SUCCESS)
			try_to_update_config_file (config_file);
		else {
			DEBUG_ERROR ("Problem parsing config");
			g_printerr (_("Problem parsing config file\n"));
			return 1;
		}
	}
	/* This is commented out because we don't want to do this. Basically, there
	 * is no need to write the config until we have shown the wizard, which is
	 * automatically shown on the first run. */
#if 0
	else {
		/* Write out the defaults */
		config_write (config_file);
	}
#endif

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

    /* Check to see if writing is disabled. Leave early if it is. */
    if (config_writing_disabled)
        return 1;

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

/*
 * Compare config file versions and see if we know about a new version.
 *
 * If we have a version newer than what we know about, we need to stop
 * anything from writing to the config file.
 *
 * If we are at the same config version, we're done, so exit early.
 *
 * If the config is older than we are now, update it and then write
 * it to disk.
 */
static void try_to_update_config_file (const gchar *config_file)
{
    DEBUG_FUNCTION ("try_to_update_config_file");
    DEBUG_ASSERT (config_file != NULL);

    gboolean changed = FALSE;
    gchar *current_config = config_getstr ("tilda_config_version");

    if (compare_config_versions (current_config, PACKAGE_VERSION) == CONFIGS_SAME)
        return; // Same version as ourselves, we're done!

    if (compare_config_versions (current_config, PACKAGE_VERSION) == CONFIG1_NEWER)
    {
        // We have a version newer than ourselves!
        // Disable writing to the config for safety.
        config_writing_disabled = TRUE;
        return; // Out early, since we won't be able to update anyway!
    }

    /* NOTE: Start with the oldest config version first, and work our way up to the
     * NOTE: newest that we support, updating our current version each time.
     *
     * NOTE: You may need to re-read the config each time! Probably not though,
     * NOTE: since you should be updating VALUES not names directly in the config.
     * NOTE: Try to rely on libconfuse to generate the configs :) */

    /* Below is a template for creating new entries in the updater. If you ever
     * change anything between versions, copy this, replacing YOUR_VERSION
     * with the new version that you are making. */
#if 0
    if (compare_config_versions (current_config, YOUR_VERSION) == CONFIG1_OLDER)
    {
        // TODO: Add things here to migrate from whatever we are to YOUR_VERSION
        current_config = YOUR_VERSION;
        changed = TRUE;
    }
#endif

    if (compare_config_versions (current_config, "0.09.4") == CONFIG1_OLDER)
    {
        /* Nothing to update here. All we did was add an option, there is no
         * need to rewrite the config file here, since the writer at the end
         * will automatically add the default value of the new option. */
        current_config = "0.09.4";
        changed = TRUE;
    }

    if (compare_config_versions (current_config, "0.9.5") == CONFIG1_OLDER)
    {
        char *old_key;
        char *new_key;

        old_key = config_getstr ("key");
        new_key = upgrade_key_to_095 (old_key);

        config_setstr ("key", new_key);
        free (new_key);

        current_config = "0.9.5";
        changed = TRUE;
    }

    /* We've run through all the updates, so set our config file version to the
     * version we're at now, then write out the config file.
     *
     * NOTE: this only happens if we upgraded the config, due to some early-exit
     * logic above.
     */

    if (changed)
    {
        config_setstr ("tilda_config_version", current_config);
        config_write (config_file);
    }
}

