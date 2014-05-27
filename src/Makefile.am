# vim: set noexpandtab ts=8 sts=8 sw=8:

# Define a C macro LOCALEDIR indicating where catalogs 
# will be installed.
DEFS += -DLOCALEDIR=\"$(localedir)\" -DDATADIR=\"$(datadir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

bin_PROGRAMS += src/tilda

src_tilda_CPPFLAGS = \
	$(AM_CPPFLAGS) \
    -I$(top_builddir)/src \
    -DDATADIR='"$(datadir)"' \
    -DMODULEDIR='"$(moduledir)"' \
    -DLIBEXECDIR='"$(libexecdir)"'

# Keep the headers here so that make dist-bzip2 works
src_tilda_SOURCES = src/callback_func.h src/callback_func.c \
		src/configsys.h src/configsys.c \
		src/debug.h \
		src/eggaccelerators.h src/eggaccelerators.c \
		src/key_grabber.h src/key_grabber.c \
		src/tilda.h src/tilda.c \
		src/tilda_terminal.h src/tilda_terminal.c \
		src/tilda_window.h src/tilda_window.c \
		src/tomboykeybinder.h src/tomboykeybinder.c \
		src/wizard.h src/wizard.c \
		src/xerror.h src/xerror.c

src_tilda_CFLAGS = $(AM_CFLAGS) \
		$(GTK_CFLAGS) \
		$(LIBGLADE_CFLAGS) \
		$(VTE_CFLAGS) \
		$(LIBCONFUSE_CFLAGS)

src_tilda_LDFLAGS = $(AM_LDFLAGS) \
		$(GTK_LDFLAGS) \
		$(LIBGLADE_LDFLAGS) \
		$(VTE_LDFLAGS) \
		$(LIBCONFUSE_LDFLAGS) \
		-Wl,--as-needed

src_tilda_LDADD = $(AM_LDADD) \
		$(GTK_LIBS) \
		$(LIBGLADE_LIBS) \
		$(VTE_LIBS) \
		$(LIBCONFUSE_LIBS) \
		$(LTLIBINTL) \
		$(LEXLIB)
