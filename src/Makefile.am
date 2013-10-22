## Process this file with automake to produce Makefile.in
# vim: set noexpandtab ts=8 sts=8 sw=8:

AUTOMAKE_OPTIONS = gnu

# Define a C macro LOCALEDIR indicating where catalogs 
# will be installed.
DEFS += -DLOCALEDIR=\"$(localedir)\" -DDATADIR=\"$(datadir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

bin_PROGRAMS = tilda

# Keep the headers here so that make dist-bzip2 works
tilda_SOURCES = callback_func.h callback_func.c \
		configsys.h configsys.c \
		debug.h \
		eggaccelerators.h eggaccelerators.c \
		key_grabber.h key_grabber.c \
		tilda.h tilda.c \
		tilda_terminal.h tilda_terminal.c \
		tilda_window.h tilda_window.c \
		tomboykeybinder.h tomboykeybinder.c \
		wizard.h wizard.c \
		xerror.h xerror.c

tilda_CFLAGS = $(AM_CFLAGS) \
		$(GTK_CFLAGS) \
		$(LIBGLADE_CFLAGS) \
		$(VTE_CFLAGS) \
		$(LIBCONFUSE_CFLAGS)

tilda_LDFLAGS = $(AM_LDFLAGS) \
		$(GTK_LDFLAGS) \
		$(LIBGLADE_LDFLAGS) \
		$(VTE_LDFLAGS) \
		$(LIBCONFUSE_LDFLAGS) \
		-Wl,--as-needed

tilda_LDADD = $(AM_LDADD) \
		$(GTK_LIBS) \
		$(LIBGLADE_LIBS) \
		$(VTE_LIBS) \
		$(LIBCONFUSE_LIBS) \
		$(LTLIBINTL) \
		$(LEXLIB)

MAINTAINERCLEANFILES = Makefile.in

