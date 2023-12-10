#include "queue.h"

Queue queue = {.len=0};

void print_queue(local_id id) {
  printf("DEBUG %i QUEUE: len %i [", id, queue.len);
  for (int i = 0; i < queue.len; i++) {
    printf("(%i %i) ", queue.nodes[i].id, queue.nodes[i].time);
  }
  printf("]\n");
}

void pop_queue() {
  for (int i = 0; i < queue.len - 1; i++) {
    queue.nodes[i] = queue.nodes[i + 1];
  }
  queue.len = queue.len - 1;
}

void add_queue(Node *node) {
  // printf("DEBUG %i: add queue\n", node->id);
  if (queue.len == 0) {
    queue.nodes[0].id = node->id;
    queue.nodes[0].time = node->time;
    queue.len++;
    // printf("DEBUG %i: add queue: queue.len = 0 nodes[0] = %i %i\n", node->id, queue.nodes[0].id, queue.nodes[0].time);
    return;
  }

  queue.len++;
  for (int i = 0; i < queue.len - 1; i++) {
    // printf("DEBUG %i: add queue (%i %i) cur (%i %i)\n", node->id, node->id,
          //  node->time, node->id, node->time);
    Node cur = queue.nodes[i];
    // less time has priority || greater id has prioroty
    if ((node->time < cur.time) ||
        (node->time == cur.time && node->id < cur.id)) {
      for (int j = queue.len - 2; j >= i; j--) {
        queue.nodes[j + 1] = queue.nodes[j];
      }
      queue.nodes[i].id = node->id;
      queue.nodes[i].time = node->time;
      return;
    }
  }
  queue.nodes[queue.len - 1].id = node->id;
  queue.nodes[queue.len - 1].time = node->time;
  
  // printf("DEBUG %i: added to the last queue position\n", node->id);
}

void manage_request(Process* this, Message* msg){
  Node node;
  memcpy(&node, msg->s_payload, msg->s_header.s_payload_len);
  add_queue(&node);
  print_queue(this->id);
}

int wait_cs(Process *this) {
  Message msg;
  // пока мы не на вершине очереди -- ждём ответов других

  while (queue.nodes[0].id != this->id) {
    int replies = 0;
    if (lamport_receive_any(this, &msg) == 0 && replies < this->num_of_processes - 1) {
      printf("DEBUG %i: got request\n", this->id);
      if (msg.s_header.s_type == CS_REQUEST) {
        manage_request(this, &msg);
      } else if (msg.s_header.s_type == CS_REPLY){
        replies++;
      }
    } else {
      printf("Fail to recive message in process %i (wait_for_all)\n", this->id);
      return 1;
    }
  }
  return 0;
}
