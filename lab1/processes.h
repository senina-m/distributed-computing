
typedef struct{
    int fw;
    int fr;
} Pipe;

typedef struct{
    FILE* processes;
    FILE* pipes;
} Log;

typedef struct{
    local_id id;
    local_id parent_id;
    pid_t this_pid;
    pid_t parent_pid;
    int num_of_processes; 
    Pipe** pipes;
    Log* log;
} Process;