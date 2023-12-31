#include "common.h"
#include "ipc.h"
#include "pa1.h"

#include <stdlib.h>
#include <argp.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include "pipes.h"


#define BUF_SIZE 1024

#define logger(file, str, ...) {\
	fprintf(file, str, __VA_ARGS__);\
	printf(str, __VA_ARGS__);\
}

void free_process(Process* ptr){
    free_pipes(ptr->pipes, ptr->num_of_processes);
    free(ptr->log);
    free(ptr);
}

int wait_for_all(Process* this, MessageType t){
    int n = this->num_of_processes;
    Message msg;
    local_id id = 1; //because parent doesn't send anything
    while(id < n){
        if(id == this->id) id++;
        else{
            if(receive(this, id, &msg) == 0){
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

int run_child_rutine(Process* this){

    //START section starts here
    //print that child's process was started
    logger(this->log->processes, log_started_fmt, this->id, this->pid, this->parent_pid);

    Message msg;
    msg.s_header.s_type = STARTED;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = time(NULL);
    int msg_len = sprintf(msg.s_payload, log_started_fmt, this->id, this->pid, this->parent_pid);
    msg.s_header.s_payload_len = msg_len;

    // printf("Process %i  is going to send message \'%s\'\n", this->id, msg.s_payload);
    if (send_multicast(this, &msg) != 0){
        printf("Fail to do multicast STARTED request from process %i\n", this->id);
        return 1;
    }

    if(wait_for_all(this, STARTED) != 0){
        printf("Fail to receive all STARTED messages in child %i\n", this->id);
        return 1;
    }else logger(this->log->processes, log_received_all_started_fmt, this->id);

    //here we do our work
    logger(this->log->processes, log_done_fmt, this->id);
    // log that we've done all our work

    msg.s_header.s_type = DONE;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = time(NULL);
    msg_len = sprintf(msg.s_payload, log_done_fmt, this->id);
    msg.s_header.s_payload_len = msg_len;

    if (send_multicast(this, &msg) != 0){
        printf("Fail to do multicast DONE request from process %i\n", this->id);
        return 1;
    }

    if(wait_for_all(this, DONE) != 0){
        printf("Fail to receive all DONE messages %i\n", this->id);
        return 1;
    }else logger(this->log->processes, log_received_all_done_fmt, this->id);

    if(close_used_pipes(this) !=0){
        printf("Fail to close used pipes %i\n", this->id);
        return 1;
    }

    fclose(this->log->pipes);
    fclose(this->log->processes);
    // printf("end of process %d\n", this->id);
    free_process(this);
    exit(EXIT_SUCCESS);
}

int run_parent_rutine(Process* this){
    // logger(this->log->processes, log_started_fmt , this->id, this->pid, this->parent_pid);
    //послушать все

    if(wait_for_all(this, STARTED) != 0){
        printf("Fail to receive all STARTED messages in parent %i\n", this->id);
        return 1;
    }else logger(this->log->processes, log_received_all_started_fmt, this->id);

    // printf("parent receive all started message\n");

    if(wait_for_all(this, DONE) != 0){
        printf("Fail to receive all DONE messages %i\n", this->id);
        return 1;
    }else logger(this->log->processes, log_received_all_done_fmt, this->id);

    // printf("parent receive all done message\n");
    for (int i = 1; i < this->num_of_processes; i++) {
        if (wait(NULL) == -1) {
            printf("Fail to close %i children\n", this->num_of_processes - i);
            return 1;
        }
    }

    if(close_used_pipes(this) !=0){
        printf("Fail to close used pipes %i\n", this->id);
        return 1;
    }

    fclose(this->log->pipes);
    fclose(this->log->processes);
    free_process(this);
    return 0;
}

int main (int argc, const char * argv[]){
    if (argc != 3){
        printf("Invalid amount of arguments = %i\n", argc);
        return -1;
    }

    local_id total_N = 0;
    if (strcmp(argv[1], "-p") == 0){
        total_N = atoi(argv[2]) + 1;
        // printf("num_of_processes = %i\n", num_of_processes);
        if(total_N > 10 || total_N < 1){
            printf("Num of processes has to be from 1 to 10\n");
            return -1;
        }
    }

    // setup parent process
    Process* this = mmalloc(Process);
    this->id = 0;
    this->parent_id = -1;
    this->parent_pid = -1;
    this->pid = getpid();
    this->num_of_processes = total_N;
    this->log = mmalloc(Log);
    this->log->processes = fopen(events_log, "w");
    this->log->pipes = fopen(pipes_log, "w");
    this->pipes = alloc_pipes(total_N);

    if(init_pipes(this)) return -1;
    fflush(this->log->pipes);

    pid_t my_parent_pid = this->pid;
    for(int i = 1; i < this->num_of_processes; i++){
        if (fork() == 0){
            this->parent_id = 0;
            this->parent_pid = my_parent_pid;
            this->pid = getpid();
            this->id = i;
            if(close_unused_pipes(this)) return -1;
            run_child_rutine(this);
            exit(0);
        }
    }

    if(close_unused_pipes(this)) return -1;
    if(run_parent_rutine(this)) return -1;
}
