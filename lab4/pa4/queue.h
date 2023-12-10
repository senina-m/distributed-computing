#ifndef __IFMO_DISTRIBUTED_CLASS_QUEUE__H
#define __IFMO_DISTRIBUTED_CLASS_QUEUE__H


#include "processes.h"
#include <stdlib.h>
#include "ipc.h"
#include "lamport.h"

typedef struct {
  timestamp_t time;
  local_id id;
} Node;

typedef struct {
  int len;
  Node nodes[MAX_PROCESS_ID];
} Queue;

void print_queue(local_id id);
void pop_queue();
void add_queue(Node *node);
void manage_request(Process* this, Message* msg);
int wait_cs(Process* this);

#endif
