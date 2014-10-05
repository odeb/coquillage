#ifndef PROCESS_ENVIRONMENT_H
#define PROCESS_ENVIRONMENT_H


struct process_environment_t
{
    char* command;
    char* args;
    int stdin_fd;
    int stdout_fd;
    struct process_environment_t* next;
};
typedef struct process_environment_t* list_process_environment_t;

//list_process_environment_t add_process_env( list_process_environment_t origin, const char* command, const char* args, const char* file_in, const char* file_out );
list_process_environment_t add_process_env( list_process_environment_t origin, const char* command, const char* args, int fdin, int fdout );
void delete_process_env_list( list_process_environment_t origin );

#endif