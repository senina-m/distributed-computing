#include "queue.h"

Queue queue = {.len = 0};

timestamp_t my_request_time = -1;

void set_my_request_time(timestamp_t time){
  my_request_time = time;
}

/*if his request is more priorited than mine return 1, else 0*/
int compare_requests(Node* node, local_id my_id){
  if (my_request_time == -1) return 1;
  if ((node->time < my_request_time) ||
        (node->time == my_request_time && node->id < my_id)) {
      return 1;
  } else return 0;
}

void print_queue(local_id id) {
  printf("DEBUG %i QUEUE: len %i [", id, queue.len);
  for (int i = 0; i < queue.len; i++) {
    printf("(%i %i) ", queue.nodes[i].id, queue.nodes[i].time);
  }
  printf("]\n");
}

void add_queue(Node *node) {
  queue.nodes[queue.len].id = node->id;
  queue.nodes[queue.len].time = node->time;
  queue.len++;
}

void get_node_from_msg(Message* msg, Node* node){
  memcpy(node, msg->s_payload, sizeof(Node));
}

void send_queue(Process* this){
  Message msg;
  for (int i = 0; i < queue.len; i++){
    create_message(&msg, CS_REPLY, &msg, 0);
    lamport_send(this, queue.nodes[i].id, &msg);
    // printf("DEBUG %i: send delayed CS_REPLY (%i %i)\n", this->id, queue.nodes[i].id, queue.nodes[i].time);
  }
  queue.len = 0;
}

int manage_CS(Process* this){
  Message msg;
  Node node;
  if (lamport_receive_any(this, &msg) == 0) {
      get_node_from_msg(&msg, &node);
      // При получении запроса от другого потока, запрос добавляется в очередь и запрашивающему потоку посылается ответ
      if (msg.s_header.s_type == CS_REQUEST) {
          // printf("DEBUG %i: got CS_REQUEST (%i %i)\n", this->id, node.id, node.time);
          if (compare_requests(&node, this->id) == 0){
              add_queue(&node);
              // print_queue(this->id);
          }else{
            Message send_msg;
            create_message(&send_msg, CS_REPLY, &msg, 0);
            lamport_send(this, node.id, &send_msg);
            // printf("DEBUG %i: send CS_REPLY (%i %i)\n", this->id, node.id, node.time);
          }
      } else if (msg.s_header.s_type == CS_REPLY) {
          // printf("DEBUG %i: got CS_REPLY from %i\n", this->id, node.id);
          return 1;
      } else if (msg.s_header.s_type == DONE){
        this->done++;
        // printf("DEBUG %i: got DONE %i\n", this->id, this->done);
      }
  }
  return 0;
}

int wait_cs(Process *this) {
  //4. Процесс может войти в критический участок только после получения откликов ото всех других узлов сети.
    int replies = 0;
    while (replies != this->num_of_processes - 2) {
        replies += manage_CS(this);
    }
    return 0;
}
