#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    int fp[2];
    pipe( fp );
    printf( "%d  %d\n", fp[0], fp[1] );
    
    return 0;
}
