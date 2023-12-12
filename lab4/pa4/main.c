#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "pipes.h"
#include "lamport.h"
#include "queue.h"

bool critical = false;

#define logger(file, str, ...)                                                 \
  {                                                                            \
    fprintf(file, str, __VA_ARGS__);                                     \
    printf(str, __VA_ARGS__);                                            \
  }

void free_process(Process *ptr) {
  free_pipes(ptr->pipes, ptr->num_of_processes);
  free(ptr->log);
  free(ptr);
}

int wait_for_all(Process *this, MessageType t) {
  Message msg;
  int amount = 0;
  int n = this->num_of_processes - ((this->id == 0) ? 1 : 2);
  while (amount < n) {
    if (lamport_receive_any(this, &msg) == 0) {
      if (msg.s_header.s_type == (int16_t)t)
        amount++;
    } else {
      printf("Fail to recive message in process %i (wait_for_all)\n", this->id);
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
              if(msg.s_header.s_type == (short)t) id++;
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
  logger(this->log->processes, log_started_fmt, get_lamport_time(), this->id, this->pid,
         this->parent_pid, 0);

  Message msg;
  create_message(&msg, STARTED, &msg, 0);

  if (lamport_send_multicast(this, &msg) != 0) {
    printf("Fail to do multicast STARTED request from process %i\n", this->id);
    return 1;
  }

  if (blocked_wait_for_all(this, STARTED) != 0) {
    printf("Fail to receive all STARTED messages in child %i\n", this->id);
    return 1;
  } else
    logger(this->log->processes, log_received_all_started_fmt, get_lamport_time(), this->id);

  int iterations = 5;
  for (int i = 1; i <= this->id * iterations; i++){
		if (critical){
			request_cs(this);
		}
		
    // printf("DEBUG %i: In CS\n", this->id);

    char buffer[MAX_PAYLOAD_LEN];
		snprintf(buffer, MAX_PAYLOAD_LEN, log_loop_operation_fmt, this->id, i, this->id * iterations);
    // logger(this->log->processes, log_loop_operation_fmt, this->id, i, this->id * iterations);
		print(buffer);
		
		/* If "--mutexl" is set, notify all about exiting critical area */
		if (critical){
			release_cs(this);
		}

    // printf("DEBUG %i: Out of CS\n", this->id);
	}

  logger(this->log->processes, log_done_fmt, get_lamport_time(), this->id, 0);

  create_message(&msg, DONE, &msg, 0);

  if (lamport_send_multicast(this, &msg) != 0) {
    printf("Fail to do multicast DONE request from process %i in clid %d\n",
           this->id, this->id);
    return 1;
  }

  while (this->done < this->num_of_processes - 2){
		manage_CS(this);
  }

  logger(this->log->processes, log_received_all_done_fmt, get_lamport_time(), this->id);

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
    logger(this->log->processes, log_received_all_started_fmt, get_lamport_time(), this->id);

  // ----------- wait for DONE messages from all -------
  if (wait_for_all(this, DONE) != 0) {
    printf("Fail to lamport_receive all DONE messages %i\n", this->id);
    return 1;
  } else
    logger(this->log->processes, log_received_all_done_fmt, get_lamport_time(), this->id);

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

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    printf("Invalid amount of arguments = %i\n", argc);
    return -1;
  }

  local_id total_N = 0;
  for (int i = 1; i < argc; i++){
    if (strcmp(argv[i], "--mutexl") == 0) {
      critical = true;
    }

    if (strcmp(argv[i], "-p") == 0 && i != argc - 1) {
      total_N = atoi(argv[i + 1]) + 1;
      if (total_N > 10 || total_N < 1) {
        printf("Num of processes has to be from 1 to 10\n");
        return -1;
      }
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
  this->done = 0;

  if (init_pipes(this))
    return -1;
  fflush(this->log->pipes);
  
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
