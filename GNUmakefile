CFLAGS=-fpic -shared -pedantic -Wall -g3
TARGET=libidc.so
PREFIX:=/usr/local

all: libidc.so ircify

ircify: LDLIBS=-lidc -L.
ircify: CFLAGS=-g3 -Wall -pedantic
ircify: ircify.c

$(TARGET): libidc.o
	ld -shared -o $(TARGET) libidc.o

clean:
	rm -f $(TARGET) ircify
	rm *.o

install: all
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	install $(TARGET) $(PREFIX)/lib/$(TARGET)
	install idc.h $(PREFIX)/include/idc.h
