CFLAGS=--std=c17 -Wall -pedantic -Isrc/ -ggdb -Wextra -Werror -DDEBUG
BUILDDIR=build
SRCDIR=pa1
CC=gcc

main: build
	$(CC) $(SRCDIR)/main.c $(SRCDIR)/pipes.c $(SRCDIR)/ipc.c -o $(BUILDDIR)/runnable

build: clean
	mkdir -p $(BUILDDIR)
	

all: main

clean:
	rm -rf $(BUILDDIR)
	rm -rf events.log pipes.log
