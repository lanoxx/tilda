# Compiling and Installing

## Dependencies

 * Glib >= 2.30 - http://developer.gnome.org/glib/2.30/
 * Gtk+3 >= 3.0 - http://developer.gnome.org/gtk3/3.0/
 * VTE >= 2.91 - http://developer.gnome.org/vte/0.30/
 * libConfuse - http://www.nongnu.org/confuse/
 * libx11-dev - http://www.x.org/wiki/

On Ubuntu based system install dependencies with:

    sudo apt-get install git dh-autoreconf autotools-dev debhelper \
      libconfuse-dev libgtk-3-dev libpcre2-dev libvte-2.91-dev pkg-config

You possibly need other packages such as `gettext`, `automake`, 
`autoconf`, `autopoint`, and X11 development libraries.

On Fedora:

    # This may be outdated (see Ubuntu example above, for a more thorough list)
    sudo yum install git automake libconfuse-devel vte3-devel gtk3-devel \
      glib-devel gettext-devel

The dependencies section above is complete but the sample command may not be
complete, depending on your system you may need to install additional packages.
Please carefully read the output of the `autogen.sh` (see below) for more
information of what you need to install.

## Compiling

Generally if you have installed the development packages (e.g. `*-dev` or
`*-devel`) of the dependencies above and the autotools suite then it should
be possible to compile with:

    mkdir build
    cd build
    ../autogen.sh --prefix=/usr
    make --silent

Changing to `build/` and calling `autogen.sh` relative from there makes sure
that we perform an out-of-tree build and all generated files are stored inside
`build/`. This way build artifacts will not clutter the source directory.

If you get the following error message, then you are missing the `autopoint`
binary which is part of the autotools suite. On Ubuntu the `dh-autoreconf`
package installs it along with automake, autoconf and autoreconf.

    Can't exec "autopoint": No such file or directory at [\]
      /usr/share/autoconf/Autom4te/FileUtils.pm line 345.

You do not need the `--silent` option, but I prefer to use it to reduce the
output a bit. If you experience any problem during build, then drop the
`--silent` option.

## Installing

After you have compiled the package run the following command to install tilda
to the prefix that you have chosen before:

    sudo make install

If you don't want to install to the `/usr` prefix, choose some other prefix
when you run the `autogen.sh` script, such as `/opt/tilda` and add it to your
path.

# Packaging for Debian

This section explains how to package Tilda for Debian and Debian derived
distributions.

## Preparation before building the package

In order to build a package which can be uploaded to some Debian based
distribution the following steps are necessary. Replace '#' with the number of
the current minor and patch release version.

 1. Check out the latest stable branch (e.g. tilda-1-#)
   and add any changes or bugfixes which you want to include,
   then commit these changes. Tilda stable branches are named
   `tilda-<MAJOR>-<MINOR>` and all patch level releases for the same minor
   release go into that branch.
 2. Change into the `po/` folder, run `make update-po` and commit any changed
    `.po` files in the `po` folder. If this change is forgotten, then after
    running `make distcheck` below there may be uncommitted changed to the `po`
    folder in the source tree.
 3. Update the `Changelog` and commit it:

        git commit -m "Update the change log for 1.#.#"
 4. Update the version number in `configure.ac` and make
   a commit, the version number as commit message:

        git commit -m "1.#.#"
 5. Create a tarball from the build folder:

        cd build/
        make distcheck
    
    This will give you a tarball in the build folder named `tilda-1.#.#`, the
    tarball needs to be copied to the location where you are building the
    package and it needs to be renamed (or symlinked) as
    `tilda_1.#.#.orig.tar.gz`.

 6. Checkout the [packaging files for tilda][1] and update the change log at
    `debian/changelog` such that it contains an entry for the latest version
    of tilda. Note, the `debian/changelog` should not contain information about
    tilda specific changes but about changes related to the Debian packaging.

## Building a package

With the above `make distcheck` command you get a tarball from which a Debian
package can be build. A Debian package consists of a separate source and binary
package. The following steps document the basic commands that are required to
build both the source package and the binary package, to verify that both are
correct and to upload the source package to *mentors.debian.org*.

I am using **pbuilder** to build the source and binary packages.
Please refer to the man pages **pbuilder(8)** on howto 
setup the base image. I also use **pdebuild** as a convenient script to
run `debuild` inside the **pbuilder** environment (see **pdebuild(1)**).

The following process creates several files and packages in the folder from
where these commands are executed, its useful to perform these commands in a
separate folder such as `tilda-releases`:

 1. `mkdir tilda-releases; cd tilda-releases`.
 2. Copy the release tarball to the current location and extract it:
    `tar -xf tilda_1.2.#.orig.tar.gz`
 3. `cd tilda-1.#.#`
 4. Checkout the `tilda-debian` [repository][1] from Github and copy the
    `debian/` folder to `tilda-1.#.#/`.
 5. To build the source package you need to run **debuild**. You can use one of
    the following two methods to do this:
    * Run **debuild** inside a change root by using `pdebuild`:
    
          sudo pdebuild --use-pdebuild-internal \
            -- --basetgz ~/pbuilder/unstable-base.tgz
    * Run `debuild` directly from the current folder (e.g. from `tilda-1.#.#/`)
 6. If `debuild` finishes without a problem next run `pbuilder`, this will
    verify that the package is buildable (without warnings or errors) in a clean
    environment:

        sudo pbuilder --build --basetgz ~/pbuilder/unstable-base.tgz \
          tilda_1.#.#-1.dsc
 7. Optionally run `lintian`:
 
        lintian -I --show-overrides tilda_1.#.#-1_amd64.changes
 8. Run `debsign` to sign the package with your PGP key:
 
        debsign tilda_1.#.#-1_amd64.changes
 9. If `pbuilder` does not complain and you don not see any warnings in
    `lintian`, then upload the package to mentors:

        dput mentors tilda_1.#.#-1_amd64.changes

## Notes on pbuilder

The commands above assume an up to date pbuilder image for debian unstable
which can be created via:

    mkdir ~/pbuilder; cd ~/pbuilder
    sudo pbuilder create --basetgz unstable-base-test.tgz --distribution sid \
      --mirror http://deb.debian.org/debian

The `--mirror` option is necessary when creating the image on downstream
distributions such as Ubuntu. In such a case pbuilder would use the Ubuntu
mirror and will complain that the repository has no `unstable` release.

[1]: https://salsa.debian.org/debian/tilda/
