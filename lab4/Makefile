CFLAGS=--std=c17 -Wall -pedantic -Isrc/ -ggdb -Wextra -Werror -DDEBUG
BUILDDIR=build
SRCDIR=pa4
CC=clang

# export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/marsen/itmo/distributed_computing/lab3/pa4"

LD_PRELOAD=/home/marsen/itmo/distributed_computing/lab3/pa4/lib64/libruntime.so ./build/pa4 -p 2 10 20

main: build
	$(CC) -L./pa4/lib64/ -lruntime -std=c99 -Wall -pedantic $(SRCDIR)/main.c $(SRCDIR)/pipes.c $(SRCDIR)/ipc.c $(SRCDIR)/bank_robbery.c $(SRCDIR)/lamport.c -o $(BUILDDIR)/pa4

build: clean
	mkdir -p $(BUILDDIR)
	

all: main

clean:
	rm -rf $(BUILDDIR)
	rm -rf events.log pipes.log
