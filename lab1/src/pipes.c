
#include "pipes.h"

// #define mmalloc_array(T, count) ((T**)malloc(sizeof(T)*count))

Pipe*** alloc_pipes(int n){
    Pipe*** pipes = mmalloc_array(Pipe*, n);
    for (int i = 0; i < n; i++){
        pipes[i] = mmalloc_array(Pipe, n);
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
            if (i != j && pipe((int*)&this->pipes[i][j]) != -1){
                fcntl(this->pipes[i][j]->fr, F_SETFL, O_NONBLOCK);
                fcntl(this->pipes[i][j]->fw, F_SETFL, O_NONBLOCK);
                fprintf(this->log->pipes, "Process %i pipe %i -> %i\n", this->id, i, j);
            }else{
                printf("Can't create pipes!\n");
                return 0;
            }
        }
    }
    return 1;
}

int close_unused_pipes(Process* this){
    for (int i = 0; i < this->num_of_processes; i++) {
        for (int j = 0; j < this->num_of_processes; j++) {
            if ( i != this->id && i != j ) {
                if((close(this->pipes[i][j]->fr) == 0) && (close(this->pipes[i][j]->fw) == 0)){
                    fprintf(this->log->pipes, "Process %i closed pipes %i %i and %i %i\n", this->id, i, j, j, i);
                }else{
                    fprintf(this->log->pipes, "Process %i CAN'T close pipes %i %i and %i %i\n", this->id, i, j, j, i);
                    return 0;
                }
            }
        }
    } 
    return 1;
}