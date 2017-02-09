#!/bin/sh

echo "This script will generate the initial build system files to compile"
echo "tilda successfully. When done it will call the ./configure script"
echo "and pass on any options which you passed to this script"
echo "See ./configure --help to know which options are available"
echo
echo "When it is finished, take the usual steps to install:"
echo "make"
echo "make install"
echo

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed to compile tilda";
	echo;
	exit 1;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile tilda";
	echo;
	exit 1;
}

(autopoint --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autopoint installed to compile tilda";
	echo;
	exit 1;
}

test -n "$srcdir" || srcdir=$(dirname "$0")
test -n "$srcdir" || srcdir=.

olddir=$(pwd)

cd $srcdir

(test -f configure.ac) || {
        echo "*** ERROR: Directory '$srcdir' does not look like the top-level project directory ***"
        exit 1
}

# shellcheck disable=SC2016
PKG_NAME=$(autoconf --trace 'AC_INIT:$1' configure.ac)

if [ "$#" = 0 -a "x$NOCONFIGURE" = "x" ]; then
        echo "*** INFO: 'configure' will be run with no arguments." >&2
        echo "*** If you wish to pass any to it, please specify them on the" >&2
        echo "*** '$0' command line." >&2
        echo "" >&2
fi

echo "Generating build system configuration for tilda, please wait..."
echo;

# Autoreconf will call run autopoint, aclocal, autoconf, autoheader and automake
# to setup and configure the build environment
autoreconf --verbose --install --symlink --force || {
    echo;
    echo "autoreconf has encountered an error.";
    echo;
    exit 1;
}

# Next we invoke the configure script. "$@" contains the arguments that
# were passed to this script and we used it to forward them to configure.

cd "$olddir"
if [ "$NOCONFIGURE" = "" ]; then
        echo "Running configure..."
        $srcdir/configure "$@" || exit 1

        if [ "$1" = "--help" ]; then exit 0 else
                echo "Now type 'make' to compile $PKG_NAME" || exit 1
        fi
else
        echo "Skipping configure process."
fi
