#include "common.h"
#include "ipc.h"
#include "pa1.h"

#include <stdlib.h>
#include <argp.h>
#include <sys/wait.h>
#include <string.h>
#include "pipes.h"

void free_process(Process* ptr){
    free_pipes(ptr->pipes, ptr->num_of_processes - 1);
    free(ptr->log);
    free(ptr);
}


void init_system(Process* this){

}

int main (int argc, const char * argv[]){
    if (argc != 3){
        printf("Invalid amount of arguments = %i", argc);
        return -1;
    }

    local_id num_of_processes;
    if (strcmp(argv[1], "-p")){
        num_of_processes = atoi(argv[3]);
        printf("num_of_processes = %i", num_of_processes);
        if(num_of_processes > 10 || num_of_processes < 1){
            printf("Num of processes has to be from 1 to 10");
            return -1;
        }
    }

    local_id total_N = num_of_processes + 1;

    // setup parent process
    Process* this = mmalloc(Process);
    this->id = 0;
    this->parent_id = -1;
    this->parent_pid = -1;
    this->pid = getpid();
    this->num_of_processes = total_N;
    this->log = mmalloc(Log);
    this->log->processes = fopen(events_log, "a");
    this->log->pipes = fopen(pipes_log, "w");
    this->pipes = mmalloc(Pipe);

    init_pipes(this);
    fclose(this->log->pipes);

    pid_t parent_pid = this->parent_pid;
    for(int i = 1; i < this->num_of_processes; i++){
        if (fork() == 0){
            this->parent_id = 0;
            this->parent_pid = parent_pid;
            this->pid = getpid();
            this->id = i;
        }
    }


    run_system();
}