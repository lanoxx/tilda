## Process this file with automake to produce a Makefile.in
# vim: set noexpandtab ts=8 sts=8 sw=8:

AUTOMAKE_OPTIONS = gnu

SUBDIRS = po m4 src

CLEANFILES = tilda.desktop

MAINTAINERCLEANFILES = \
		ABOUT-NLS \
		Makefile.in \
		aclocal.m4 \
		stamp-h1 \
		missing \
		install-sh \
		decomp \
		compile \
		INSTALL \
		m4/*.m4 \
		tilda-config.h \
		config.guess \
		config.rpath \
		config.sub \
		configure \
		depcomp \
		m4/Makefile.in \
		po/Makefile.in.in \
		po/Makevars.template \
		po/Rules-quot \
		po/boldquot.sed \
		po/en@boldquot.header \
		po/en@quot.header \
		po/insert-header.sin \
		po/quot.sed \
		po/remove-potcdate.sin \
		po/tilda.pot \
		tilda-config.h.in \
		tilda-config.h.in~

Applicationsdir = ${datadir}/applications
Applications_DATA = tilda.desktop

Pixmapsdir = ${datadir}/pixmaps
Pixmaps_DATA = tilda.png

data_DATA = tilda.glade

EXTRA_DIST = tilda.desktop.in tilda.png tilda.glade

tilda.desktop: tilda.desktop.in
	sed -e 's|\@BINDIR\@|$(bindir)|' \
		-e 's|\@PIXMAPSDIR\@|$(datadir)/applications|' $< > $@

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
