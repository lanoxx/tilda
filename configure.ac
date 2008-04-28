# Autoconf file for tilda
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([Tilda], [0.9.6], [http://bugs.launchpad.net/tilda])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/tilda.c])
AM_CONFIG_HEADER([tilda-config.h:tilda-config.h.in])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.16.1])

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

# Needed for libglade signal autoconnect
# LDFLAGS="$LDFLAGS -Wl,--export-dynamic"

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LEX

# Notice if there are no versions of lex installed
# and error out of the configure.
case $LEX in
	:)
		AC_MSG_ERROR([No lex program found])
		;;
esac

# Checks for libraries.
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.0.0])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0])
PKG_CHECK_MODULES([LIBGLADE], [libglade-2.0])
PKG_CHECK_MODULES([VTE], [vte])
PKG_CHECK_MODULES([LIBCONFUSE], [libconfuse])

# Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h string.h strings.h sys/ioctl.h unistd.h confuse.h])
AC_CHECK_HEADERS([libintl.h locale.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([mkdir strcasecmp strchr strncasecmp strstr strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 m4/Makefile
                 po/Makefile.in])
AC_OUTPUT
