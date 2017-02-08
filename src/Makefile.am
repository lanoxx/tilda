# vim: set noexpandtab ts=8 sts=8 sw=8:

NULL =

bin_PROGRAMS += src/tilda

# Rules to compile resources
src/glade-resources.h: src/glade-resources.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --generate $< --target=$@ --sourcedir=$(srcdir)/src
src/glade-resources.c: src/glade-resources.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --generate $< --target=$@ --sourcedir=$(srcdir)/src

# Dependencies rule, and do not forget to do a make after
# editing the UI with glade!
src/glade-resources.c src/glade-resources.h: \
    src/glade-resources.gresource.xml \
    $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies $(srcdir)/src/glade-resources.gresource.xml --sourcedir=$(srcdir)/src)

src_tilda_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_builddir)/src \
	-DDATADIR='"$(datadir)"' \
	-DLOCALEDIR='"$(localedir)"' \
	-DPKGDATADIR='"$(pkgdatadir)"' \
	$(NULL)

# Keep the headers here so that make dist-bzip2 works
src_tilda_SOURCES = src/callback_func.h src/callback_func.c \
		src/configsys.h src/configsys.c \
		src/debug.h \
		src/eggaccelerators.h src/eggaccelerators.c \
		src/key_grabber.h src/key_grabber.c \
		src/tilda.h src/tilda.c \
		src/tilda-keybinding.c src/tilda-keybinding.h \
		src/tilda_terminal.h src/tilda_terminal.c \
		src/tilda_window.h src/tilda_window.c \
		src/tomboykeybinder.h src/tomboykeybinder.c \
		src/wizard.h src/wizard.c \
		src/xerror.h src/xerror.c \
		src/glade-resources.h src/glade-resources.c \
		src/glade-resources.gresource.xml

src_tilda_CFLAGS = \
		$(AM_CFLAGS) \
		$(GTK_CFLAGS) \
		$(LIBGLADE_CFLAGS) \
		$(VTE_CFLAGS) \
		$(LIBCONFUSE_CFLAGS) \
		$(X11_CFLAGS) \
		$(NULL)

src_tilda_LDFLAGS = \
		$(AM_LDFLAGS) \
		$(GTK_LDFLAGS) \
		$(LIBGLADE_LDFLAGS) \
		$(VTE_LDFLAGS) \
		$(LIBCONFUSE_LDFLAGS) \
		$(X11_LDFLAGS) \
		-Wl,--as-needed \
		$(NULL)

src_tilda_LDADD = \
		$(AM_LDADD) \
		$(GTK_LIBS) \
		$(LIBGLADE_LIBS) \
		$(VTE_LIBS) \
		$(LIBCONFUSE_LIBS) \
		$(X11_LIBS) \
		$(LTLIBINTL) \
		$(LEXLIB) \
		$(NULL)

EXTRA_DIST += \
		src/glade-resources.gresource.xml \
		src/tilda.ui \
		$(NULL)

CLEANFILES += src/glade-resources.h src/glade-resources.c
