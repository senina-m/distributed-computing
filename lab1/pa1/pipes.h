
#ifndef __IFMO_DISTRIBUTED_CLASS_PIPES__H
#define __IFMO_DISTRIBUTED_CLASS_PIPES__H

#define mmalloc(T) ((T*)malloc(sizeof(T)))
#define mmalloc_array(T, count) ((T**)malloc(sizeof(T)*count))

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "processes.h"

Pipe*** alloc_pipes(int n);

int init_pipes(Process* this);

void free_pipes(Pipe*** ptr, int n);

int close_unused_pipes(Process* this);

int close_used_pipes(Process* this);

#endif // __IFMO_DISTRIBUTED_CLASS_PIPES__H