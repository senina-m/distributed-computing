
#include "pipes.h"


Pipe*** alloc_pipes(int n){
    Pipe*** pipes = (Pipe***)malloc(sizeof(Pipe**)*n);
    for (int i = 0; i < n; i++){
        pipes[i] = (Pipe**)malloc(sizeof(Pipe*)*n);
        for(int j = 0; j < n; j++){
            pipes[i][j] = mmalloc(Pipe);
        }
    }
    return pipes;
}

void free_pipes(Pipe*** ptr, int n){
    for (int i = 0; i < n; i++){
        free(ptr[i]);
    }
    free(ptr);
}

int init_pipes(Process* this){
    int n = this->num_of_processes;
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            if(i != j){
                printf("%i, %i\n", i, j);
                int fd[2];
                if (pipe(fd) == 0){
                    printf("%i, %i\n", i, j);
                    this->pipes[i][j]->fr = fd[0];
                    printf("%i, %i\n", i, j);
                    this->pipes[i][j]->fw = fd[1];
                    // fcntl(this->pipes[i][j]->fr, F_SETFL, O_NONBLOCK);
                    // fcntl(this->pipes[i][j]->fw, F_SETFL, O_NONBLOCK);
                    fprintf(this->log->pipes, "Process %i pipe %i -> %i\n", this->id, i, j);
                }else{
                    printf("Can't create pipes!\n");
                    return 1;
                }
            }
        }
    }
    return 0;
}

int close_unused_pipes(Process* this){
    for (int i = 0; i < this->num_of_processes; i++) {
        for (int j = 0; j < this->num_of_processes; j++) {
            if ( i != this->id && i != j ) {
                if((close(this->pipes[i][j]->fr) == 0) && (close(this->pipes[i][j]->fw) == 0)){
                    fprintf(this->log->pipes, "Process %i closed pipes %i %i and %i %i\n", this->id, i, j, j, i);
                }else{
                    fprintf(this->log->pipes, "Process %i CAN'T close pipes %i %i and %i %i\n", this->id, i, j, j, i);
                    return 1;
                }
            }
        }
    } 
    return 0;
}

int close_used_pipes(Process* this){
    for (int i = 0; i < this->num_of_processes; i++) {
        for (int j = 0; j < this->num_of_processes; j++) {
            if ( i = this->id) {
                if((close(this->pipes[i][j]->fr) == 0) && (close(this->pipes[i][j]->fw) == 0)){
                    fprintf(this->log->pipes, "Process %i closed pipes %i %i and %i %i\n", this->id, i, j, j, i);
                }else{
                    fprintf(this->log->pipes, "Process %i CAN'T close pipes %i %i and %i %i\n", this->id, i, j, j, i);
                    return 1;
                }
            }
        }
    } 
    return 0;
}