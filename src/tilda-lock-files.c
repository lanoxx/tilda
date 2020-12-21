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

#include "tilda-lock-files.h"

#include "debug.h"

#include <fcntl.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

static gchar *create_lock_file (struct lock_info *lock);
static struct lock_info *islockfile (const gchar *filename);
static GSList *getPids();
static gint remove_stale_lock_files (void);
static gint get_instance_number (void);

gboolean
tilda_lock_files_obtain_instance_lock (struct lock_info * lock_info)
{
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
        return FALSE;
    }
    flock(global_lock.file_descriptor, LOCK_EX);

    /* Start of atomic section. */
    lock_info->pid = getpid ();
    lock_info->instance = get_instance_number ();
    lock_info->lock_file = create_lock_file (lock_info);
    /* End of atomic section */

    flock(global_lock.file_descriptor, LOCK_UN);
    remove (global_lock_file);
    close(lock_info->file_descriptor);

    g_free(global_lock_file);

    return TRUE;
}

void tilda_lock_files_free (struct lock_info * lock_info)
{
    remove (lock_info->lock_file);
    g_free (lock_info->lock_file);
    lock_info->lock_file = NULL;
}

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
        ret = open(lock_file_full, O_CREAT, S_IRUSR | S_IWUSR);
    } else {
        ret = open(lock_file_full, O_WRONLY | O_CREAT | O_EXCL, 0);
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
static gint remove_stale_lock_files (void)
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

    /* if the lock dir does not exist then there are no stale lock files to remove. */
    if (!g_file_test (lock_dir, G_FILE_TEST_EXISTS)) {
        g_free (lock_dir);
        return 0;
    }

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
            remove (remove_file);
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

static gint _cmp_locks(gconstpointer a, gconstpointer b, gpointer userdata) {
    return GPOINTER_TO_INT (a) - GPOINTER_TO_INT (b);
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
            g_sequence_insert_sorted(seq, GINT_TO_POINTER(lock->instance), _cmp_locks, NULL);
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
