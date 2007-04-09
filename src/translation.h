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

#ifndef TRANSLATION_H
#define TRANSLATION_H

G_BEGIN_DECLS

#include <tilda-config.h>

/* Things for translations */
#include <gettext.h>

#if HAVE_LOCALE_H
	#include <locale.h>
#endif

#if ENABLE_NLS
	#define _(STR) gettext(STR)
	#define N_(STR) gettext_noop(STR)
#else
	#define _(STR) STR
	#define N_(STR) STR
#endif

G_END_DECLS

#endif /* TRANSLATION_H */
