#include "queue.h"

Queue queue = {.len = 0};

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
    // printf("DEBUG %i: add queue: queue.len = 0 nodes[0] = %i %i\n", node->id,
    // queue.nodes[0].id, queue.nodes[0].time);
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

void manage_request(Process *this, Message *msg) {
  Node node;
  memcpy(&node, msg->s_payload, msg->s_header.s_payload_len);
  printf("DEBUG %i: got CS_REQUEST (%i %i)\n", this->id, node.id, node.time);
  add_queue(&node);
  print_queue(this->id);
  Message send_msg;
  create_message(&send_msg, CS_REPLY, &msg, 0);
  lamport_send(this, node.id, &send_msg);
  printf("DEBUG %i: send CS_REPLY (%i %i)\n", this->id, node.id, node.time);
}

void queue_drop_by_id(local_id proc_id){
  bool found = false;
  for (int i = 0; i < queue.len - 1; i++) {
    if (queue.nodes[i].id == proc_id) found = true;
    if (found)
      queue.nodes[i] = queue.nodes[i + 1];
  }
  queue.len = queue.len - 1;
}

int manage_CS(Process* this){
  Message msg;
  if (lamport_receive_any(this, &msg) == 0) {
      // При получении запроса от другого потока, запрос добавляется в очередь и запрашивающему потоку посылается ответ
      if (msg.s_header.s_type == CS_REQUEST) {
          manage_request(this, &msg);
      } else if (msg.s_header.s_type == CS_REPLY) {
          printf("DEBUG %i: got CS_REPLY\n", this->id);
          return 1;
      // При получении release от другого потока, его запрос удаляется из очереди
      } else if (msg.s_header.s_type == CS_RELEASE){
          local_id proc_id = *msg.s_payload;
          printf("DEBUG %i: got CS_RELEASE from %i\n", this->id, proc_id);
          queue_drop_by_id(proc_id);
          print_queue(this->id);
      } else if (msg.s_header.s_type == DONE){
        this->done++;
        printf("DEBUG %i: got DONE %i\n", this->id, this->done);
      }
  }
  return 0;
}

int wait_cs(Process *this) {
  // пока мы не на вершине очереди -- ждём ответов других
    int replies = 0;
    while (queue.nodes[0].id != this->id ||
            replies != this->num_of_processes - 2) {
        // printf("DEBUG %i: node first=%i replies=%i\n", this->id,
            // queue.nodes[0].id != this->id, replies);
        replies += manage_CS(this);
    }
    return 0;
}
