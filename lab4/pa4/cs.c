
#include "processes.h"
#include "ipc.h"
#include "lamport.h"
#include "pa2345.h"

typedef struct {
  timestamp_t time;
  local_id id;
} Node;

typedef struct {
  int len;
  Node *nodes;
} Queue;

Queue *queue;

Queue *create_queue(int size) {
  Queue *q = (Queue *)calloc(1, sizeof(Queue));
  q->nodes = (Node *)calloc(1, sizeof(Node) * size);
  q->len = 0;
  return q;
}

void pop_queue() {
  for (int i = 0; i < queue->len - 1; i++) {
    queue->nodes[i] = queue->nodes[i + 1];
  }
  queue->len = queue->len - 1;
}

void add_queue(Node *node) {
  if (queue->len == 0) {
    queue->nodes[queue->len++].id = node->id;
    queue->nodes[queue->len++].time = node->time;
    return;
  }
  queue->len++;
  for (int i = 0; i < queue->len - 1; i++) {
    Node cur = queue->nodes[i];
    // less time has priority || greater id has prioroty
    if ((node->time < cur.time) ||
        (node->time == cur.time && node->id < cur.id)) {
      for (int j = queue->len - 2; j >= i; j--) {
        queue->nodes[j + 1] = queue->nodes[j];
      }
      queue->nodes[i].id = node->id;
      queue->nodes[i].time = node->time;
      return;
    }
  }
  queue->nodes[queue->len - 1].id = node->id;
  queue->nodes[queue->len - 1].time = node->time;
}

void wait_cs(Process *this) {
  Message msg;
  // пока мы не на вершине очереди -- ждём ответов других
  while (queue->nodes[0].id != this->id) {
    if (lamport_receive_any(this, &msg) == 0) {
      printf("DEBUG %i: got request\n", this->id);
      if (msg.s_header.s_type == CS_REQUEST) {
        Node node;
        memcpy(&node, msg.s_payload, msg.s_header.s_payload_len);
        add_queue(&node);
      }
    } else {
      printf("Fail to recive message in process %i (wait_for_all)\n", this->id);
      // return 1;
    }
  }
  return 0;
}

int request_cs(const void * self){
  Process *this = (Process *)self;
  Message msg;
  Node node;
  node.id = this->id;
  node.time = get_lamport_time();
  // 1. Добавляет свой запрос в свою очередь (т.е временную метку и номер
  // потока)
  add_queue(&node);
  // 2. Посылает всем потокам запрос (req)
  create_message(&msg, CS_REQUEST, &node, sizeof(Node));

  // 3. Ждет от них ответа (ok)
  // 4. Получив все ответы, ждет, когда он станет первым в своей очереди, и
  // входит в критическую секцию
  wait_cs(this);
}

int release_cs(const void * self){
  // 5. Выйдя из критической секции, удаляет свой запрос из своей очереди,
  // посылает всем сообщение о том, что вышел (rel)
  Process *this = (Process *)self;
  printf("DEBUG %i: RELEASE critical section\n", this->id);
  pop_queue();
  Message msg;
  create_message(&msg, CS_RELEASE, NULL, 0);
  int lamport_send_multicast(this, msg);
  return 0;
}

void print(const char * s){
  printf(s);
}