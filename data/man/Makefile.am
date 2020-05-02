data/man/tilda.1: $(srcdir)/data/man/tilda.1.in $(srcdir)/configure.ac $(srcdir)/data/man/Makefile.am
	$(AM_V_GEN) mkdir -p data/man && sed -e 's|\@VERSION\@|$(VERSION)|' $< > $@

man1_MANS += \
             data/man/tilda.1 \
             $(NULL)

EXTRA_DIST += data/man/tilda.1.in

CLEANFILES += data/man/tilda.1
