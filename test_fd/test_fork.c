#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


int main(int argc, char **argv)
{
    int pid;
    int statusFils;
    
    pid = fork();
    
    if( pid < 0 )
    {
        fprintf( stderr, "Erreur\n" );
    }
    else if( pid == 0 )
    {
        // Child
        close(1);
        printf( "Pate\n" );
        fprintf( stderr, "STDERR : Pate\n" );
        exit(0);
    }
    else
    {
        // Parent
        wait(&statusFils);
    }
    
    printf( "Patata\n" );
    fprintf( stderr, "STDERR : Patata\n" );
    
    return 0;
}
