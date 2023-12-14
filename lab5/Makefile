CFLAGS=--std=c17 -Wall -Werror -pedantic -Isrc/ -ggdb -Wextra  -DDEBUG -g -fsanitize=address -L./pa5/lib64/  -lruntime -std=c99
BUILDDIR=build
SRCDIR=pa5
CC=clang

# export LD_LIBRARY_PATH="/home/marsen/itmo/distributed_computing/lab5/pa5/lib64"

# export LD_PRELOAD=/home/marsen/itmo/distributed_computing/lab5/pa5/lib64/libruntime.so
#  ./build/pa5 --mutexl -p

main: build
	$(CC) $(CFLAGS) $(SRCDIR)/main.c $(SRCDIR)/pipes.c $(SRCDIR)/ipc.c \
																	  $(SRCDIR)/lamport.c $(SRCDIR)/cs.c $(SRCDIR)/queue.c \
																	  -o $(BUILDDIR)/pa5 

build: clean
	mkdir -p $(BUILDDIR)

all: main

clean:
	rm -rf $(BUILDDIR)
	rm -rf events.log pipes.log
