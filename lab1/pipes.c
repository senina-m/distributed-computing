
#include "pipes.h"

// #define mmalloc_array(T, count) ((T**)malloc(sizeof(T)*count))

Pipe** alloc_pipes(int n){
    Pipe** pipes = mmalloc_array(Pipe*, n);
    for (int i = 0; i < n; i++){
        pipes[i] = mmalloc_array(Pipe, n);
    }
    return pipes;
}

void free_pipes(Pipe** ptr, int n){
    for (int i = 0; i < n; i++){
        free(ptr[i]);
    }
    free(ptr);
}

int init_pipes(Process* this){
    int n = this->num_of_processes - 1;
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            if (i != j && pipe((int*)&this->pipes[i][j]) != -1){
                fcntl(this->pipes[i][j].fr, F_SETFL, O_NONBLOCK);
                fcntl(this->pipes[i][j].fw, F_SETFL, O_NONBLOCK);
                fprintf(this->log->pipes, "Created pipe %i -> %i\n", i, j);
            }else{
                printf("Can't create pipes!\n");
                return -1;
            }
        }
    }
    return 1;
}

int close_unused_pipes(Process* this){
    //here we need to close all pipes, which doesn't refere to current process
}