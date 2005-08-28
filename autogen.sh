#!/bin/sh

# This was borrowed from Gaim (see gaim.sf.net) and modified
# for our purposes. Thanks guys!

echo "This will do all the autotools stuff so that you can build"
echo "tilda successfully."
echo
echo "When it is finished, take the usual steps to install:"
echo "./configure"
echo "make"
echo "make install"
echo

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed to compile tilda";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile tilda";
	echo;
	exit;
}

echo "Generating configuration files for tilda, please wait..."
echo;

aclocal $ACLOCAL_FLAGS || exit;
autoheader || exit;
automake --add-missing --copy;
autoconf || exit;
automake || exit;

