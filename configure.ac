# Autoconf file for tilda
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([tilda], [0.09.4], [sloutri@iit.edu])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/tilda.c])
AM_CONFIG_HEADER([tilda-config.h:tilda-config.h.in])

# Add optional debugging features
AC_ARG_ENABLE([maintainer-flags],
		[AC_HELP_STRING([--enable-maintainer-flags], [enable extra debugging (don't use as user)])])

if test "x$enable_maintainer_flags" = "xyes"; then
	enable_debug="yes"
	enable_debug_functions="yes"
	CFLAGS="$CFLAGS -Wall -Wextra -pedantic -fno-common -Wstrict-aliasing=2"
	CFLAGS="$CFLAGS -Wshadow -Wmissing-prototypes -Wmissing-declarations"
fi

AC_ARG_ENABLE([debug], [AC_HELP_STRING([--enable-debug], [enable extra debugging output])])

if test "x$enable_debug" = "xyes"; then
	AC_DEFINE([DEBUG], [1], [Enable extra debugging output])
	CFLAGS="$CFLAGS -g -ggdb"
fi

AC_ARG_ENABLE([debug-functions], [AC_HELP_STRING([--enable-debug-functions], [enable function call tracing])])

if test "x$enable_debug_functions" = "xyes"; then
	AC_DEFINE([DEBUG_FUNCTIONS], [1], [Enable function call tracing])
fi

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.0.0])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0])
PKG_CHECK_MODULES([VTE], [vte])
PKG_CHECK_MODULES([LIBCONFUSE], [libconfuse])

dnl uncomment following lines to add gettext support
dnl GETTEXT_PACKAGE=tilda
dnl AC_SUBST(GETTEXT_PACKAGE)
dnl AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

dnl Add the languages which your application supports here.
dnl ALL_LINGUAS=""
dnl AM_GLIB_GNU_GETTEXT

# Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h string.h strings.h sys/ioctl.h unistd.h confuse.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([mkdir strcasecmp strchr strncasecmp strstr strtol])

LIBS="$LIBS"

# Figure out the prefix we are installing in, so we can install icons in the
# correct place. There is probably an easier way to do this, but this is almost
# exactly what is recommended in the GNU Autoconf Standards.
# See: http://www.gnu.org/software/autoconf/manual/autoconf-2.60/html_node/Defining-Directories.html
CPPFLAGS="$CPPFLAGS -DDATADIR='\"$datadir\"'"

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
