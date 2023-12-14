
#include "pa2345.h"
#include "queue.h"


int request_cs(const void *self) {
  Process *this = (Process *)self;
  Message msg;

  // 2. Посылает всем потокам запрос (req)
  create_message(&msg, CS_REQUEST, &msg, 0);
  lamport_send_multicast(this, &msg);
  // 1. Добавляет свой запрос в свою очередь (т.е временную метку и номер
  // потока)
  Node node;
  node.id = this->id;
  node.time = get_lamport_time();
  add_queue(&node);

  // printf("DEBUG %i: sent CS_REQUEST (%i %i)\n", this->id, node.id, node.time);
  // print_queue(node.id);

  // 3. Ждет от них ответа (ok)
  // 4. Получив все ответы, ждет, когда он станет первым в своей очереди, и
  // входит в критическую секцию
  return wait_cs(this);
}

int release_cs(const void *self) {
  // 5. Выйдя из критической секции, удаляет свой запрос из своей очереди,
  // посылает всем сообщение о том, что вышел (rel)
  Process *this = (Process *)self;
  // printf("DEBUG %i: RELEASE critical section\n", this->id);
  pop_queue();
  Message msg;
  create_message(&msg, CS_RELEASE, &msg, 0);
  lamport_send_multicast(this, &msg);
  return 0;
}
