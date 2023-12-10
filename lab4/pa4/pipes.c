
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
    // printf("INITING PIPES\n");
    int n = this->num_of_processes;
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            if(i != j){
                int fd[2];
                if (pipe(fd) == 0){
                    this->pipes[i][j]->fr = fd[0];
                    this->pipes[i][j]->fw = fd[1];
                    fcntl( fd[0], F_SETFL, O_NONBLOCK);
                    fcntl( fd[1], F_SETFL, O_NONBLOCK);
                    fprintf(this->log->pipes, "Process %i opens pipe %i -> %i, with descriptors r:%i, w:%i\n", this->id, i, j, fd[0], fd[1]);
                    // printf("Process %i opens pipe %i -> %i, with descriptors r:%i, w:%i\n", this->id, i, j, fd[0], fd[1]);
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
                if((close(this->pipes[i][j]->fr) == 0) && (close(this->pipes[j][i]->fw) == 0)){
                    fprintf(this->log->pipes, "Process %i closed pipes r:%i %i and w:%i %i\n", this->id, i, j, j, i);
                    // printf("Process %i closed pipes r:%i %i and w:%i %i\n", this->id, i, j, j, i);
                }else{
                    fprintf(this->log->pipes, "Process %i CAN'T close pipes r:%i %i and w:%i %i\n", this->id, i, j, j, i);
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
            if (i == this->id && i != j) {
                if((close(this->pipes[i][j]->fr) == 0) && (close(this->pipes[j][i]->fw) == 0)){
                    fprintf(this->log->pipes, "Process %i closed used pipes r:%i %i and w:%i %i\n", this->id, i, j, j, i);
                }else{
                    fprintf(this->log->pipes, "Process %i CAN'T close used pipes r:%i %i and w:%i %i\n", this->id, i, j, j, i);
                    return 1;
                }
            }
        }
    } 
    return 0;
}
