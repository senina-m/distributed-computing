CFLAGS=--std=c17 -Wall -Werror -pedantic -Isrc/ -ggdb -Wextra  -DDEBUG -g -fsanitize=address -L./pa4/lib64/  -lruntime -std=c99
BUILDDIR=build
SRCDIR=pa4
CC=clang

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/marsen/itmo/distributed_computing/lab4/pa4/lib64"

LD_PRELOAD=/home/marsen/itmo/distributed_computing/lab4/pa4/lib64/libruntime.so ./build/pa4 --mutexl -p

main: build
	$(CC) $(CFLAGS) $(SRCDIR)/main.c $(SRCDIR)/pipes.c $(SRCDIR)/ipc.c \
																	  $(SRCDIR)/lamport.c $(SRCDIR)/cs.c $(SRCDIR)/queue.c \
																	  -o $(BUILDDIR)/pa4 

build: clean
	mkdir -p $(BUILDDIR)

all: main

clean:
	rm -rf $(BUILDDIR)
	rm -rf events.log pipes.log