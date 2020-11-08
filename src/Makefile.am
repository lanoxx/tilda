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

ENUM_TYPES = \
	$(srcdir)/src/tilda-search-box.h \
	$(NULL)

src/tilda-enum-types.h: $(srcdir)/src/tilda-enum-types.h.template $(ENUM_TYPES) $(GLIB_MKENUMS)
	$(AM_V_GEN) ($(GLIB_MKENUMS) --template $(srcdir)/src/tilda-enum-types.h.template $(ENUM_TYPES)) > $@

src/tilda-enum-types.c: $(srcdir)/src/tilda-enum-types.c.template $(ENUM_TYPES) $(GLIB_MKENUMS)
	$(AM_V_GEN) ($(GLIB_MKENUMS) --template $(srcdir)/src/tilda-enum-types.c.template $(ENUM_TYPES)) > $@

src_tilda_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_builddir)/src \
	-DG_LOG_DOMAIN='"tilda"' \
	-DG_LOG_USE_STRUCTURED \
	-DDATADIR='"$(datadir)"' \
	-DLOCALEDIR='"$(localedir)"' \
	-DPKGDATADIR='"$(pkgdatadir)"' \
	$(NULL)

BUILT_SOURCES = \
		src/glade-resources.h src/glade-resources.c \
		src/tilda-enum-types.c src/tilda-enum-types.h \
		$(NULL)

# Keep the headers here so that make dist-bzip2 works
src_tilda_SOURCES = \
		$(BUILT_SOURCES) \
		src/configsys.h src/configsys.c \
		src/debug.h \
		src/eggaccelerators.h src/eggaccelerators.c \
		src/key_grabber.h src/key_grabber.c \
		src/screen-size.h src/screen-size.c \
		src/tilda.h src/tilda.c \
		src/tilda-keybinding.c src/tilda-keybinding.h \
		src/tilda-cli-options.c src/tilda-cli-options.h \
		src/tilda-context-menu.c src/tilda-context-menu.h \
		src/tilda-match-registry.c src/tilda-match-registry.h \
		src/tilda-palettes.h src/tilda-palettes.c \
		src/tilda-regex.h \
		src/tilda-search-box.c src/tilda-search-box.h \
		src/tilda_terminal.h src/tilda_terminal.c \
		src/tilda_window.h src/tilda_window.c \
		src/tomboykeybinder.h src/tomboykeybinder.c \
		src/wizard.h src/wizard.c \
		src/vte-util.h \
		$(NULL)

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
		-lm \
		$(NULL)

EXTRA_DIST += \
		src/glade-resources.gresource.xml \
		src/tilda-search-box.ui \
		src/menu.ui \
		src/tilda.ui \
		src/tilda-enum-types.h.template \
		src/tilda-enum-types.c.template \
		$(NULL)

CLEANFILES += \
		src/glade-resources.h \
		src/glade-resources.c \
		src/tilda-enum-types.h \
		src/tilda-enum-types.c \
		$(NULL)
