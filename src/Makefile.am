## Process this file with automake to produce Makefile.in
# vim: set ts=8 sts=8 sw=8:

AUTOMAKE_OPTIONS = gnu

# Define a C macro LOCALEDIR indicating where catalogs 
# will be installed.
#localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" -DDATADIR=\"$(datadir)\" @DEFS@

bin_PROGRAMS = tilda

tilda_SOURCES = tilda_window.c \
		tilda_terminal.c \
		wizard.c \
		callback_func.c \
		key_grabber.c \
		load_tilda.c  \
		tilda.c \
		xerror.c \
		configsys.c \
		tomboykeybinder.c \
		eggaccelerators.c \
		key_converter.l

tilda_CFLAGS = $(AM_CFLAGS) \
		$(GTK_CFLAGS) \
		$(GTHREAD_CFLAGS) \
		$(LIBGLADE_CFLAGS) \
		$(VTE_CFLAGS) \
		$(LIBCONFUSE_CFLAGS)

tilda_LDFLAGS = $(AM_LDFLAGS) \
		$(GTK_LDFLAGS) \
		$(GTHREAD_LDFLAGS) \
		$(LIBGLADE_LDFLAGS) \
		$(VTE_LDFLAGS) \
		$(LIBCONFUSE_LDFLAGS)

tilda_LDADD = $(AM_LDADD) \
		$(GTK_LIBS) \
		$(GTHREAD_LIBS) \
		$(LIBGLADE_LIBS) \
		$(VTE_LIBS) \
		$(LIBCONFUSE_LIBS) \
		$(LTLIBINTL) \
		$(LEXLIB)

maintainer-clean-generic:
	-rm Makefile.in

