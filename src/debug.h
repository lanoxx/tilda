/* vim: set ts=4 sts=4 sw=4 expandtab textwidth=92: */
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

#ifndef DEBUG_H
#define DEBUG_H

#include <tilda-config.h>

#include <glib.h>
#include <stdio.h>

/* Debug Macros
 *
 * Add -DDEBUG to your compile options to turn on assert()s.
 *
 * If you do not have the DEBUG symbol defined, then asserts will
 * be turned off and compiled into a noop.
 */

#if DEBUG

    /* Enable asserts */
    #undef NDEBUG
    #include <assert.h>

    #define DEBUG_ASSERT assert

    #define DEBUG_ERROR(ERRMSG) g_printerr("*** DEVELOPER ERROR *** at %s:%d" \
                                         " -> %s\n", __FILE__, __LINE__, (ERRMSG))

#else

    /* Disable asserts */
    #define NDEBUG
    #include <assert.h>

    #define DEBUG_ASSERT assert

    #define DEBUG_ERROR(ERRMSG) {}

#endif

/* Function tracing
 *
 * Add -DDEBUG_FUNCTIONS to your compile options to turn on function tracing.
 *
 * If you do not have the DEBUG_FUNCTIONS symbol defined, then function tracing will
 * be turned off and compiled into a noop.
 */

#if DEBUG_FUNCTIONS

    #define DEBUG_FUNCTION(NAME) g_printerr("FUNCTION ENTERED: %s\n", (NAME))

#else

    #define DEBUG_FUNCTION(NAME) {}

#endif

/* A macro that calls perror() with a consistent header string */
#define TILDA_PERROR() perror("tilda")


#endif /* DEBUG_H */

