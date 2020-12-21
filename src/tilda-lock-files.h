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

#ifndef TILDA_LOCK_FILES_H
#define TILDA_LOCK_FILES_H

#include "glib.h"

struct lock_info
{
    gint pid;
    gint instance;
    gint file_descriptor;
    char * lock_file;
};

gboolean tilda_lock_files_obtain_instance_lock (struct lock_info * lock_info);

void tilda_lock_files_free (struct lock_info * lock_info);

#endif
