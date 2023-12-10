#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#include "common.h"
#include "cs.h"
#include "ipc.h"
#include "pa2345.h"
#include "pipes.h"

#define BUF_SIZE 1024

void free_process(Process *ptr) {
  free_pipes(ptr->pipes, ptr->num_of_processes);
  free(ptr->log);
  free(ptr);
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
  local_id id = 1; // because parent doesn't lamport_send anything
  while (id < n) {
    if (id == this->id)
      id++;
    else {
      if (lamport_receive(this, id, &msg) == 0) {
        // printf("Process %i received message \'%s\'\n", this->id,
        // msg.s_payload);
        if (msg.s_header.s_type == t)
          id++;
      } else {
        printf("Can't receive STARTED messages from process %i in process %i\n",
               id, this->id);
        return 1;
      }
    }
  }
  return 0;
}

int wait_for_history(Process *this, AllHistory *all_history) {
  Message msg;
  all_history->s_history_len = 0;
  while (all_history->s_history_len < this->num_of_processes - 1) {
    if (lamport_receive_any(this, &msg) == 0) {
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

void print_h(local_id id, BalanceState *history, uint8_t len) {
  printf("DEBUG %i: HISTORY time=%i, balance=%i pending=%i\n", id,
         history[len].s_time, history[len].s_balance,
         history[len].s_balance_pending_in);
}

void fill_balance_history(BalanceState history[], uint8_t *len,
                          timestamp_t time, balance_t balance,
                          balance_t pending) {
  balance_t current_bal = history[*len].s_balance;
  balance_t current_pen = history[*len].s_balance_pending_in;
  // printf("DEBUG: current_bal=%i, time=%i, balance=%i, len=%i\n", current_bal,
  // time, balance, *len);

  for (uint8_t i = *len + 1; i < time; i++) {
    history[i].s_balance_pending_in = current_pen;
    history[i].s_time = i;
    history[i].s_balance = current_bal;
  }
  history[time].s_balance = balance;
  history[time].s_time = time;
  // printf("DEBUG: %i=cur_pend %i=new_pend\n", current_pen, pending);
  history[time].s_balance_pending_in = current_pen + pending;
  *len = time;
}

void change_balance(Process *this, BalanceState *history, uint8_t *len,
                    balance_t amount) {
  timestamp_t now = get_lamport_time();
  this->balance += amount;
  // balance_t pending = amount > 0? 0 : -amount;
  balance_t pending = -amount;
  fill_balance_history(history, len, now, this->balance, pending);
  // print_h(this->id, history, *len);
  // printf("DEBUG %i: send ACK that I received amount=%i from=%i\n", this->id,
  // order->s_amount, order->s_src);
}

int run_child_rutine(Process *this) {
  // START section
  logger(this->log->processes, log_started_fmt, this->id, this->pid,
         this->parent_pid, this->balance);

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

  BalanceState history[MAX_T + 1];
  history[0].s_balance = this->balance;
  history[0].s_balance_pending_in = 0;
  history[0].s_time = 0;
  uint8_t history_len = 0;

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
    case TRANSFER:
      // printf("DEBUG %i: receive TRANSFER\n", this->id);

      order = (TransferOrder *)receive_msg.s_payload;
      if (order->s_src == this->id) {
        // printf("DEBUG %i: TRANSFER SRC\n", this->id);
        change_balance(this, history, &history_len, -order->s_amount);
        lamport_send(this, order->s_dst, &receive_msg);
        logger(this->log->processes, log_transfer_out_fmt, this->id,
               order->s_amount, order->s_dst);
      } else if (order->s_dst == this->id) {
        // printf("DEBUG %i: TRANSFER DST\n", this->id);
        change_balance(this, history, &history_len, order->s_amount);
        logger(this->log->processes, log_transfer_in_fmt, this->id,
               order->s_amount, order->s_src);
        Message ack;
        create_message(&ack, ACK, NULL, 0);
        lamport_send(this, 0, &ack);
      } else {
        printf("Alien message receive: src %d, dst %d, in clid %d\n",
               order->s_src, order->s_dst, this->id);
      }
      break;
    default:
      printf("Non expected type of message receive %d in clid %d\n",
             msg.s_header.s_type, this->id);
      break;
    }
  }

  logger(this->log->processes, log_done_fmt, this->id, this->balance);

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
    case TRANSFER:
      order = (TransferOrder *)msg.s_payload;
      if (order->s_src == this->id) {
        printf("Too late to transfer message to src child %d\n", this->id);
      } else if (order->s_dst == this->id) {
        change_balance(this, history, &history_len, order->s_amount);
        logger(this->log->processes, log_transfer_in_fmt, this->id,
               order->s_amount, order->s_src);
        Message ack;
        create_message(&ack, ACK, NULL, 0);
        lamport_send(this, 0, &ack);
      } else {
        printf("Alien message receive: src %d, dst %d, in clid %d\n",
               order->s_src, order->s_dst, this->id);
      }
      break;
    case DONE:
      count--;
      break;
    default:
      printf("Non expected type of message receive %d\n", msg.s_header.s_type);
      break;
    }
  }

  logger(this->log->processes, log_received_all_done_fmt, this->id);
  fill_balance_history(history, &history_len, get_lamport_time(), this->balance,
                       0);
  history_len++;
  size_t s_history_len = sizeof(BalanceState) * history_len;
  BalanceHistory b_history;
  memcpy(b_history.s_history, history, s_history_len);
  b_history.s_history_len = history_len;
  b_history.s_id = this->id;
  Message his_msg;
  create_message(&his_msg, BALANCE_HISTORY, &b_history,
                 sizeof(local_id) + sizeof(uint8_t) + s_history_len);
  lamport_send(this, 0, &his_msg);

  // printf("DEBUG %i: sent HISTORY to parent\n", this->id);

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
  bank_robbery(this, this->num_of_processes - 1); // do robbery

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
  //------------ collect BALANCE_HISTORY ---------------

  AllHistory *all_history = (AllHistory *)malloc(sizeof(AllHistory));
  if (wait_for_history(this, all_history) != 0) {
    printf("Fail to receive BALANCE_HISTORY from all clients %i\n", this->id);
    return 1;
  }
  // ----------- print BALANCE_HISTORY -----------------

  print_history(all_history);
  // printf("DEBUG: HISTORY 1\n");
  // for (int i = 0; i < all_history[0].s_history_len; i++){
  //   print_h((all_history[0]).s_history);
  // }
  free(all_history);

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
  // printf("DEBUG %i: sent TRANSFER from %i to %i amount=%i \n", this->id, src,
  // dst, amount);
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

  bool critical;
  if (strcmp(argv[1], "--mutexl") == 0) {
    critical = true;
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
      balances[i] = atoi(argv[3 + i]);
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
  this->is_cs = critical;

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
      this->is_cs = critical;
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
