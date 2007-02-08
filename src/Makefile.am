## Process this file with automake to produce Makefile.in
# vim: set ts=8 sts=8 sw=8:

AUTOMAKE_OPTIONS = gnu

bin_PROGRAMS = tilda

tilda_headers = tilda.h \
		callback_func.h \
		tilda_terminal.h \
		wizard.h \
		key_grabber.h \
		tilda_window.h \
		xerror.h


tilda_SOURCES = $(tilda_headers) \
		tilda_window.c \
		tilda_terminal.c \
		wizard.c \
		callback_func.c \
		key_grabber.c \
		load_tilda.c  \
		tilda.c \
		xerror.c

tilda_CFLAGS = $(AM_CFLAGS) \
		$(GTK_CFLAGS) \
		$(GTHREAD_CFLAGS) \
		$(VTE_CFLAGS) \
		$(LIBCONFUSE_CFLAGS)

tilda_LDFLAGS = $(AM_LDFLAGS) \
		$(GTK_LDFLAGS) \
		$(GTHREAD_LDFLAGS) \
		$(VTE_LDFLAGS) \
		$(LIBCONFUSE_LDFLAGS)

tilda_LDADD = $(AM_LDADD) \
		$(GTK_LIBS) \
		$(GTHREAD_LIBS) \
		$(VTE_LIBS) \
		$(LIBCONFUSE_LIBS)

maintainer-clean-generic:
	-rm Makefile.in

