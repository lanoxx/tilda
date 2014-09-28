# Compiling and Installing

## Dependencies

 * Glib >= 2.30 - http://developer.gnome.org/glib/2.30/
 * Gtk+3 >= 3.0 - http://developer.gnome.org/gtk3/3.0/
 * VTE >= 2.90 - http://developer.gnome.org/vte/0.30/
 * libConfuse - http://www.nongnu.org/confuse/

On Ubuntu based system install dependencies with:

    sudo apt-get install git automake libgtk-3-dev libvte-2.90-dev libconfuse-dev

You possibly need other packages such as `gettext`, `automake`, `autoconf`, `autopoint`, and X11 development libraries.

On Fedora:

    sudo yum install git automake libconfuse-devel vte3-devel gtk3-devel glib-devel gettext-devel

The dependencies section is complete, depending on your system you my need ot install additional packages. Please
carefully read the output of the `autogen.sh` (see below) for more information of what you need to install.

## Compiling

Generally if you have installed the development packages (*-dev or *-devel) of
the dependencies above then it should be possible to compile with:

    ./autogen.sh --prefix=/usr
    make --silent

I recommend to run make with the `--silent` (or `-s`) option.

## Installing

After you have compiled the package run the following command to install tilda to the prefix that you have chosen
before:

    sudo make install

If you don't want to install to the `/usr` prefix, choose some other prefix when you run the `autogen.sh` script,
such as `/opt/tilda` and add it to your path.

# Packaging for Debian

This section explains how to package Tilda for Debian and Debian derived distributions.

## Preparation before building the package

In order to build a package which can be uploaded to some
Debian based distribution the following steps are necessary.
Replace '#' with the number of the current minor release.

 1. Check out the latest stable branch (e.g. tilda-1-1)
   and commit any changes or patches which you want to include,
   then commit these changes.
 2. Update the changelog with a message such as:
       "Update the change log for 1.1.#"
 3. Update the version number in configure.ac and make
   a commit with the version number:
       "1.1.#"
 4. Rebase the tilda-debian-1-1 ontop of the latest stable HEAD.
   This is not very nice currently as this branch needs to be
   rebased every time a new release is made. I am still looking
   for a better solution.
 5. Change debian/changelog and update the Debian specific changelog.
 6. Create a tarball using git-archive:
        git archive --prefix=tilda-1.1.#/ -o ../tilda_1.1.#.orig.tar.gz HEAD

# Building a package

You now have a tarball which can be build into a package. The following steps
are just to document the basic commands that are required to build the package,
then verify its correct and upload the package to mentors. However the steps which
are required to setup such a development environment are not explained here:

 7. cd ..; tar -xf tilda_1.1.#.orig.tar.gz
 8. cd tilda-1.1.#
 9. Run debuild
10. If debuild finishes without a problem next run pbuilder, this will verify that
    the packge is buildable (without warnings or errors) in a clean environment:
        sudo pbuilder --build --basetgz ~/pbuilder/unstable-base.tgz tilda_1.1.8-1.dsc
11. If pbuilder does not complain and you don't see any warnings in lintian, then
    upload the package to mentors:
        dput mentors tilda_1.1.#-1_amd64.changes
