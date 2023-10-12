
#include "pipes.h"

Pipe** alloc_pipes(int n){
    Pipe** pipes = mmalloc_array(Pipe*, n);
    for (int i = 0; i < n; i++){
        pipes[i] = mmalloc(Pipe);
    }
    return pipes;
}

int init_pipes(Process* this){
    int n = this->num_of_processes - 1;
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            if (i != j && pipe((int*)&this->pipes[i][j]) != -1){
                fcntl(this->pipes[i][j].fr, F_SETFL, O_NONBLOCK);
                fcntl(this->pipes[i][j].fw, F_SETFL, O_NONBLOCK);
            }else{
                printf("Can't create pipes!");
                return -1;
            }
        }
    }
    return 1;
}

void free_pipes(Pipe** ptr, int n){
    for (int i = 0; i < n; i++){
        free(ptr[i]);
    }
    free(ptr);
}