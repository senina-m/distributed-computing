CFLAGS=--std=c17 -Wall -pedantic -Isrc/ -ggdb -Wextra -Werror -DDEBUG
BUILDDIR=build
SRCDIR=pa3
CC=clang

# export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/marsen/itmo/distributed_computing/lab3/pa3"

LD_PRELOAD=/home/marsen/itmo/distributed_computing/lab3/pa3/lib64/libruntime.so ./build/pa3 -p 2 10 20

main: build
	$(CC) -L./pa3/lib64/ -lruntime -g -std=c99 -Wall -pedantic $(SRCDIR)/main.c $(SRCDIR)/pipes.c $(SRCDIR)/ipc.c $(SRCDIR)/bank_robbery.c $(SRCDIR)/lamport.c -o $(BUILDDIR)/pa3

build: clean
	mkdir -p $(BUILDDIR)
	

all: main

clean:
	rm -rf $(BUILDDIR)
	rm -rf events.log pipes.log
