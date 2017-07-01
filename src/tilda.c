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
#define _POSIX_SOURCE /* feature test macro for signal functions */
#define _XOPEN_SOURCE /* feature test macro for popen */

/*
 * This message is shown in a modal dialog when tilda starts and there is a problem parsing the configuration file.
 * Such problems can occur for example if the config file contains a key-value pair that is unknown to tilda. This
 * can be the case if the user manually modified the configuration file or if a newer version of tilda was run and
 * which saved some settings that are not known by older versions of tilda.
 */
#define TILDA_CONFIG_PARSE_ERROR \
    "<b>A problem occurred while parsing the config file.</b>\n\n" \
    "This can happen if the tilda config contains a setting that is unknown to tilda. " \
    "Tilda will now start with the default configuration."

/*
 * This message is shown in a modal dialog when tilda starts and there is a any other problem with the configuration
 * file except a parse error.
 */
#define TILDA_CONFIG_OTHER_ERROR \
    "<b>An unexpected error occured while parsing the config file.</b>\n\n" \
    "The default configuration will be used instead. This error can occur if the configuration file is corrupted" \
    "or otherwise unreadable. Tilda will now start with a default configuration."


#include <tilda-config.h>

#include "debug.h"
#include "tilda.h"
#include "callback_func.h"
#include "configsys.h"
#include "tilda_window.h"
#include "key_grabber.h" /* for pull */
#include "wizard.h"
#include "xerror.h"
#include "tomboykeybinder.h"
#include "tilda-keybinding.h"

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/dir.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <stdio.h>
#include <signal.h>
#include <pwd.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>

#include <vte/vte.h>
#include <glib/gstdio.h>

/**
* If lock->pid is 0 then the file is not opened exclusively. Instead flock() must be used to obtain a lock.
* Otherwise an exclusive lock file is created for the process.
*/
static gchar *create_lock_file (struct lock_info *lock)
{
    DEBUG_FUNCTION ("create_lock_file");
    DEBUG_ASSERT (lock != NULL);
    DEBUG_ASSERT (lock->instance >= 0);
    DEBUG_ASSERT (lock->pid >= 0);

    gint ret;
    gchar *lock_file_full;
    gchar *lock_dir = g_build_filename (g_get_user_cache_dir (), "tilda", "locks", NULL);
    gchar *lock_file = g_strdup_printf ("lock_%d_%d", lock->pid, lock->instance);

    /* Make the ~/.cache/tilda/locks directory */
    ret = g_mkdir_with_parents (lock_dir,  S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret == -1)
        goto mkdir_fail;

    /* Create the full path to the lock file */
    lock_file_full = g_build_filename (lock_dir, lock_file, NULL);

    /* Create the lock file */
    if(lock->pid == 0) {
        ret = g_open(lock_file_full, O_CREAT, S_IRUSR | S_IWUSR);
    } else {
        ret = g_open(lock_file_full, O_WRONLY | O_CREAT | O_EXCL, 0);
    }

    if (ret == -1)
        goto creat_fail;

    lock->file_descriptor = ret;

    g_free (lock_file);
    g_free (lock_dir);

    return lock_file_full;

    /* Free memory and return NULL */
creat_fail:
    g_free (lock_file_full);
mkdir_fail:
    g_free (lock_file);
    g_free (lock_dir);

    return NULL;
}

/**
 * Check if a filename corresponds to a valid lockfile. Note that this
 * routine does NOT check whether it is a stale lock, however. This
 * will return the lock file's corresponding pid, if it is a valid lock.
 *
 * @param filename the filename to check
 * @return a new struct lock_info
 *
 * Success: struct lock_info will be filled in and non-NULL
 * Failure: return NULL
 */
static struct lock_info *islockfile (const gchar *filename)
{
    DEBUG_FUNCTION ("islockfile");
    DEBUG_ASSERT (filename != NULL);

    struct lock_info *lock;

    lock = g_malloc (sizeof (struct lock_info));

    if (lock == NULL)
        return NULL;

#ifdef USE_GLIB_2_14
    GRegex *regex;
    GMatchInfo *match_info;
    gboolean matched;
    gchar *temp;

    regex = g_regex_new ("^lock_(\d+)_(\d+)$", 0, 0, NULL);
    matched = g_regex_match (regex, filename, 0, &match_info);

    if (!matched)
    {
        g_free (lock);
        lock = NULL;
        goto nomatch;
    }

    /* Get the lock pid */
    temp = g_match_info_fetch (match_info, 0);
    lock->pid = atoi (temp);
    g_free (temp);

    /* Get the configuration number */
    temp = g_match_info_fetch (match_info, 1);
    lock->instance = atoi (temp);
    g_free (temp);

nomatch:
    if (match_info)
        g_match_info_free (match_info);

    g_regex_unref (regex);

    return matched;

#else /* Glib version is <2.14 */

    gboolean matches = g_str_has_prefix (filename, "lock_");
    gboolean islock = FALSE;
    gchar *pid_s, *instance_s;

    if (matches) /* we are prefixed with "lock_" and are probably a lock */
    {
        pid_s = strstr (filename, "_");

        if (pid_s) /* we have a valid pid */
        {
            /* Advance the pointer past the underscore */
            pid_s++;

            lock->pid = atoi (pid_s);
            instance_s = strstr (pid_s, "_");

            if (lock->pid > 0 && instance_s)
            {
                /* Advance the pointer past the underscore */
                instance_s++;

                /* Extract the instance number and store it */
                lock->instance = atoi (instance_s);

                /* we parsed everything, so yes, we were a lock */
                islock = TRUE;
            }
        }
    }

    if (!islock)
    {
        g_free (lock);
        lock = NULL;
    }

    return lock;
#endif
}

/**
* Gets a list of the pids in
*/
static GSList *getPids() {
    GSList *pids = NULL;
    FILE *ps_output;
    const gchar ps_command[] = "ps -C tilda -o pid=";
    gchar buf[16]; /* Really shouldn't need more than 6 */

    if ((ps_output = popen (ps_command, "r")) == NULL) {
        g_printerr (_("Unable to run command: `%s'\n"), ps_command);
        return NULL;
    }

    /* The popen() succeeded, get all of the pids */
    while (fgets (buf, sizeof(buf), ps_output) != NULL)
        pids = g_slist_append (pids, GINT_TO_POINTER (atoi (buf)));

    /* We've read all of the pids, exit */
    pclose (ps_output);
    return pids;
}

/**
 * Remove stale lock files from the ~/.tilda/locks/ directory.
 *
 * Success: returns 0
 * Failure: returns non-zero
 */
static gint remove_stale_lock_files ()
{
    DEBUG_FUNCTION ("remove_stale_lock_files");

    GSList *pids = getPids();
    if(pids == NULL) {
        return -1;
    }

    struct lock_info *lock;
    gchar *lock_dir = g_build_filename (g_get_user_cache_dir (), "tilda", "locks", NULL);
    gchar *remove_file;
    gchar *filename;
    GDir *dir;

    /* Open the lock directory for reading */
    dir = g_dir_open (lock_dir, 0, NULL);

    if (dir == NULL)
    {
        g_printerr (_("Unable to open lock directory: %s\n"), lock_dir);
        g_free (lock_dir);
        return -2;
    }

    /* For each possible lock file, check if it is a lock, and see if
     * it matches one of the running tildas */
    while ((filename = (gchar*) g_dir_read_name (dir)) != NULL)
    {
        lock = islockfile (filename);

        if (lock && (g_slist_find (pids, GINT_TO_POINTER (lock->pid)) == NULL))
        {
            /* We have found a stale element */
            remove_file = g_build_filename (lock_dir, filename, NULL);
            g_remove (remove_file);
            g_free (remove_file);
        }

        if (lock)
            g_free (lock);
    }

    g_dir_close (dir);
    g_free (lock_dir);
    g_slist_free(pids);

    return 0;
}

/**
 * Parse all of the Command-Line Options given to tilda.
 * This can modify argv and argc, and will set values in the config.
 *
 * @param cli_options pointer to a struct to store command-line options into
 * @param argc argc from main
 * @param argv argv from main
 * @param config_file pointer to config file path if specified via command-line
 * @return TRUE if we should show the configuration wizard, FALSE otherwise
 */
static gboolean parse_cli (int argc, char *argv[], tilda_cli_options *cli_options, gchar **config_file)
{
    DEBUG_FUNCTION ("parse_cli");
    DEBUG_ASSERT (argc != 0);
    DEBUG_ASSERT (argv != NULL);
    // *config_file must be non-null only if a configuration file path has been parsed
    DEBUG_ASSERT (*config_file == NULL);

    /* All of the various command-line options */
    GOptionEntry cl_opts[] = {
        { "antialias",          'a', 0, G_OPTION_ARG_NONE,      &(cli_options->antialias),         N_("Use Antialiased Fonts"), NULL },
        { "background-color",   'b', 0, G_OPTION_ARG_STRING,    &(cli_options->background_color),  N_("Set the background color"), NULL },
        { "command",            'c', 0, G_OPTION_ARG_STRING,    &(cli_options->command),           N_("Run a command at startup"), NULL },
        { "hidden",             'h', 0, G_OPTION_ARG_NONE,      &(cli_options->hidden),            N_("Start Tilda hidden"), NULL },
        { "font",               'f', 0, G_OPTION_ARG_STRING,    &(cli_options->font),              N_("Set the font to the following string"), NULL },
        { "config-file",        'g', 0, G_OPTION_ARG_STRING,    config_file,                       N_("Configuration file"), NULL },
        { "lines",              'l', 0, G_OPTION_ARG_INT,       &(cli_options->lines),             N_("Scrollback Lines"), NULL },
        { "scrollbar",          's', 0, G_OPTION_ARG_NONE,      &(cli_options->scrollbar),         N_("Use Scrollbar"), NULL },
        { "version",            'v', 0, G_OPTION_ARG_NONE,      &(cli_options->version),           N_("Print the version, then exit"), NULL },
        { "working-dir",        'w', 0, G_OPTION_ARG_STRING,    &(cli_options->working_dir),       N_("Set Initial Working Directory"), NULL },
        { "x-pos",              'x', 0, G_OPTION_ARG_INT,       &(cli_options->x_pos),             N_("X Position"), NULL },
        { "y-pos",              'y', 0, G_OPTION_ARG_INT,       &(cli_options->y_pos),             N_("Y Position"), NULL },
#ifdef VTE_290
        { "image",              'B', 0, G_OPTION_ARG_STRING,    &(cli_options->image),             N_("Set Background Image"), NULL },
        { "transparency",       't', 0, G_OPTION_ARG_INT,       &(cli_options->transparency),      N_("Opaqueness: 0-100%"), NULL },
#else
        { "background-alpha",   't', 0, G_OPTION_ARG_INT,       &(cli_options->back_alpha),        N_("Opaqueness: 0-100%"), NULL },
#endif
        { "config",             'C', 0, G_OPTION_ARG_NONE,      &(cli_options->show_config),       N_("Show Configuration Wizard"), NULL },
        { NULL }
    };

    /* Set up the command-line parser */
    GError *error = NULL;
    GOptionContext *context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, cl_opts, NULL);
    g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free (context);

    /* Check for unknown options, and give a nice message if there are some */
    if (error)
    {
        g_printerr (_("Error parsing command-line options. Try \"tilda --help\"\nto see all possible options.\n\nError message: %s\n"),
                    error->message);

        exit (EXIT_FAILURE);
    }

    /* If we need to show the version, show it then exit normally */
    if (cli_options->version)
    {
        g_print ("%s\n\n", TILDA_VERSION);

        g_print ("Copyright (c) 2012-2013 Sebastian Geiger (lanoxx@gmx.net)\n");
        g_print ("Copyright (c) 2005-2009 Tristan Sloughter (sloutri@iit.edu)\n");
        g_print ("Copyright (c) 2005-2009 Ira W. Snyder (tilda@irasnyder.com)\n\n");

        g_print ("General Information: https://github.com/lanoxx/tilda\n");
        g_print ("Bug Reports: https://github.com/lanoxx/tilda/issues?state=open\n\n");

        g_print ("This program comes with ABSOLUTELY NO WARRANTY.\n");
        g_print ("This is free software, and you are welcome to redistribute it\n");
        g_print ("under certain conditions. See the file COPYING for details.\n");

        exit (EXIT_SUCCESS);
    }

    /* This block is only used to initialize the Glib and GTK internal options. That way the users can pass additional command line options,
     * that are used by Glib and GTK. We do this separate from the above options, because we pass TRUE to the gtk_get_option group function
     * which causes GTK to initialize the default display. This way it is possible to invoke `tilda --version` without getting an
     * error if there is no display available.
     */
    error = NULL;
    context = g_option_context_new (NULL);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free (context);

    if (error)
    {
        g_printerr (_("Error parsing Glib and GTK specific command-line options. Try \"tilda --help-all\"\nto see all possible options.\n\nError message: %s\n"),
                    error->message);

        exit (EXIT_FAILURE);
    }

    /* TRUE if we should show the config wizard, FALSE otherwize */
    return cli_options->show_config;
}

/**
 * Initialize a structure in which command-line parameters will be stored.
 * @return a pointer to that structure
 */
tilda_cli_options *init_cli_options()
{
    tilda_cli_options *options = g_malloc0(sizeof(tilda_cli_options));
    if (!options)
    {
        g_printerr (_("Error allocating memory for a new tilda_cli_options structure.\n"));
        exit (EXIT_FAILURE);
    }

    return options;
}

/**
 * Set values in the config from command-line parameters
 *
 * @param cli_options pointer to a struct containing command-line options
 */
static void setup_config_from_cli_options(tilda_cli_options *cli_options)
{
    if (cli_options->background_color != NULL
            && cli_options->background_color != config_getstr ("background_color")) {
        config_setstr ("background_color", cli_options->background_color);

        GdkColor col;
        if (gdk_color_parse(cli_options->background_color, &col)) {
            config_setint("back_red", col.red);
            config_setint("back_green", col.green);
            config_setint("back_blue", col.blue);
        }

        g_free(cli_options->background_color);
    }
    if (cli_options->command != NULL
            && cli_options->command != config_getstr ("command"))
    {
        config_setbool ("run_command", TRUE);
        config_setstr ("command", cli_options->command);
        g_free(cli_options->command);
    }
    if (cli_options->font != NULL
            && cli_options->font != config_getstr ("font")) {
        config_setstr ("font", cli_options->font);
        g_free(cli_options->font);
    }
#ifdef VTE_290
    if (cli_options->image != NULL
            && cli_options->image != config_getstr ("image")) {
        config_setstr ("image", cli_options->image);
        g_free(cli_options->image);
    }
    if (cli_options->transparency != 0
            && cli_options->transparency != config_getint ("transparency"))
    {
        config_setbool ("enable_transparency", cli_options->transparency);
        config_setint ("transparency", cli_options->transparency);
    }
#else
    if (cli_options->back_alpha != 0
            && cli_options->back_alpha != config_getint ("back_alpha"))
    {
        config_setbool ("enable_transparency", ~cli_options->back_alpha & 0xffff);
        config_setint ("back_alpha", cli_options->back_alpha);
    }
#endif
    if (cli_options->working_dir != NULL
            && cli_options->working_dir != config_getstr ("working_dir")) {
        config_setstr ("working_dir", cli_options->working_dir);
        g_free(cli_options->working_dir);
    }

    if (cli_options->lines != 0
            && cli_options->lines != config_getint ("lines"))
        config_setint ("lines", cli_options->lines);
    if (cli_options->x_pos != 0
            && cli_options->x_pos != config_getint ("x_pos"))
        config_setint ("x_pos", cli_options->x_pos);
    if (cli_options->y_pos != 0
            && cli_options->y_pos != config_getint ("y_pos"))
        config_setint ("y_pos", cli_options->y_pos);

    if (cli_options->antialias != FALSE
            && cli_options->antialias != config_getbool ("antialias"))
        config_setbool ("antialias", cli_options->antialias);
    if (cli_options->hidden != FALSE
            && cli_options->hidden != config_getbool ("hidden"))
        config_setbool ("hidden", cli_options->hidden);
    if (cli_options->scrollbar != FALSE
            && cli_options->scrollbar != config_getbool ("scrollbar"))
        config_setbool ("scrollbar", cli_options->scrollbar);
}

/**
 * Get a pointer to the config file name for this instance.
 *
 * NOTE: you MUST call free() on the returned value!!!
 *
 * @param tw the tilda_window structure corresponding to this instance
 * @return a pointer to a string representation of the config file's name
 */
static gchar *get_config_file_name (gint instance)
{
    DEBUG_FUNCTION ("get_config_file_name");
    DEBUG_ASSERT (instance >= 0);

    gchar *config_dir = g_build_filename (g_get_user_config_dir (), "tilda", NULL);

    /* Make the ~/.config/tilda directory */
    if (!g_file_test (config_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
        g_print (_("Creating directory:'%s'\n"), config_dir);
        g_mkdir_with_parents (config_dir,  S_IRUSR | S_IWUSR | S_IXUSR);
    }

    gchar *config_file_prefix = g_build_filename (config_dir, "config_", NULL);
    gchar *config_file;

    config_file = g_strdup_printf ("%s%d", config_file_prefix, instance);
    g_free (config_file_prefix);

    return config_file;
}

static gint _cmp_locks(gint a, gint b) {
  return a - b;
}

/**
 * get_instance_number ()
 *
 * Gets the next available tilda instance number. This will always pick the
 * lowest non-running tilda available.
 *
 * Success: return next available instance number (>=0)
 * Failure: return 0
 */
static gint get_instance_number ()
{
    DEBUG_FUNCTION ("get_instance_number");

    gchar *name;

    GSequence *seq;
    GSequenceIter *iter;
    gint lowest_lock_instance = 0;
    gint current_lock_instance;

    GDir *dir;
    struct lock_info *lock;
    gchar *lock_dir = g_build_filename (g_get_user_cache_dir (), "tilda", "locks", NULL);

    /* Open the lock directory */
    dir = g_dir_open (lock_dir, 0, NULL);

    /* Check for failure to open */
    if (dir == NULL)
    {
        g_printerr (_("Unable to open lock directory: %s\n"), lock_dir);
        g_free (lock_dir);
        return 0;
    }

    /* Look through every file in the lock directory, and see if it is a lock file.
     * If it is a lock file, insert it in a sorted sequence. */
    seq = g_sequence_new(NULL);
    while ((name = (gchar*)g_dir_read_name (dir)) != NULL)
    {
        lock = islockfile (name);

        if (lock != NULL)
        {
            g_sequence_insert_sorted(seq, GINT_TO_POINTER(lock->instance), (GCompareDataFunc)_cmp_locks, NULL);
            g_free (lock);
        }
    }

    g_dir_close (dir);
    g_free (lock_dir);

    /* We iterate the sorted sequence of lock instances to find the first (lowest) number *not* taken. */
    for (iter = g_sequence_get_begin_iter(seq); !g_sequence_iter_is_end(iter); iter = g_sequence_iter_next(iter)) {
      current_lock_instance = GPOINTER_TO_INT(g_sequence_get(iter));
      if (lowest_lock_instance < current_lock_instance)
        break;
      else
        lowest_lock_instance = current_lock_instance + 1;
    }

    g_sequence_free(seq);

    return lowest_lock_instance;
}

static void termination_handler (G_GNUC_UNUSED gint signum) {
    DEBUG_FUNCTION ("termination_handler");
    gtk_main_quit ();
}

/*
 * This is to do the migration of config files from ~/.tilda to the
 * XDG_*_HOME folders
 */
static void migrate_config_files(char *old_config_path) {
    gchar* old_lock_dir = g_build_filename(old_config_path, "locks", NULL);
    gchar* new_lock_dir = g_build_filename(g_get_user_cache_dir (), "tilda", "locks", NULL);
    gchar* new_config_dir = g_build_filename(g_get_user_config_dir (), "tilda", NULL);

    if(!g_file_test(new_lock_dir, G_FILE_TEST_IS_DIR)) {
           g_mkdir_with_parents(new_lock_dir, 0700);
    }

    //we basically need to move the files from old_config_path to config and cache
    g_rename(old_lock_dir, new_lock_dir);
    //we must move the config files after we have moved the locks directory, otherwise it gets moved aswell
    g_rename(old_config_path, new_config_dir);
    g_free(old_lock_dir);
    g_free(new_lock_dir);
    g_free(new_config_dir);
}

static void load_application_css () {
    GtkCssProvider *provider;
    const gchar* style;
    GError *error;

    provider = gtk_css_provider_new ();

    gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                               GTK_STYLE_PROVIDER (provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);

    style = "#search{background:#fff;}";
    error = NULL;
    gtk_css_provider_load_from_data (provider, style, -1, &error);

    if (error) {
        g_print ("Error %s\n", error->message);
        g_error_free (error);
    }
}

static void load_custom_css_file () {
    GtkCssProvider *provider;
    GFile *file;
    GError *error;
    gchar *filename;

    filename = g_build_filename (g_get_user_config_dir (),
                                 "tilda", "style.css", NULL);

    if (!g_file_test (filename, G_FILE_TEST_EXISTS))
        return;

    g_print (_("Found style.css in the user config directory, "
               "applying user css style.\n"));

    provider = gtk_css_provider_new ();

    gtk_style_context_add_provider_for_screen (gdk_screen_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);


    file = g_file_new_for_path (filename);
    error = NULL;
    gtk_css_provider_load_from_file (provider, file, &error);

    if (error) {
        g_print ("Error: %s", error->message);
        g_error_free (error);
    }

    g_object_unref (file);
    g_free (filename);
}

int main (int argc, char *argv[])
{
    DEBUG_FUNCTION_MESSAGE ("main", "Using libvte version: %i.%i.%i\n",
                            VTE_MAJOR_VERSION, VTE_MINOR_VERSION, VTE_MICRO_VERSION);

    tilda_window tw;
    /* NULL set the tw pointers so we can get a clean exit on initialization failure */
    memset(&tw, 0, sizeof(tilda_window));

    struct sigaction sa;
    struct lock_info lock;
    gboolean need_wizard = FALSE;
    gchar *config_file, *lock_file, *old_config_path;

    /*
     * Migration code to move old files to new XDG folders
     */
    old_config_path = g_build_filename(g_get_home_dir (), ".tilda", NULL);
    if (g_file_test(old_config_path, G_FILE_TEST_IS_DIR)) {
        g_print (_("Migrating old config path to XDG folders\n"));
        migrate_config_files(old_config_path);
        g_free(old_config_path);
    }

    /* Remove stale lock files */
    remove_stale_lock_files ();

    /* The global lock file is used to synchronize the start up of multiple simultaneously starting tilda processes.
     * The processes will synchronize on a lock file named lock_0_0, such that the part of determining the instance
     * number and creating the per process lock file (lock_<pid>_<instance>) is atomic. Without this it could
     * happen, that a second tilda instance was trying to determine its instance number before the first instance
     * had finished creating its lock file, this resulted in two processes with the same instance number and could lead
     * to corruption of the config file.
     * In order to test if this works the following shell command can be used: "tilda & tilda && fg", which causes
     * two tilda processes to be started simultaneously. See:
     *
     *     http://stackoverflow.com/questions/3004811/how-do-you-run-multiple-programs-from-a-bash-script
     */
    struct lock_info global_lock;
    global_lock.instance = 0;
    global_lock.pid = 0;
    gchar *global_lock_file = NULL;

    global_lock_file = create_lock_file(&global_lock);
    if(global_lock_file == NULL) {
        perror("Error creating global lock file");
        return EXIT_FAILURE;
    }
    flock(global_lock.file_descriptor, LOCK_EX);

    /* Start of atomic section. */
    lock.pid = getpid ();
    lock.instance = get_instance_number ();
    lock_file = create_lock_file (&lock);

    /* End of atomic section */

    flock(global_lock.file_descriptor, LOCK_UN);
    g_remove (global_lock_file);
    close(lock.file_descriptor);
    g_free(global_lock_file);


#if ENABLE_NLS
    /* Gettext Initialization */
    char* locale = setlocale (LC_ALL, "");
    if(!locale) {
        g_warning ("Could not setup locale.");
    }
    bindtextdomain (PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain (PACKAGE);
#endif


#ifdef DEBUG
    /* Have to do this early. */
    if (getenv ("VTE_PROFILE_MEMORY"))
        if (atol (getenv ("VTE_PROFILE_MEMORY")) != 0)
            g_mem_set_vtable (glib_mem_profiler_table);
#endif

    config_file = NULL;

    /* Parse the command line */
    tilda_cli_options *cli_options = init_cli_options();
    need_wizard = parse_cli (argc, argv, cli_options, &config_file);

    if (config_file) {	  // if there was a config file specified via cli
        if (!g_file_test (config_file, G_FILE_TEST_EXISTS)) {
            g_printerr (_("Specified config file '%s' does not exist. Reverting to default path.\n"),
                    config_file);
            config_file = get_config_file_name (lock.instance);
        }
    } else {    // if not, we look for the defaut config file
        config_file = get_config_file_name (lock.instance);
    }

    /* Start up the configuration system and load from file */
    gint config_init_result = config_init (config_file);

    /* Set up possible overridden config options */
    setup_config_from_cli_options(cli_options);
    g_free(cli_options);

    /* We're about to startup X, so set the error handler. */
    XSetErrorHandler (xerror_handler);

    /* Initialize GTK. Any code that interacts with GTK (e.g. creating a widget) should come after this call. */
    gtk_init (&argc, &argv);

    /* This section shows a modal dialog to notify the user that something has gone wrong when loading the config.
     * Earlier version only used to print a message to stderr, but since tilda is usually not started from a
     * console this message would have been lost and a message dialog is much more user friedly.
     */
    GtkWidget *dialog = NULL;
    if(config_init_result == 1) {
        dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                        TILDA_CONFIG_PARSE_ERROR);
    } else if (config_init_result != 0) {
        dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                        TILDA_CONFIG_OTHER_ERROR);
    }

    if(dialog) {
        g_message("Running Dialog");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    load_application_css ();
    load_custom_css_file ();

    /* create new tilda_window */
    gboolean success = tilda_window_init (config_file, lock.instance, &tw);

    if(!success) {
        fprintf(stderr, "tilda.c: initialization failed\n");
        goto initialization_failed;
    }

    /* Adding widget title for CSS selection */
    gtk_widget_set_name (GTK_WIDGET(tw.window), "Main");

    /* Initialize and set up the keybinding to toggle tilda's visibility. */
    tomboy_keybinder_init ();

    /* Hook up signal handlers */
    sa.sa_handler = termination_handler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction (SIGINT,  &sa, NULL);
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGABRT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    sigaction (SIGKILL, &sa, NULL);

    /* If the config file doesn't exist open up the wizard */
    if (access (tw.config_file, R_OK) == -1)
    {
        /* We probably need a default key, too ... */
        gchar *default_key = g_strdup_printf ("F%d", tw.instance+1);
        config_setstr ("key", default_key);
        g_free (default_key);

        need_wizard = TRUE;
    }

    /* Show the wizard if we need to.
     *
     * Note that the key will be bound upon exiting the wizard */
    if (need_wizard) {
        g_print ("Starting the wizard to configure tilda options.");
        wizard (&tw);
    } else {
        gint ret = tilda_keygrabber_bind (config_getstr ("key"), &tw);

        if (!ret)
        {
            /* The key was unbindable, so we need to show the wizard */
            const char *message = _("The keybinding you chose for \"Pull Down Terminal\" is invalid. Please choose another.");

            tilda_keybinding_show_invalid_keybinding_dialog (NULL,
                                                             message);
            wizard (&tw);
        }
    }

    pull (&tw, config_getbool ("hidden") ? PULL_UP : PULL_DOWN, FALSE);

    g_print ("Tilda has started. Press %s to pull down the window.\n",
        config_getstr ("key"));
    /* Whew! We're finally all set up and ready to run GTK ... */
    gtk_main();

initialization_failed:
    tilda_window_free(&tw);

    /* Ok, we're at the end of our run. Time to clean up ... */
    config_free (config_file);
    g_remove (lock_file);
    close(lock.file_descriptor);
    g_free (lock_file);
    g_free (config_file);
    return 0;
}
