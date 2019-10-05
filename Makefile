.PHONY = clean prepare
PROG=mini-SBC
CC=gcc
CFILES=$(wildcard src/*.c)


all: clean prepare $(PROG)

$(PROG): build_o
	$(CC) *.o -o bin/$(PROG) -g `pkg-config --cflags --libs libpjproject`
	rm *.o

build_o: $(CFILES)
	$(CC) $(CFILES) -c -I includes -g

clean:
	rm -rf bin

prepare:
	mkdir bin
