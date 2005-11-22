AUTOMAKE_OPTIONS = gnu

bin_PROGRAMS = tilda

tilda_headers = \
    tilda.h     \
	callback_func.h  \
	config.h         \
    tilda_terminal.h  \
    wizard.h \
	key_grabber.h   \
    tilda_window.h 


tilda_SOURCES = $(tilda_headers)	\
                tilda_window.c \
                tilda_terminal.c \
                wizard.c \
                callback_func.c \
                key_grabber.c \
                load_tilda.c  \
				tilda.c 

tilda_CFLAGS = @PACKAGE_CFLAGS@
tilda_LDADD = @PACKAGE_LIBS@

maintainer-clean-generic:
	-rm Makefile.in
