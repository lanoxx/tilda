
bin_PROGRAMS = tilda

tilda: $(wildcard src/*.[ch])
	gcc -g -Wall -pthread `pkg-config --cflags --libs gtk+-2.0 vte` -otilda src/key_bindings.c src/wizard.c src/tilda.c
