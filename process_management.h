#ifndef _PROCESS_MANAGEMENT
#define _PROCESS_MANAGEMENT

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PMODE 0664		/* permet d'affecter des droits aux fichiers créés */


// signature des fonctions
void mask_stdout( const char* nom_fic, int* sortie_std );
void restore_stdout( int sortie_std );
void mask_stdin( const char* nom_fic, int* entree_std );
void restore_stdin( int entree_std );
int read_and_move_forward( char** string, char* buffer );
int forkNexec( char* commande, char* argument );
int creationPipe( int fp[2], int* copieEcriture, int* copieEcriturePipe );
int recuperationPipe( int fp[2], int* copieLecture, int* copieLecturePipe );
int fermerPipe( int* es, int* pipe );

#endif
