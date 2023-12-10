#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#include "banking.h"
#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "pipes.h"
#include "lamport.h"

#define logger(file, str, ...)                                                 \
  {                                                                            \
    timestamp_t time = get_lamport_time();                                    \
    fprintf(file, str, time, __VA_ARGS__);                                     \
    printf(str, time, __VA_ARGS__);                                            \
  }

void free_process(Process *ptr) {
  free_pipes(ptr->pipes, ptr->num_of_processes);
  free(ptr->log);
  free(ptr);
}

int wait_for_all(Process *this, MessageType t) {
  Message msg;
  int amount = 0;
  // if it's parent proc we need to wait n-1, if it's client proc we need to
  // wait n-2;
  int n = this->num_of_processes - ((this->id == 0) ? 1 : 2);
  while (amount < n) {
    if (lamport_receive_any(this, &msg) == 0) {
      // printf("DEBUG %i: go msg %i\n", this->id, msg.s_header.s_type);
      if (msg.s_header.s_type == t)
        amount++;
      // printf("DEBUG %i: amount %i\n", this->id, amount);
    } else {
      printf("Fail to recive message in process %i (wait_for_all)\n", this->id);
      // return 1;
    }
  }
  return 0;
}

int blocked_wait_for_all(Process *this, MessageType t) {
  Message msg;
  int n = this->num_of_processes;
  local_id id = 1; //because parent doesn't lamport_send anything
  while(id < n){
      if(id == this->id) id++;
      else{
          if(lamport_receive(this, id, &msg) == 0){
              // printf("Process %i received message \'%s\'\n", this->id, msg.s_payload);
              if(msg.s_header.s_type == t) id++;
          }else{
              printf("Can't receive STARTED messages from process %i in process %i\n", id, this->id);
              return 1;
          }
      }
  }
  return 0;
}


int run_child_rutine(Process *this) {
  // START section
  logger(this->log->processes, log_started_fmt, this->id, this->pid,
         this->parent_pid, 0);

  Message msg;
  create_message(&msg, STARTED, NULL, 0);

  if (lamport_send_multicast(this, &msg) != 0) {
    printf("Fail to do multicast STARTED request from process %i\n", this->id);
    return 1;
  }

  if (blocked_wait_for_all(this, STARTED) != 0) {
    printf("Fail to receive all STARTED messages in child %i\n", this->id);
    return 1;
  } else
    logger(this->log->processes, log_received_all_started_fmt, this->id);

  // 3. Ждет от них ответа (ok)
// 4. Получив все ответы, ждет, когда он станет первым в своей очереди, и входит в критическую секцию

  Message receive_msg;
  bool is_waiting = true;
  TransferOrder *order;
  while (is_waiting) {
    // printf("DEBUG %i: try to receive some msg\n", this->id);
    lamport_receive_any(this, &receive_msg);
    switch (receive_msg.s_header.s_type) {
    case STOP:
      // printf("DEBUG %i: receive STOP\n", this->id);
      is_waiting = false;
      break;
    default:
      printf("Non expected type of message receive %d in clid %d\n",
             msg.s_header.s_type, this->id);
      break;
    }
  }

  logger(this->log->processes, log_done_fmt, this->id, 0);

  create_message(&msg, DONE, NULL, 0);

  if (lamport_send_multicast(this, &msg) != 0) {
    printf("Fail to do multicast DONE request from process %i in clid %d\n",
           this->id, this->id);
    return 1;
  }

  int count = this->num_of_processes - 2;
  while (count > 0) {
    lamport_receive_any(this, &msg);
    switch (msg.s_header.s_type) {
    case DONE:
      count--;
      break;
    default:
      printf("Non expected type of message receive %d\n", msg.s_header.s_type);
      break;
    }
  }

  logger(this->log->processes, log_received_all_done_fmt, this->id);

  if (close_used_pipes(this) != 0) {
    printf("Fail to close used pipes %i\n", this->id);
    return 1;
  }

  fclose(this->log->pipes);
  fclose(this->log->processes);
  free_process(this);
  exit(EXIT_SUCCESS);
}

int run_parent_rutine(Process *this) {

  // ------------ wait for all STARTED ------------------
  if (wait_for_all(this, STARTED) != 0) {
    printf("Fail to receive all STARTED messages in parent %i\n", this->id);
    return 1;
  } else
    logger(this->log->processes, log_received_all_started_fmt, this->id);

  // ------------ do robbery ----------------------------
  // printf("DEBUG: DO BANCK ROBBERY\n");
  // bank_robbery(this, this->num_of_processes - 1); // do robbery

  // ------------ lamport_send for all STOP message -------------
  Message msg;
  create_message(&msg, STOP, NULL, 0);

  if (lamport_send_multicast(this, &msg) != 0) {
    printf("Fail to do multicast STOP request from process parent %i\n",
           this->id);
    return 1;
  }
  // ----------- wait for DONE messages from all -------
  if (wait_for_all(this, DONE) != 0) {
    printf("Fail to lamport_receive all DONE messages %i\n", this->id);
    return 1;
  } else
    logger(this->log->processes, log_received_all_done_fmt, this->id);

  // ----------- close all processes ------------------

  for (int i = 1; i < this->num_of_processes; i++) {
    if (wait(NULL) == -1) {
      printf("Fail to close %i children\n", this->num_of_processes - i);
      return 1;
    }
  }

  if (close_used_pipes(this) != 0) {
    printf("Fail to close used pipes %i\n", this->id);
    return 1;
  }

  fclose(this->log->pipes);
  fclose(this->log->processes);
  free_process(this);
  return 0;
}

void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
  Process *this = (Process *)parent_data;
  // ----------- lamport_send transfer request ----------------
  Message msg;
  TransferOrder order;
  order.s_src = src;
  order.s_dst = dst;
  order.s_amount = amount;
  create_message(&msg, TRANSFER, &order, sizeof(TransferOrder));
  // printf("DEBUG %i: sent TRANSFER from %i to %i amount=%i \n", this->id, src, dst, amount);
  lamport_send(this, src, &msg);

  // ----------- listen for ASK -----------------------
  lamport_receive(this, dst, &msg);
  if (msg.s_header.s_type == ACK) {
    // printf("DEBUG %i: lamport_receive ACK from %i\n", this->id, dst);
  } else {
    printf("ERROR resciving ASK from client %i", dst);
  }
}

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    printf("Invalid amount of arguments = %i\n", argc);
    return -1;
  }

  local_id total_N = 0;
  if (strcmp(argv[1], "-p") == 0) {
    total_N = atoi(argv[2]) + 1;
    if (total_N > 10 || total_N < 1) {
      printf("Num of processes has to be from 1 to 10\n");
      return -1;
    }
  }

  // setup parent process
  Process *this = mmalloc(Process);
  this->id = 0;
  this->parent_id = -1;
  this->parent_pid = -1;
  this->pid = getpid();
  this->num_of_processes = total_N;
  this->log = mmalloc(Log);
  this->log->processes = fopen(events_log, "a");
  this->log->pipes = fopen(pipes_log, "w");
  this->pipes = alloc_pipes(total_N);

  if (init_pipes(this))
    return -1;

  pid_t my_parent_pid = this->pid;
  for (int i = 1; i < this->num_of_processes; i++) {
    if (fork() == 0) {
      this->parent_id = 0;
      this->parent_pid = my_parent_pid;
      this->pid = getpid();
      this->id = i;

      break;
    }
  }

  if (close_unused_pipes(this))
    return -1;
  if (this->parent_id == 0) {
    run_child_rutine(this);
  } else if (run_parent_rutine(this))
    return -1;
}
