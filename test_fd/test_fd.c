#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


int main(int argc, char **argv)
{
    char name[] = "patata#";
    name[6] = *(argv[1]);
    
    int fd = creat( name, 0755 );
    
    printf( "%d  %s\n", fd, name );
    
    getchar();
    
    close( fd );
    
    return 0;
}
