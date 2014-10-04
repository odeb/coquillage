#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "process_environment.h"

static const char* PMODE = "0755";

char* copy_string( const char* str )
{
    if( str == NULL )
    {
        return NULL;
    }
    int length = strlen( str );
    char* copy = malloc( length + 1 );
    if( copy == NULL )
    {
        exit(1);
    }
    strcpy( copy, str );
    return copy;
}

list_process_environment_t get_previous( list_process_environment_t origin )
{
    list_process_environment_t current = origin;
    while( current->next != NULL )
    {
        current = current->next;
    }
    return current;
}

/*list_process_environment_t add_process_env( list_process_environment_t origin, const char* command, const char* args, const char* file_in, const char* file_out )
{
    list_process_environment_t pro_env = malloc( sizeof( list_process_environment_t ) );
    if( list_process_environment_t == NULL )
    {
        exit(1);
    }
    pro_env->command = copy_string( command );
    pro_env->args = copy_string( args );
    if( file_in != NULL )
    {
        pro_env->stdin_fd = open( file_in, 0 );
    }
    else
    {
        pro_env->stdin_fd = 0;
    }
    if( file_out != NULL )
    {
        pro_env->stdout_fd = creat( file_out, PMODE );
    }
    else
    {
        pro_env->stdout_fd = 1;
    }
    pro_env->next = NULL;
    if( origin == NULL )
    {
        return pro_env;
    }
    else
    {
        get_previous( origin )->next = pro_env;
    }
}*/

list_process_environment_t add_process_env( list_process_environment_t origin, const char* command, const char* args, int fdin, int fdout )
{
    list_process_environment_t pro_env = malloc( sizeof( list_process_environment_t ) );
    if( pro_env == NULL )
    {
        exit(1);
    }
    pro_env->command = copy_string( command );
    pro_env->args = copy_string( args );
    pro_env->stdin_fd = fdin;
    pro_env->stdout_fd = fdout;
    pro_env->next = NULL;
    
    if( origin == NULL )
    {
        return pro_env;
    }
    else
    {
        get_previous( origin )->next = pro_env;
    }
    
    return pro_env;
}

void delete_process_env_list( list_process_environment_t origin )
{
    list_process_environment_t current = origin;
    list_process_environment_t next;
    while( current != NULL )
    {
        if( current->command != NULL )
        {
            free( current->command );
        }
        if( current->args != NULL )
        {
            free( current->args );
        }
        
        next = current->next;
        free( current );
        current = next;
    }
    
}





