export bindir=/usr/bin

bin_PROGRAMS = tilde

tilde: src/tilde.c
	gcc `pkg-config --cflags --libs gtk+-2.0` -lpthread -lvte -otilde src/tilde.c

