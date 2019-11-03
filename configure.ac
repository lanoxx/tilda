dnl ****************************************************************************
dnl Tilda autoconf configuration file.
dnl Use autogen.sh to bootstrap the build system.
dnl ****************************************************************************

m4_define ([tilda_issues],
           [https://github.com/lanoxx/tilda/issues])

m4_define ([tilda_repository],
           [https://github.com/lanoxx/tilda])

AC_PREREQ([2.68])

AC_INIT([Tilda],
        [1.6-alpha],
        [tilda_issues],
        [tilda],
        [tilda_repository])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/tilda.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])

# We are going to use silent builds which have a much
# nicer output. More information can be found here:
#     https://wiki.gnome.org/Initiatives/GnomeGoals/NicerBuilds
AM_SILENT_RULES([yes])

# See http://www.gnu.org/savannah-checkouts/gnu/automake/manual/html_node/maintainer_002dmode.html
# for an explanation of using this macro. The short explanation is:
#     AM_MAINTAINER_MODE is bad but
#     AM_MAINTAINER_MODE([enable]) is good
AM_MAINTAINER_MODE([enable])

# This registers the option '--enable-maintainer-flags' to the ./configure script
# If this option is set then it activates both the '--enable-debug' and '--enable-debug-functions'
# options that are defined below. It also activates several flags to the compile such that it
# will show more warnings and errors to indicate problems in the source code.
AC_ARG_ENABLE([maintainer-flags],
	[AS_HELP_STRING([--enable-maintainer-flags],[\
            Enables extra debugging; use this option if you want to develop for tilda,
            but to not use it as an end user if you just want to compile and use tilda. \
            This option includes the --enable-debug and --enable-debug-functions \
            options. And sets several CFLAGS to enable more gcc warnings and errors.])])

if test "x$enable_maintainer_flags" = "xyes"; then
	enable_debug="yes"
	enable_debug_functions="yes"
	CFLAGS="$CFLAGS \
            -Wall \
            -Wextra \
            -fno-common \
            -Wstrict-aliasing=2 \
            -Wshadow \
            -Wmissing-prototypes \
            -Wmissing-declarations \
            -Wcast-align \
            -Wuninitialized \
            -Wno-strict-aliasing \
            -Werror=pointer-arith \
            -Werror=missing-declarations \
            -Werror=redundant-decls \
            -Werror=empty-body \
            -Werror=format \
            -Werror=format-security \
            -Werror=format-nonliteral \
            -Werror=init-self \
            -Werror=vla \
            -Wno-unused-parameter \
           "
fi

# This registers the option '--enable-debug' in the ./configure script
# If this option is set, then the the options -g and -ggdb will be
# passed to the compiler.
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],[Enables extra debugging output and \
                                      sets debug flags for gdb])])

if test "x$enable_debug" = "xyes"; then
	AC_DEFINE([DEBUG], [1], [Enable extra debugging output])
	CFLAGS="$CFLAGS -g -ggdb"
fi

# This registers the option '--debug-functions' in the ./configure script
# If this option is set, then the name of each entered function will be
# printed on the shell.
AC_ARG_ENABLE([debug-functions],
	[AS_HELP_STRING([--enable-debug-functions],[Enables function call tracing. \
							                    When this option is enabled tilda will print \
												the name of each function that is called to the \
												console.])])

if test "x$enable_debug_functions" = "xyes"; then
	AC_DEFINE([DEBUG_FUNCTIONS], [1], [Enable function call tracing])
fi

AC_ARG_ENABLE([clang],
    [AS_HELP_STRING([--enable-clang],[use clang instead of gcc as C compiler.])])

#Use C99 compilation mode
if test "x$enable_clang" = "xyes"; then
    # clang uses c99 mode by default, so we just set CC to clang and we are done
    CC="clang";
else
    # We do not need to set CC as the default is gcc, but we need to set it to
    # use C99 compilation mode
    CFLAGS="$CFLAGS -std=c99";
fi

AC_ARG_ENABLE([lto],
    [AS_HELP_STRING([--enable-lto],[Optimize at link time. This enables the compiler \
                                    to do a better job at optimization and (hopefully) \
                                    produce smaller binaries.])])

if test "x$enable_lto" = "xyes"; then
    CFLAGS="$CFLAGS -flto"
    if test "$CC" != "clang"; then
        # If the user has enabled lto explicitly, we assume he has made sure
        # that his toolchain can indeed handle lto objects.
        CFLAGS="$CFLAGS -fno-fat-lto-objects"
    fi
    # In the case of lto optimizations, we need to pass the optimization options
    # to the linker as well as telling it to use the linker plugin.
    LDFLAGS="$LDFLAGS $CFLAGS -fuse-linker-plugin"
fi



AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums, no)
if test x$GLIB_MKENUMS = xno; then
  AC_MSG_ERROR(Could not find a glib-mkenums in your PATH)
fi

AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources, no)
if test x$GLIB_COMPILE_RESOURCES = xno; then
  AC_MSG_ERROR(Could not find a glib-compile-resources in your PATH)
fi

AC_PATH_PROG(GDBUS_CODEGEN, gdbus-codegen, no)
if test x$GDBUS_CODEGEN = xno; then
  AC_MSG_ERROR(Could not find a glib-codegen binary in your PATH)
fi

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# This will initialize the internationalization
# capabilities of glib (glib/gi18n.h) and gettext
AM_GNU_GETTEXT_VERSION([0.18.1])
AM_GNU_GETTEXT([external])
# Checks for libraries.
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.10.0])
PKG_CHECK_MODULES([VTE], [vte-2.91 >= 0.40])
PKG_CHECK_MODULES([LIBCONFUSE], [libconfuse])
PKG_CHECK_MODULES([X11], [x11])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h string.h strings.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([mkdir strcasecmp strchr strncasecmp strstr strtol])

AC_CONFIG_FILES([Makefile
                 po/Makefile.in])
AC_OUTPUT

dnl ---------------------------------------------------------------------------
dnl - Show summary
dnl ---------------------------------------------------------------------------

dnl The following 'for' block splits the CFLAGS variable into multiple lines of 4 options per line
cflaglines=""
cflagcount=0
for flag in ${CFLAGS}; do
    cflaglines=$cflaglines$flag" "
    cflagcount=$(($cflagcount+1))
    if (( $cflagcount % 4 == 0)); then
       dnl Here we concatenate the current cflaglines variable with a newline and enough spaces
       dnl such that the lines are correctly indented. Do not try to indent or reformat the
       dnl position of the double quotes (\").
       cflaglines=$cflaglines"
                                       "
    fi
done;

echo "
              ${PACKAGE} $VERSION
              `echo ${PACKAGE} $VERSION | sed "s/./=/g"`

        prefix:                        ${prefix}
        datarootdir:                   ${datarootdir}
        datadir:                       ${datadir}
        pkgdatadir:                    `if test x${pkgdatadir} != "x"; then echo ${pkgdatadir}; else echo ${datadir}/${PACKAGE}; fi`
        source code location:          ${srcdir}
        compiler:                      ${CC}
        cflags:                        ${cflaglines}
        Maintainer mode:               ${USE_MAINTAINER_MODE}
        VTE:                           ${vte_package}
        Use *_DISABLE_DEPRECATED:      ${enable_deprecation_flags}
"
