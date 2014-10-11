#include "process_management.h"

void mask_stdout( const char* nom_fic, int* sortie_std )
{
	// on transforme un fichier en sortie standard, le temps de l'execution
	*sortie_std = dup( 1 );
	if( *sortie_std == -1 )
	{
		fprintf( stderr, "erreur dans la fonction mask_stdout\n" );
		exit( 1 );
	}
	
	if( close( 1 ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction mask_stdout\n" );
		exit( 1 );
	}
	
	if( creat( nom_fic, PMODE ) < 0 ) 
	{
		fprintf( stderr, "erreur dans la fonction mask_stdout\n" );
		exit( 1 );
	}
}

void restore_stdout( int sortie_std )
{
	// on ferme le ficher
	if( close( 1 ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdout\n" );
		exit( 1 );
	}
	
	// il trouve tout seul 1 qui est le premier dispo
	if( dup( sortie_std ) == -1 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdout\n" );
		exit( 1 );		
	}
	
	if( close( sortie_std ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdout\n" );
		exit( 1 );
	}
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
	if( *entree_std == -1 )
	{
		fprintf( stderr, "erreur dans la fonction mask_stdin\n" );
		exit( 1 );
	}
	
	if( close(0) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction mask_stdin\n" );
		exit( 1 );
	}
	
	if( open( nom_fic, 0 ) < 0 ) 
	{
		fprintf( stderr, "erreur dans la fonction mask_stdin\n" );
		exit(1);
	}
}

void restore_stdin( int entree_std )
{
	// on ferme le ficher
	if( close( 0 ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdin\n" );
		exit( 1 );
	}
	
	// il trouve tout seul 1 qui est le premier dispo
	if( dup( entree_std ) == -1 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdin\n" );
		exit( 1 );		
	}
	
	if( close( entree_std ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdin\n" );
		exit( 1 );
	}
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
			execlExit= execl( commande, commande, argument, NULL );
			
		// je sors en erreur si execl à un problème d'exécution
		if( execlExit < 0 ) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n" );
		// j'ai fini alors je m'arrête
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
	if( pipe( fp ) != 0 ) return 1;
	
	*copieEcriture = dup( 1 );
	if( *copieEcriture == -1 ) return 1;
	
	if( close( 1 ) != 0 ) return 1;
	
	*copieEcriturePipe = dup( fp[1] );
	if( *copieEcriturePipe == -1 ) return 1;
	
	if( close( fp[1] ) != 0 ) return 1;
	
	return 0;
}

int recuperationPipe( int fp[2], int* copieLecture, int* copieLecturePipe )
{
	*copieLecture = dup( 0 );
	if( *copieLecture == -1 ) return 1;
	
	if( close( 0 ) != 0 ) return 1;
	
	*copieLecturePipe = dup( fp[0] );
	if( *copieLecturePipe == -1 ) return 1;
	
	if( close( fp[0] ) != 0 ) return 1;
	
	return 0;
}

int fermerPipe( int* es, int* pipe )
{
	if( close( *pipe ) != 0 ) return 1;
	
	if( dup( *es ) == -1 ) return 1;
	
	if( close( *es ) != 0 ) return 1;
	
	return 0;
}
