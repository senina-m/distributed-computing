#include "cs.h"

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

void add_queue(timestamp_t time, local_id id) {
  if (queue->len == 0) {
    queue->nodes[queue->len++].id = id;
    queue->nodes[queue->len++].time = time;
    return;
  }
  queue->len++;
  for (int i = 0; i < queue->len - 1; i++) {
    Node cur = queue->nodes[i];
    // less time has priority || greater id has prioroty
    if ((time < cur.time) || (time == cur.time && id < cur.id)) {
      for (int j = queue->len - 2; j >= i; j--) {
        queue->nodes[j + 1] = queue->nodes[j];
      }
      queue->nodes[i].id = id;
      queue->nodes[i].time = time;
      return;
    }
  }
  queue->nodes[queue->len - 1].id = id;
  queue->nodes[queue->len - 1].time = time;
}

int request_cs(Process *this) {
// 1. Добавляет свой запрос в свою очередь (т.е временную метку и номер потока)
// 2. Посылает всем потокам запрос (req)
// 3. Ждет от них ответа (ok)
// 4. Получив все ответы, ждет, когда он станет первым в своей очереди, и входит в критическую секцию
}

int release_cs(Process *this) {
// 5. Выйдя из критической секции, удаляет свой запрос из своей очереди, посылает всем сообщение о том, что вышел (rel)
  printf("DEBUG %i: RELEASE critical section\n", this->id);
  pop_queue();
  Message msg;
  create_message(&msg, CS_RELEASE, NULL, 0);
  int lamport_send_multicast(this, msg);
  return 0;
}

void create_message(Message *msg, MessageType type, void *contens, int len) {
  msg->s_header.s_type = type;
  msg->s_header.s_magic = MESSAGE_MAGIC;
  msg->s_header.s_local_time = -1;
  if (contens != NULL) {
    memcpy(msg->s_payload, contens, len);
    msg->s_header.s_payload_len = len;
  }
}