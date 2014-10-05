/*
 * coquillage.c
 * 
 * Copyright 2014 Loup <loup@Vroum>
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


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "process_environment.h"


#define BUFFERSIZE 512
#define PMODE = 0777;


// signature des fonctions
void mask_stdout( const char* nom_fic, int* sortie_std );
void restore_stdout( int sortie_std );
void mask_stdin( const char* nom_fic, int* entree_std );
void restore_stdin( int entree_std );
int read_and_move_forward( char** string, char* buffer );



int main(int argc, char **argv)
{
	
	// Variables
	int nbCarLus = 0;
	char tampon[BUFFERSIZE];	// tableau qui contient la commande tapée
	int execlExit;
	int processus;
	int statusFils;
	int sortie_std = 1;
	int entree_std = 0;
	char* pointer;
    char commande[512];
    char argument[512];
    char mot[512];
    char buffer[512];
    int redirection_sortie;
	int redirection_entree;
	int redirection_sortie_entree;
	int faire_la_redirection;
	int attention_redirection_sortie;
	int	attention_redirection_entree;
	int	attention_redirection_sortie_entree;
    int fp[2];
    int fp_temporaire;
    list_process_environment_t list_origin;
    int analyseEnCours; // sert à arrêter l'analyse de la ligne de commande
    int argumentEnCours; // sert à savoir si l'on analyse un argument ou une commande/mot
	
	
	// Traitement
	
	
	printf("Bienvenue sur le coquillage ! :D\n");
	
	// Boucle d'attente des commandes
	while (nbCarLus >= 0)
	{
		
		// On débuffurise stdout afin de pouvoir afficher immédiatement la ligne d'un printf qui ne contient pas un retour chariot
		setbuf(stdout, NULL);
		// Affichage d'un prompt
		printf("Coquillage $ ");
	
	
		// Les caractères lus sont stockés dans tampon
		nbCarLus=read(0,tampon,BUFFERSIZE);
		// On sort en erreur si read à un problème de lecture
		if(nbCarLus<0) 
		{
			fprintf( stderr, "erreur de lecture\n");
			exit(1);
		}
		pointer = tampon;
		
		// Ici on remplace le retour chariot à la fin de la chaine de caractère par une fin de chaine de caractère
		tampon[nbCarLus-1]='\0';
		
		// DEBUG
		// Test d'affichage du nombre de caractères lus et du tampon
		//fprintf( stderr, "J'ai lu %d caractères, et la chaine est '%s'.\n",nbCarLus-1,tampon);

		// On traite le cas où le tampon est vide, pas de commande, alors on execute rien ! Et on recommence la boucle, du coup.
		if(!strcmp(tampon,""));
		// Sinon, on gère la demande de fermeture du shell coquillage avec la commande exit
		else if(!strcmp(tampon,"exit"))
		{
			printf("Fermeture de coquillage...\n");
			exit(0);
		}
		// Et sinon, on exécute la commande demandée !
		else
		{

			// Analyse de la ligne de commande
			analyseEnCours = 0;
			argumentEnCours = 0;
			redirection_sortie = 0;
			redirection_entree = 0;
			redirection_sortie_entree = 0;
			attention_redirection_sortie = 0;
			attention_redirection_entree = 0;
			attention_redirection_sortie_entree = 0;
			faire_la_redirection = 0;
			list_origin = malloc( sizeof( list_process_environment_t ) );
			list_origin = NULL;
			strcpy( argument, "" );
			
			while( analyseEnCours == 0 )
			{
				if( read_and_move_forward( &pointer, mot ) != 0 )
				{
					//fprintf( stderr, "DEBUG mot: '%s'.\n",mot);
					
					if( !strcmp( mot, ">" ) )
					{
						redirection_sortie = 1;
						attention_redirection_sortie = 1;
						argumentEnCours = 0;
						//fprintf( stderr, "DEBUG redirection: %s\n",mot);
						//fprintf( stderr, "DEBUG argumentEnCours: '%d'.\n",argumentEnCours);
					}
					else if ( !strcmp( mot, "<" ) )
					{
						redirection_entree = 1;
						attention_redirection_entree = 1;
						argumentEnCours = 0;
						//fprintf( stderr, "DEBUG redirection: %s\n",mot);
						//fprintf( stderr, "DEBUG argumentEnCours: '%d'.\n",argumentEnCours);
					}
					else if( !strcmp( mot, "|" ) )
					{
						redirection_sortie_entree = 1;
						attention_redirection_sortie_entree = 1;
						argumentEnCours = 0;
						//fprintf( stderr, "DEBUG redirection : %s\n",mot);
						//fprintf( stderr, "DEBUG argumentEnCours: '%d'.\n",argumentEnCours);
					}
					else
					{
						if( argumentEnCours == 1 )
						{
							//fprintf( stderr, "DEBUG argument: '%s'.\n",mot);
							strcpy( argument, mot );
						}
						else if( redirection_sortie == 1 )
						{
							redirection_sortie = 0;
							mask_stdout( mot, &sortie_std );
							//fprintf( stderr, "DEBUG fichier sortie: '%s'.\n",mot);
						}
						else if( redirection_entree == 1 )
						{
							redirection_entree = 0;
							if ( attention_redirection_sortie_entree == 1 )
							{
								fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '<'.\n");
							}
							if ( attention_redirection_sortie_entree == 0 )
								mask_stdin( mot, &entree_std );
							else
								mask_stdin( mot, &fp[0] );
							//fprintf( stderr, "DEBUG fichier entrée: '%s'.\n",mot);
						}
						else if( redirection_sortie_entree == 1 )
						{
							redirection_sortie_entree = 0;
							if ( attention_redirection_sortie == 1 )
							{
								fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '>'.\n");
							}
							if( faire_la_redirection == 0 )
							{
								pipe( fp );
								if( attention_redirection_sortie == 0 )
									list_origin = add_process_env( list_origin, commande, argument, entree_std, fp[1] );
								else
									list_origin = add_process_env( list_origin, commande, argument, entree_std, sortie_std );
							}
							else
							{
								fp_temporaire = fp[0];
								pipe( fp );
								if( attention_redirection_sortie == 0 )
									list_origin = add_process_env( list_origin, commande, argument, fp_temporaire, fp[1] );
								else
									list_origin = add_process_env( list_origin, commande, argument, fp_temporaire, sortie_std );
							}
							strcpy( commande, mot );
							fprintf( stderr, "DEBUG processus à lancer: '%s'.\n",commande);
							faire_la_redirection = 1;
							attention_redirection_sortie = 0;
						}
						else
						{
							strcpy( commande, mot );
							fprintf( stderr, "DEBUG processus à lancer: '%s'.\n",commande);
						}
						argumentEnCours = 1;
						//fprintf( stderr, "DEBUG argumentEnCours: '%d'.\n",argumentEnCours);
					}					
				}
				else
				{
					analyseEnCours = 1;
					if( faire_la_redirection == 0 )
					{
						//fprintf( stderr, "TEST1.\n");
						//fprintf( stderr, "DEBUG commande: '%s'.\n", commande );
						//fprintf( stderr, "DEBUG argument: '%s'.\n", argument );
						//fprintf( stderr, "DEBUG stdin: '%d'.\n", entree_std );
						//fprintf( stderr, "DEBUG stdout: '%d'.\n", sortie_std );
						//fprintf( stderr, "DEBUG sortie_std: '%d'.\n", sortie_std );
						list_origin = add_process_env( list_origin, commande, argument, entree_std, sortie_std );
						//fprintf( stderr, "TEST1.\n");
					}
					else
					{
						//fprintf( stderr, "DEBUG attention_redirection_sortie: '%d'.\n", attention_redirection_sortie );
						//fprintf( stderr, "DEBUG sortie_std: '%d'.\n", sortie_std );
						//fprintf( stderr, "DEBUG fp[1]: '%d'.\n", fp[1] );
						if( attention_redirection_sortie == 0 )
						{
							//fprintf( stderr, "DEBUG if.\n");
							list_origin = add_process_env( list_origin, commande, argument, fp[0], 1 );
						}
						else
						{
							//fprintf( stderr, "DEBUG else.\n");
							list_origin = add_process_env( list_origin, commande, argument, fp[0], sortie_std );
						}
						//faire_la_redirection = 0;
					}
				}
			}
			fprintf( stderr, "TEST liste niveau 1.\n");
			fprintf( stderr, "DEBUG command: '%s'.\n", list_origin->command );
			fprintf( stderr, "DEBUG args: '%s'.\n", list_origin->args );
			fprintf( stderr, "DEBUG stdin_fd: '%d'.\n", list_origin->stdin_fd );
			fprintf( stderr, "DEBUG stdout_fd: '%d'.\n", list_origin->stdout_fd );
			fprintf( stderr, "DEBUG next: '%p'.\n", list_origin->next );
			
			fprintf( stderr, "TEST liste niveau 2.\n");
			fprintf( stderr, "DEBUG command: '%s'.\n", (list_origin->next)->command );
			fprintf( stderr, "DEBUG args: '%s'.\n", (list_origin->next)->args );
			fprintf( stderr, "DEBUG stdin_fd: '%d'.\n", (list_origin->next)->stdin_fd );
			fprintf( stderr, "DEBUG stdout_fd: '%d'.\n", (list_origin->next)->stdout_fd );
			fprintf( stderr, "DEBUG next: '%p'.\n", (list_origin->next)->next );
			
			//restore_stdout( sortie_std );
			//restore_stdin( entree_std );
			
			exit(0);
			
			
			
			// AVEC FORK
			// On se prépare à exécuter la commande demandée dans un processus fils !
			processus = fork();
			if(processus == -1)
				fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n");
			// Si je suis le fils
			if(processus == 0)
			{
	
				
				// j'exécute la commande demandée
				execlExit=execl(commande,commande,NULL);
				// je sors en erreur si execl à un problème d'exécution
				if(execlExit<0) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n");
				exit(0);
			}		
			// Sinon, je suis le père
			else
			{
				// Et j'attends la fin de mes fils
				processus=wait(&statusFils);
			}
			
			if ( redirection_sortie == 1 )
			{
				restore_stdout( sortie_std );
			}	
			if ( redirection_entree == 1 )
			{
				restore_stdin( entree_std );
			}
			//printf( "tigre\n" );

		}
		
	    redirection_sortie = 0;
	    redirection_entree = 0;
	    
	    //delete_process_env_list( list_origin );
	    //list_origin = NULL;
	}

	return 0;
}

void mask_stdout( const char* nom_fic, int* sortie_std )
{
	// DEBUG
	//fprintf( stderr, "passé par la fonction mask_stdout\n");
	
	// on transforme un fichier en sortie standard, le temps de l'execution
	
	*sortie_std = dup(1);
	
	//fprintf( stderr, "DEBUG mask_stdout: '%d'.\n", *sortie_std);
	
	close(1);
	
	if( creat( nom_fic, 0777 ) < 0 ) 
	{
		fprintf( stderr, "erreur dans la fonction mask_stdout\n");
		exit(1);
	}
}

void restore_stdout( int sortie_std )
{
	// on remet en place la sortie standard
	fprintf( stderr, "passé par la fonction restore_stdout\n");
	
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
	// DEBUG
	//fprintf( stderr, "passé par la fonction mask_stdin\n");
	
	// on transforme un fichier en entree standard, le temps de l'execution
	
	*entree_std = dup(0);
	
	//fprintf( stderr, "DEBUG mask_stdin: '%d'.\n", *entree_std);
	
	close(0);
	
	if( open( nom_fic, 0 ) < 0 ) 
	{
		fprintf( stderr, "erreur dans la fonction mask_stdin\n");
		exit(1);
	}
}

void restore_stdin( int entree_std )
{
	// on remet en place la sortie standard
	fprintf( stderr, "passé par la fonction restore_stdin\n");
	
	// on ferme le ficher
	close( 0 );
	
	dup( entree_std );	// il trouve tout seul 1 qui est le premier dispo ?
	close( entree_std );
}
