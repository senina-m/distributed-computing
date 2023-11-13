CFLAGS=--std=c17 -Wall -pedantic -Isrc/ -ggdb -Wextra -Werror -DDEBUG
BUILDDIR=build
SRCDIR=pa2
CC=gcc

main: build
	$(CC) -fsanitize=address $(SRCDIR)/main.c $(SRCDIR)/pipes.c $(SRCDIR)/ipc.c $(SRCDIR)/bank_robbery.c -o $(BUILDDIR)/runnable

build: clean
	mkdir -p $(BUILDDIR)
	

all: main

clean:
	rm -rf $(BUILDDIR)
	rm -rf events.log pipes.log
