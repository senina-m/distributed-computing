
#include "pa2345.h"
#include "queue.h"


int request_cs(const void *self) {
  Process *this = (Process *)self;
  Message msg;

  // 1. Посылает всем потокам запрос (req)
  create_message(&msg, CS_REQUEST, &msg, 0);
  lamport_send_multicast(this, &msg);
  // printf("DEBUG %i: send CS_REQUEST (%i %i)\n", this->id, this->id, get_lamport_time());
  

  set_my_request_time(get_lamport_time());

  // 2. Ждет от них ответа (ok)
  // 3. Получив все ответы, входит в критическую секцию
  return wait_cs(this);
}

int release_cs(const void *self) {
  // 5. После выхода из него он рассылает задержанные отклики на все ожидающие запросы.
  Process *this = (Process *)self;
  // printf("DEBUG %i: RELEASE critical section\n", this->id);
  send_queue(this);
  set_my_request_time(-1);
  
  return 0;
}
