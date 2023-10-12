
#ifndef __IFMO_DISTRIBUTED_CLASS_PIPES__H
#define __IFMO_DISTRIBUTED_CLASS_PIPES__H

#define mmalloc(T) ((T*)malloc(sizeof(T)))
#define mmalloc_array(T, count) ((T**)malloc(sizeof(T)*count))

#include <fcntl.h>
#include <unistd.h>
#include "processes.h"

Pipe** alloc_pipes(int n);

int init_pipes(Process* this);

void free_pipes(Pipe** ptr, int n);


#endif // __IFMO_DISTRIBUTED_CLASS_PIPES__H