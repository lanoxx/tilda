
bin_PROGRAMS = tilda

tilda: $(wildcard src/*.[ch])
	gcc -O2 -Wall -fno-strict-aliasing `pkg-config --cflags --libs gtk+-2.0 gthread-2.0 vte` -o tilda src/tilda.c
