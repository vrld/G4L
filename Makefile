CC=clang
INCLUDES=-I/usr/include/GL
CFLAGS=-fPIC --std=c99 -fomit-frame-pointer -Wall -Wextra -Wformat -pedantic -O0 -g $(INCLUDES)
LDFLAGS=-lc -lglut -lGLEW -lGL -lz -lpng -lturbojpeg

sources=$(wildcard src/*.c)
objects=$(sources:.c=.o)

.PHONY: clean all

all: G4L.so

G4L.so: $(objects)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(objects) $(target)
