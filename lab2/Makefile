CFLAGS=--std=c17 -Wall -pedantic -Isrc/ -ggdb -Wextra -Werror -DDEBUG
BUILDDIR=build
SRCDIR=pa2
CC=clang

# export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/marsen/itmo/distributed_computing/lab2/pa2"

LD_PRELOAD=/home/marsen/itmo/distributed_computing/lab2/pa2/lib64/libruntime.so ./build/pa2 -p 2 10 20

main: build
	$(CC) -L./pa2/lib64/ -lruntime -std=c99 -Wall -pedantic $(SRCDIR)/main.c $(SRCDIR)/pipes.c $(SRCDIR)/ipc.c $(SRCDIR)/bank_robbery.c -o $(BUILDDIR)/pa2

build: clean
	mkdir -p $(BUILDDIR)
	

all: main

clean:
	rm -rf $(BUILDDIR)
	rm -rf events.log pipes.log
