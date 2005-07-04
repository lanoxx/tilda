AUTOMAKE_OPTIONS = gnu

LDFLAGS = -pthread -lgthread-2.0 -lvte -lz -lncurses -lSM -lICE -lXft -lX11 -lfreetype -lXrender -lgtk-x11-2.0 -lfontconfig -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangoxft-1.0 -lpangox-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0
CFLAGS = -DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API -pthread -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/pango-1.0 -I/usr/include/freetype2 -I/usr/include/freetype2/config -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include

bin_PROGRAMS = tilda

tilda_headers = \
    tilda.h     \
	callback_func.h  \
    readconf.h  \
    vte.h \
	config.h         \
    tilda_terminal.h  \
    wizard.h \
	key_grabber.h   \
    tilda_window.h 


tilda_SOURCES = $(tilda_headers)	\
                readconf.c \
                tilda_window.c \
                tilda_terminal.c \
                wizard.c \
                callback_func.c \
                key_grabber.c \
                load_tilda.c  \
				tilda.c 

