
bin_PROGRAMS = tilda

tilda: $(wildcard src/*.[ch])
	gcc -O2 -Wall -fno-strict-aliasing -pthread `pkg-config --cflags --libs gtk+-2.0 vte` -o tilda src/tilda.c
