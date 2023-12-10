
#ifndef __IFMO_DISTRIBUTED_CLASS_PROCESSES__H
#define __IFMO_DISTRIBUTED_CLASS_PROCESSES__H
#include "ipc.h"
#include "banking.h"
#include <stdio.h> 
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#define logger(file, str, ...)                                                 \
  {                                                                            \
    timestamp_t time = get_lamport_time();                                     \
    fprintf(file, str, time, __VA_ARGS__);                                     \
    printf(str, time, __VA_ARGS__);                                            \
  }

typedef struct{
    int fw;
    int fr;
} Pipe;

typedef struct{
    FILE* processes;
    FILE* pipes;
} Log;

typedef struct{
    local_id id;
    local_id parent_id;
    pid_t pid;
    pid_t parent_pid;
    int num_of_processes; 
    Pipe*** pipes;
    Log* log;
    balance_t balance;
    bool is_cs;
} Process;

#endif // __IFMO_DISTRIBUTED_CLASS_PROCESSES__H
