## Process this file with automake to produce to Makefile.in
# vim: set ts=8 sts=8 sw=8:

AUTOMAKE_OPTIONS = gnu

SUBDIRS = src

MAINTAINER_CLEAN_FILES = Makefile \
		src/Makefile \
		config.log \
		config.status \
		stamp-h1 \
		tilda-config.h

maintainer-clean-generic:
	-rm $(MAINTAINER_CLEAN_FILES)

Applicationsdir = ${datadir}/applications
Applications_DATA = tilda.desktop

Pixmapsdir = ${datadir}/pixmaps
Pixmaps_DATA = tilda.png
