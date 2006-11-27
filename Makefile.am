AUTOMAKE_OPTIONS = gnu

SUBDIRS = src

MAINTAINER_CLEAN_FILES =  \
	Makefile \
	src/Makefile \
	config.log \
	config.status \
	stamp-h1 \
	tilda-config.h

maintainer-clean-generic:
	-rm $(MAINTAINER_CLEAN_FILES)

Applicationsdir = @prefix@/share/applications
Applications_DATA = tilda.desktop

Pixmapsdir = @prefix@/share/pixmaps
Pixmaps_DATA = tilda.png
