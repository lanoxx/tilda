export bindir=/usr/bin

bin_PROGRAMS = tilda

tilda: src/tilda.c
	gcc `pkg-config --cflags --libs gtk+-2.0` -lpthread -lvte -otilda src/tilda.c
