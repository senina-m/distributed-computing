
#include "pa2345.h"
#include "queue.h"


int request_cs(const void *self) {
  Process *this = (Process *)self;
  Message msg;
  Node node;
  node.id = this->id;
  node.time = get_lamport_time();
  printf("DEBUG %i: Going to grab critical section with (%i %i)\n", this->id,
         node.id, node.time);
  // 1. Добавляет свой запрос в свою очередь (т.е временную метку и номер
  // потока)
  add_queue(&node);
  print_queue(node.id);

  // printf("DEBUG %i: ADDed to queue t:%i i:%i\n", this->id, node.time, node.id);
  // 2. Посылает всем потокам запрос (req)
  create_message(&msg, CS_REQUEST, &node, sizeof(Node));
  lamport_send_multicast(this, &msg);

  printf("DEBUG %i: sent CS_REQUEST (%i %i)\n", this->id, node.id, node.time);

  // 3. Ждет от них ответа (ok)
  // 4. Получив все ответы, ждет, когда он станет первым в своей очереди, и
  // входит в критическую секцию
  return wait_cs(this);
}

int release_cs(const void *self) {
  // 5. Выйдя из критической секции, удаляет свой запрос из своей очереди,
  // посылает всем сообщение о том, что вышел (rel)
  Process *this = (Process *)self;
  printf("DEBUG %i: RELEASE critical section\n", this->id);
  pop_queue();
  Message msg;
  local_id id = this->id;
  create_message(&msg, CS_RELEASE, &id, sizeof(local_id));
  printf("DEBUG %i: payload \"%i\"\n", this->id, *msg.s_payload);
  lamport_send_multicast(this, &msg);
  return 0;
}

void print(const char *s) { printf("%s", s); }
