CFLAGS=-fpic -shared -pedantic -Wall -g3
TARGET=libline.so
PREFIX:=/usr/local

all: libline.so ircify

ircify: LDLIBS=-Lline
ircify: ircify.c

$(TARGET): libline.o
	ld -shared -o $(TARGET) libline.o

clean:
	rm -f $(TARGET) ircify
	rm *.o

install: all
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	install $(TARGET) $(PREFIX)/lib/$(TARGET)
	install line.h $(PREFIX)/include/line.h
