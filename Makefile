SOURCES = $(sort $(wildcard *.c))

OBJECTS = $(SOURCES:.c=.o)

BINS = $(SOURCES:.c=)

CPPFLAGS = -g -I. -I/usr/include/libelf -I/usr/include/elfutils -DHAVE_CONFIG_H
LDFLAGS = -lelf -ldw -lebl

all: tests a

tests: main.c elf-print.c eblstrtab.c
	$(CC) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

a: a.c

clean:
	@rm -f tests a

