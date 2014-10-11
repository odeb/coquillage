#include "process_management.h"

void mask_stdout( const char* nom_fic, int* sortie_std )
{
	// on transforme un fichier en sortie standard, le temps de l'execution
	*sortie_std = dup( 1 );
	
	close(1);
	
	if( creat( nom_fic, PMODE ) < 0 ) 
	{
		fprintf( stderr, "erreur dans la fonction mask_stdout\n" );
		exit( 1 );
	}
}

void restore_stdout( int sortie_std )
{
	// on ferme le ficher
	close( 1 );
	
	dup( sortie_std );	// il trouve tout seul 1 qui est le premier dispo ?
	close( sortie_std );
}

int read_and_move_forward( char** string, char* buffer )
{
    const char* string_start = *string;
    while( **string != ' ' && **string != '\0' && **string != '\n' )
    {
        *buffer = **string;
        buffer++;
        (*string)++;
    }
    
    *buffer = '\0';
    if( **string == ' ' )
    {
        (*string)++;
    }
    
    return *string - string_start;
}

void mask_stdin( const char* nom_fic, int* entree_std )
{
	// on transforme un fichier en entree standard, le temps de l'execution
	*entree_std = dup( 0 );
	
	close(0);
	
	if( open( nom_fic, 0 ) < 0 ) 
	{
		fprintf( stderr, "erreur dans la fonction mask_stdin\n" );
		exit(1);
	}
}

void restore_stdin( int entree_std )
{
	// on ferme le ficher
	close( 0 );
	
	dup( entree_std );	// il trouve tout seul 1 qui est le premier dispo ?
	close( entree_std );
}

int forkNexec( char* commande, char* argument )
{
	int statusFils;								// contient le status du fils pour le wait() du père
	int processus;								// récupère le PID du processus une fois le fork() exécuté
	int execlExit;              				// récupère la valeur de sortie d'execl
	
	// fprintf( stderr, "Execution de la commande '%s' avec l'argument '%s'.\n", commande, argument );
	
	// On se divise !!
	processus = fork();
	if( processus == -1 )
	{
		fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
		return 1;
	}
	// Si je suis le fils
	if( processus == 0 )
	{
		// j'exécute la commande demandée
		if ( strcmp( argument, "" ) == 0 )
			execlExit= execl( commande, commande, NULL );
		else
		{
			//fprintf( stderr, "TITAN DEBUG: '%s'\n", argument );
			execlExit= execl( commande, commande, argument, NULL );
		}
		// je sors en erreur si execl à un problème d'exécution
		if( execlExit < 0 ) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n" );
		exit( 0 );
	}		
	// Sinon, je suis le père
	else
	{
		// Et j'attends la fin de mes fils
		processus = wait( &statusFils );
	}
	return 0;
}

int creationPipe( int fp[2], int* copieEcriture, int* copieEcriturePipe )
{
	pipe( fp );
	*copieEcriture = dup( 1 );
	close( 1 );
	*copieEcriturePipe = dup( fp[1] );
	close( fp[1] );
	
	return 0;
}

int recuperationPipe( int fp[2], int* copieLecture, int* copieLecturePipe )
{
	*copieLecture = dup( 0 );
	close( 0 );
	*copieLecturePipe = dup( fp[0] );
	close( fp[0] );
	return 0;
}

int fermerPipe( int* es, int* pipe )
{
	close( *pipe );
	dup( *es );
	close( *es );
	return 0;
}
