#ifndef __IFMO_DISTRIBUTED_CLASS_QUEUE__H
#define __IFMO_DISTRIBUTED_CLASS_QUEUE__H


#include "processes.h"
#include <stdlib.h>
#include "ipc.h"
#include "lamport.h"

typedef struct {
  local_id id;
  timestamp_t time;
} Node;

typedef struct {
  int len;
  Node nodes[MAX_PROCESS_ID];
} Queue;

void print_queue(local_id id);
void pop_queue();
void add_queue(Node *node);
int manage_CS(Process* this);
int wait_cs(Process* this);

#endif
