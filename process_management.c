/*
 * process_management.c
 * 
 * Copyright 2014 Porté Marie-Luc and Debenath-Redondo Olmo
 * 
 * 
 * IN321 : Systèmes d'exploitation
 * BE mini-shell
 * Ce programme contient toutes les fonctions définies par 
 * process_management.h destinées à la gestion des processus utilisés 
 * par le mini-shell Coquillage.
 * 
 * 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include "process_management.h"

/* Cette fonction a pour but d'échanger la sortie standard
 * avec un descripteur de fichier de sortie pour écrire dedans.
 * La fonction crée (ou écrase) le fichier en question. */
void mask_stdout( const char* nom_fic, int* sortie_std )
{
	/* On duplique la sortie standard. */
	*sortie_std = dup( 1 );
	/* On vérifie que la duplicaton s'est bien passée. */
	if( *sortie_std == -1 )
	{
		fprintf( stderr, "erreur dans la fonction mask_stdout\n" );
		exit( 1 );
	}
	
	/* On referme la sortie standard libérant ainsi la place pour offrir
	 * celle-ci à la future création de fichier. */
	if( close( 1 ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction mask_stdout\n" );
		exit( 1 );
	}
	
	/* On crée le fichier.
	 * Le descripteur de fichier est celui de la sortie standard. */
	if( creat( nom_fic, PMODE ) < 0 ) 
	{
		fprintf( stderr, "erreur dans la fonction mask_stdout\n" );
		exit( 1 );
	}
}


/* Cette fonction a pour but de restaurer la sortie standard
 * qui a été échangée avec un fichier de sortie grâce à la 
 * fonction précédente. */
void restore_stdout( int sortie_std )
{
	/* On referme la sortie standard qui correspond actuellement
	 * au descripteur de fichier. */
	if( close( 1 ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdout\n" );
		exit( 1 );
	}
	
	/* On duplique la sauvegarde de la sortie standard afin que
	 * celle-ci puisse reprendre sa place. */
	if( dup( sortie_std ) == -1 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdout\n" );
		exit( 1 );		
	}
	
	/* On referme la sauvegarde de la sortie standard car elle n'est
	 * plus utile. */	
	if( close( sortie_std ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdout\n" );
		exit( 1 );
	}
}


/* Cette fonction a pour but de lire un mot séparé par un espace, ou terminé par
 * une fin de chaine de caractère ou terminé par un retour chariot
 * puis se déplacer au suivant pour continuer au prochain appel. */
int read_and_move_forward( char** string, char* buffer )
{
	/* On récupère un pointeur sur la longue chaine de caractère étudiée. */
    const char* string_start = *string;
    /* Tant que le caractère n'est pas un espace, une fin de chaine de caractère
     * ou un retour chariot. */
    while( **string != ' ' && **string != '\0' && **string != '\n' )
    {
		/* On récupère le mot dans buffer, caractère par caractère. */
        *buffer = **string;
        buffer++;
        (*string)++;
    }
    /* On termine la chaine de caractères contenant le mot par une fin de chaine de caractères. */
    *buffer = '\0';
    /* On saute l'espace suivant le mot s'il s'agit bien d'un espace. */
    if( **string == ' ' )
    {
        (*string)++;
    }
    
    /* On retourne le nombre de caractère du mot identifié. */
    return *string - string_start;
}


/* Cette fonction a pour but d'échanger l'entrée standard
 * avec un descripteur de fichier d'entrée pour lire son contenu.
 * La fonction ouvre le fichier en question. */
void mask_stdin( const char* nom_fic, int* entree_std )
{
	/* On duplique l'entrée standard. */
	*entree_std = dup( 0 );
	/* On vérifie que la duplicaton s'est bien passée. */
	if( *entree_std == -1 )
	{
		fprintf( stderr, "erreur dans la fonction mask_stdin\n" );
		exit( 1 );
	}
	/* On referme l'entrée standard libérant ainsi la place pour offrir
	 * celle-ci à la future ouverture de fichier. */
	if( close(0) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction mask_stdin\n" );
		exit( 1 );
	}
	/* On ouvre le fichier.
	 * Le descripteur de fichier est celui de l'entrée standard. */	
	if( open( nom_fic, 0 ) < 0 ) 
	{
		fprintf( stderr, "erreur dans la fonction mask_stdin\n" );
		exit(1);
	}
}

/* Cette fonction a pour but de restaurer l'entrée standard
 * qui a été échangée avec un fichier d'entrée grâce à la 
 * fonction précédente. */
void restore_stdin( int entree_std )
{
	/* On referme l'entrée standard qui correspond actuellement
	 * au descripteur de fichier. */
	if( close( 0 ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdin\n" );
		exit( 1 );
	}
	
	/* On duplique la sauvegarde de l'entrée standard afin que
	 * celle-ci puisse reprendre sa place. */
	if( dup( entree_std ) == -1 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdin\n" );
		exit( 1 );		
	}
	/* On referme la sauvegarde de l'entrée standard car elle n'est
	 * plus utile. */	
	if( close( entree_std ) != 0 )
	{
		fprintf( stderr, "erreur dans la fonction restore_stdin\n" );
		exit( 1 );
	}
}


/* Cette fonction a pour but de diviser le programme en cours
 * afin de générer un processus fils qui sera chargé d'exécuter
 * la commande avec son argument éventuel. */
int forkNexec( char* commande, char* argument )
{
	int statusFils;	/* contiendra le status du processus fils pour que le wait() lancé par le père débloque ce dernier qui attendait la fin de l'exécution de son fils. */
	int processus;	/* contiendra le PID du processus fils côté père et 0 côté fils, une fois le fork() exécuté */
	int execlExit;  /* contiendra la valeur de sortie de execl() */

	/* On divise le processus en deux, un père et un fils */
	processus = fork();
	/* On vérifie si cela s'est bien passé. */
	if( processus == -1 )
	{
		fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
		/* La fonction retourne 1 car elle n'a pas pu accomplir sa tâche. */
		return 1;
	}
	/* Cas du processus fils. */
	if( processus == 0 )
	{
		/* Exécution de la commande sans argument si la variable argument
		 * contient une chaine vide */
		if ( strcmp( argument, "" ) == 0 )
			execlExit= execl( commande, commande, NULL );
		/* Exécution de la commande avec argument si la variable argument
		 * en contient un. */
		else
			execlExit= execl( commande, commande, argument, NULL );
			
		/* Si l'exécution rate, alors la commande est inconnue. */
		if( execlExit < 0 ) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n" );
		
		/* Le processus fils ayant fini son travail, il se termine. */
		exit( 0 );
	}		
	/* Cas du processus père. */
	else
	{
		/* Il va attendre simplement la fin de l'exécution de son fils. */
		processus = wait( &statusFils );
	}
	
	/* La fonction retourne 0 car tout s'est bien passé. */
	return 0;
}


/* Cette fonction a pour but de créer un pipe, et d'échanger la sortie standard
 * avec le descripteur d'écriture du pipe. */
int creationPipe( int fp[2], int* copieEcriture, int* copieEcriturePipe )
{
	/* On crée les descripteurs d'écriture et de lecture du pipe. */
	if( pipe( fp ) != 0 ) return 1;
	
	/* On duplique la sortie standard. */
	*copieEcriture = dup( 1 );
	/* On vérifie que la duplicaton s'est bien passée. */
	if( *copieEcriture == -1 ) return 1;
	
	/* On referme la sortie standard libérant ainsi la place pour offrir
	 * celle-ci à au futur descripteur d'écriture du pipe. */	
	if( close( 1 ) != 0 ) return 1;
	
	
	/* On duplique le descripteur d'écriture du pipe.
	 * Ce dernier va prendre la place de la sortie standard. */
	*copieEcriturePipe = dup( fp[1] );
	/* On vérifie que la duplicaton s'est bien passée. */
	if( *copieEcriturePipe == -1 ) return 1;
	
	/* On referme le descripteur d'écriture du pipe car il n'est
	 * plus utile. */
	if( close( fp[1] ) != 0 ) return 1;
	
	/* La fonction retourne 0 car tout s'est bien passé.
	 * Sinon elle aurait retourné 1 dans tous les autres cas où
	 * un pipe, un dup ou un close aurait raté. */
	return 0;
}


/* Cette fonction a pour but de récupérer un pipe et donc échanger l'entrée standard
 * avec le descripteur de lecture du pipe créé par la fonction précédente. */
int recuperationPipe( int fp[2], int* copieLecture, int* copieLecturePipe )
{
	/* On duplique l'entrée standard. */
	*copieLecture = dup( 0 );
	/* On vérifie que la duplicaton s'est bien passée. */
	if( *copieLecture == -1 ) return 1;
	
	/* On referme l'entrée standard libérant ainsi la place pour offrir
	 * celle-ci à au futur descripteur de lecture du pipe. */
	if( close( 0 ) != 0 ) return 1;
	
	/* On duplique le descripteur de lecture du pipe.
	 * Ce dernier va prendre la place de l'entrée standard. */
	*copieLecturePipe = dup( fp[0] );
	/* On vérifie que la duplicaton s'est bien passée. */
	if( *copieLecturePipe == -1 ) return 1;
	
	/* On referme le descripteur de lecture du pipe car il n'est
	 * plus utile. */
	if( close( fp[0] ) != 0 ) return 1;
	
	/* La fonction retourne 0 car tout s'est bien passé.
	 * Sinon elle aurait retourné 1 dans tous les autres cas où
	 * un un dup ou un close aurait raté. */	
	return 0;
}


/* Cette fonction a pour but de rétablir n'importe quelle entrée ou sortie
 * depuis un pipe précédemment créé. */
int fermerPipe( int* es, int* pipe )
{
	/* On referme le pipe de lecture ou d'écriture. */
	if( close( *pipe ) != 0 ) return 1;
	
	/* On duplique l'entrée ou la sortie précédemment sauvegardée.
	 * Elles reprennent donc leur place. */
	if( dup( *es ) == -1 ) return 1;
	
	/* On referme le descripteur de l'entrée ou la sortie précédemment
	 * sauvegardée car il n'est plus utiles. */
	if( close( *es ) != 0 ) return 1;
	
	/* La fonction retourne 0 car tout s'est bien passé.
	 * Sinon elle aurait retourné 1 dans tous les autres cas où
	 * un dup ou un close aurait raté. */
	return 0;
}
