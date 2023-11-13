#include <argp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#include "banking.h"
#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "pipes.h"

#define BUF_SIZE 1024

#define logger(file, str, ...)                                                 \
  {                                                                            \
    fprintf(file, str, __VA_ARGS__);                                           \
    printf(str, __VA_ARGS__);                                                  \
  }

void free_process(Process *ptr) {
  free_pipes(ptr->pipes, ptr->num_of_processes);
  free(ptr->log);
  free(ptr);
}

void create_message(Message *msg, MessageType type, void *contents, int len) {
  msg->s_header.s_type = type;
  msg->s_header.s_magic = MESSAGE_MAGIC;
  msg->s_header.s_local_time = get_physical_time();
  if (contents != NULL) {
    msg->s_header.s_payload_len = len;
    memcpy(&(msg->s_payload), contents, len);
  }
}

int wait_for_all(Process *this, MessageType t) {
  Message msg;
  int amount = 0;
  // if it's parent proc we need to wait n-1, if it's client proc we need to
  // wait n-2;
  int n = this->num_of_processes - ((this->id == 0) ? 1 : 2);
  while (amount < n) {
    if (receive_any(this, &msg) == 0) {
      if (msg.s_header.s_type == t)
        amount++;
    } else {
      printf("Fail to recive message in process %i (wait_for_all)\n", this->id);
      return 1;
    }
  }
  return 0;
}

int wait_for_history(Process *this, AllHistory *all_history) {
  Message msg;
  all_history->s_history_len = 0;
  while (all_history->s_history_len < this->num_of_processes - 1) {
    if (receive_any(this, &msg) == 0) {
      if (msg.s_header.s_type == BALANCE_HISTORY) {
        BalanceHistory history;
        memcpy(&history, &(msg.s_payload), sizeof(msg.s_payload));
        all_history->s_history[history.s_id - 1] = history;
        all_history->s_history_len++;
      }
    } else {
      printf("Can't receive BALANCE messages in parent \n");
      return 1;
    }
  }
  return 0;
}

int run_child_rutine(Process *this) {

  // START section
  logger(this->log->processes, log_started_fmt, this->id, this->pid,
         this->parent_pid);

  Message msg;
  create_message(&msg, STARTED, NULL, 0);

  if (send_multicast(this, &msg) != 0) {
    printf("Fail to do multicast STARTED request from process %i\n", this->id);
    return 1;
  }

  if (wait_for_all(this, STARTED) != 0) {
    printf("Fail to receive all STARTED messages in child %i\n", this->id);
    return 1;
  } else
    logger(this->log->processes, log_received_all_started_fmt, this->id);

  // here we do our work
  logger(this->log->processes, log_done_fmt, this->id);

  // TODO:   while(){
  //          recive_message();
  //          if STOP
  //          if TRANSFER ->
  //              if THIS == src -> send money to dist
  //              else -> send ACK to parent
  //          if DONE -> send HISTORY
  //        }
  // log that we've done all our work

  create_message(&msg, DONE, NULL, 0);

  if (send_multicast(this, &msg) != 0) {
    printf("Fail to do multicast DONE request from process %i\n", this->id);
    return 1;
  }

  if (wait_for_all(this, DONE) != 0) {
    printf("Fail to receive all DONE messages %i\n", this->id);
    return 1;
  } else
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

  bank_robbery(this, this->num_of_processes - 1); // do robbery

  // ------------ send for all STOP message -------------
  Message msg;
  create_message(&msg, STOP, NULL, 0);

  if (send_multicast(this, &msg) != 0) {
    printf("Fail to do multicast STOP request from process parent %i\n",
           this->id);
    return 1;
  }
  // ----------- wait for DONE messages from all -------
  if (wait_for_all(this, DONE) != 0) {
    printf("Fail to receive all DONE messages %i\n", this->id);
    return 1;
  } else
    logger(this->log->processes, log_received_all_done_fmt, this->id);
  //------------ collect BALANCE_HISTORY ---------------

  AllHistory *all_history;
  if (wait_for_history(this, all_history) != 0) {
    printf("Fail to receive BALANCE_HISTORY from all clients %i\n", this->id);
    return 1;
  }
  // ----------- print BALANCE_HISTORY -----------------

  print_history(all_history);

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
  // ----------- send transfer request ----------------
  Message msg;
  TransferOrder order;
  order.s_src = src;
  order.s_dst = dst;
  order.s_amount = amount;
  create_message(&msg, TRANSFER, &order, sizeof(TransferOrder));

  send(this, dst, &msg);
  logger(this->log->processes, log_transfer_in_fmt, this->id);

  // ----------- listen for ASK -----------------------
  receive(this, dst, &msg);
  if (msg.s_header.s_type == ACK) {
    logger(this->log->processes, log_transfer_out_fmt, this->id);
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

  balance_t *balances = (balance_t *)malloc(sizeof(balance_t) * (total_N - 1));
  if (argc < total_N + 2) { // command + -p + N + balance*(N-1)
    printf("Too little amount of balances for clients!\n");
    return -1;
  } else {
    for (int i = 0; i < total_N - 1; i++) {
      balances[i] = atoi(argv[2 + i]);
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
  this->balance = -1;

  if (init_pipes(this))
    return -1;

  pid_t my_parent_pid = this->pid;
  for (int i = 1; i < this->num_of_processes; i++) {
    if (fork() == 0) {
      this->parent_id = 0;
      this->parent_pid = my_parent_pid;
      this->pid = getpid();
      this->id = i;
      this->balance = balances[i - 1];
      break;
    }
  }
  free(balances);

  if (close_unused_pipes(this))
    return -1;

  if (this->parent_id == 0) {
    run_child_rutine(this);
  } else if (run_parent_rutine(this))
    return -1;
}