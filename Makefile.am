export bindir=/usr/bin

bin_PROGRAMS = tilda

tilda: src/tilda.c
	gcc `pkg-config --cflags --libs gtk+-2.0` -lpthread -lvte -otilda src/key_bindings.c src/wizard.c src/tilda.c
