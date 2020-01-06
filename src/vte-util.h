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

#ifndef VTE_UTIL_H
#define VTE_UTIL_H

#include <glib.h>
#include <vte/vte.h>

G_BEGIN_DECLS

#define VTE_CHECK_VERSION_RUMTIME(major,minor,micro) \
        (vte_get_major_version () > (major) || \
        (vte_get_major_version () == (major) && vte_get_minor_version () > (minor)) || \
        (vte_get_major_version () == (major) && vte_get_minor_version () == (minor) && vte_get_micro_version () >= (micro)))

G_END_DECLS

#endif
