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
	
	// VARIABLES
	int nbCarLus = 0;							// permet de connaitre le nombre de caractères lus
	char tampon[BUFFERSIZE];					// tableau qui contient la commande tapée
	int execlExit;              				// récupère la valeur de sortie d'execl
	int processus;								// récupère le PID du processus une fois le fork() exécuté
	int statusFils;								// contient le status du fils pour le wait() du père
	int sortie_std = 1;							// initialisation de la sortie standard à 1
	int entree_std = 0;							// initialisation de l'entrée standard à 0
	char* pointer;								// pointeur sur le tampon afin de travailler avec par la suite
    char mot[512];								// récupère le "mot" (commande, argument, ">", "<", "|") qui va être analysé
    char commande[512];							// récupère la commande analysée
    char argument[512];							// récupère l'argument de la commande analysée
    //char buffer[512];
    int redirection_sortie;						// prévient l'analyseur qu'au prochain "mot" il faudra traiter une redirection de la sortie de la "commande" vers le fichier "mot"
	int redirection_entree;						// prévient l'analyseur qu'au prochain "mot" il faudra traiter une redirection de l'entrée de la "commande" vers le fichier "mot"
	int redirection_sortie_entree;				// prévient l'analyseur qu'au prochain "mot" il faudra traiter une redirection de la sortie de la "commande" précédente vers le prochain "mot"
	int faire_la_redirection;					// prévient l'analyseur qu'une fois qu'il a détecté un "|" alors à partir de ce moment il faudra traiter différement l'ajout de la commande à la structure, en particulier son entrée qui ne sera plus la standard
	int attention_redirection_sortie;			// prévient l'analyseur qu'il y a eu un ">" avant un "|" et qu'il devra prendre en compte en priorité le ">"
	int	attention_redirection_entree;			// prévient l'analyseur qu'il y a eu un "|" avant un "<" et qu'il devra prendre en compte en priorité le "<"
	//int	attention_redirection_sortie_entree;	// prévient l'analyseur qu'il y a eu un "|" avant un "<" et qu'il devra prendre en compte en priorité le "<"
	int restaurerSortie;						// prévient l'analyseur qu'il faut restaurer la sortie standard
	int restaurerEntree;						// prévient l'analyseur qu'il faut restaurer l'entrée standard
    int fp[2];									// contient les 2 descripteurs pour faire des "|"
    //int fp_temporaire;							// contient le descripteur d'entrée temporaire avant d'appeler de nouveau pipe() afin de bien gérer le "|"
    int copieEcriture;							// contient la copie temporaire de la sortie standard
    int copieEcriturePipe;						// contient la copie temporaire du côté écriture du pipe
    int copieLecture;							// contient la copie temporaire de l'entrée standard
    int copieLecturePipe;						// contient la copie temporaire du côté lecture du pipe
    list_process_environment_t list_origin;		// contient la liste de toutes les commandes à lancer ainsi que leur argument, leur entrée et leur sortie une fois la ligne de commandes analysée
    int analyseEnCours; 						// prévient l'analyseur d'arrêter l'analyse de la ligne de commandes
    int argumentEnCours;						// prévient l'analyseur que l'on analyse un argument
	
	
	// TRAITEMENT
	
	
	// premier affichage lorsqu'on ouvre le mini-shell
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
		// on copie le pointeur vers le tampon afin de ne pas "décaler" le pointeur original
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
			
			// on initialise tous les avertisseurs à zéro, car on n'a encore rien fait
			analyseEnCours = 0;
			argumentEnCours = 0;
			redirection_sortie = 0;
			redirection_entree = 0;
			redirection_sortie_entree = 0;
			attention_redirection_sortie = 0;
			attention_redirection_entree = 0;
			//attention_redirection_sortie_entree = 0;
			faire_la_redirection = 0;
			restaurerSortie = 0;
			restaurerEntree = 0;
			fp[0] = 0;
			fp[1] = 0;
			// on crée une structure qui contiendra la commande et ses caractéristiques (arguments, fichier d'entrée, fichier de sortie)
			list_origin = malloc( sizeof( list_process_environment_t ) );
			list_origin = NULL;
			// on initialise les arguments à une chaine vide, au cas où il n'y en aurait pas
			// TODO vérifier que ça a encore du sens quand on appelera execl
			strcpy( argument, "" );
			
			// dans un premier temps, on analyse juste ce qui a été écrit
			// ensuite, on remplit la structure list_process_environment_t avec les mots analysés, chacun dans son rôle : commande, argument, fichier d'entrée ou fichier de sortie
			// TODO ce commentaire sera peut-être caduque/incomplet si on met les forks et execs dans ce while
			while( analyseEnCours == 0 )
			{
				// on sort de ce if en arrivant à la fin de la commande tapée par l'utilisateur
				// la fonction read_and_move_forward enregistre un mot (séparé par une espace) dans "mot" et fait avancer le pointeur jusqu'au prochain mot
				if( read_and_move_forward( &pointer, mot ) != 0 )
				{
					//fprintf( stderr, "DEBUG mot: '%s'.\n",mot);
					
					// cas où l'utilisateur demande une redirection de la sortie vers un fichier
					if( !strcmp( mot, ">" ) )
					{
						// on retient que le prochain mot sera le fichier vers où rediriger
						redirection_sortie = 1;
						attention_redirection_sortie = 1;
						// on prévient également qu'on en a fini des éventuels arguments de la commande
						argumentEnCours = 0;
						//fprintf( stderr, "DEBUG redirection: %s\n",mot);
						//fprintf( stderr, "DEBUG argumentEnCours: '%d'.\n",argumentEnCours);
					}
					
					// cas où l'utilisateur demande une redirection de l'entrée vers un fichier
					else if ( !strcmp( mot, "<" ) )
					{
						// on retient que le prochain mot sera le fichier vers où rediriger
						redirection_entree = 1;
						attention_redirection_entree = 1;
						//attention_redirection_entree = 1;
						// on prévient également qu'on en a fini des éventuels arguments de la commande
						argumentEnCours = 0;
						//fprintf( stderr, "DEBUG redirection: %s\n",mot);
						//fprintf( stderr, "DEBUG argumentEnCours: '%d'.\n",argumentEnCours);
					}
					
					// cas où l'utilisateur demande une redirection de la sortie d'une commande vers l'entrée d'une autre
					else if( !strcmp( mot, "|" ) )
					{
						// on retient que le prochain mot sera la deuxième commande de la paire qui s'échange les infos
						redirection_sortie_entree = 1;
						//attention_redirection_sortie_entree = 1;
						// on prévient également qu'on en a fini des éventuels arguments de la première commande de la paire
						argumentEnCours = 0;
						//fprintf( stderr, "DEBUG redirection : %s\n",mot);
						//fprintf( stderr, "DEBUG argumentEnCours: '%d'.\n",argumentEnCours);
					}
					
					// cas où le mot lu n'annonce pas une redirection des flux de données
					else
					{
						// si le dernier mot lu était une commande ou un argument, le suivant est forcément un argument
						// TODO gestion de plusieurs arguments ?
						if( argumentEnCours == 1 )
						{
							//fprintf( stderr, "DEBUG argument: '%s'.\n",mot);
							// on copie le mot dans une variable intermédiaire, pour remplir plus tard une structure pour la commande courante
							strcpy( argument, mot );
						}
						
						// si le dernier mot lu était ">", le mot suivant est le nom du fichier dans lequel rediriger la sortie
						else if( redirection_sortie == 1 )
						{
							// on remet cet avertisseur à 0, car la redirection est traitée, on passe à autre chose
							redirection_sortie = 0;
							// on utilise la fonction qui redirige la sortie
							// (on lui passe "mot" qui contient le nom du fichier à créer 
							// et "sortie_std" que la fonction remplira avec le descripteur de fichier associé)
							mask_stdout( mot, &sortie_std );
							//fprintf( stderr, "DEBUG fichier sortie: '%s'.\n",mot);
							restaurerSortie = 1;
						}
						
						// si le dernier mot lu était "<", le mot suivant est le nom du fichier vers lequel rediriger l'entrée
						else if( redirection_entree == 1 )
						{
							// on remet cet avertisseur à 0, car la redirection est traitée, on passe à autre chose
							redirection_entree = 0;

							// si aucun "|" n'a été appelé avant ce "<", tout va bien
							// on utilise la fonction qui redirige l'entrée
							// (on lui passe "mot" qui contient le nom du fichier à créer 
							// et "sortie_std" que la fonction remplira avec le descripteur de fichier associé)
							//if ( attention_redirection_sortie_entree == 0 )
							//{
								// on utilise la fonction qui redirige l'entrée
								// (on lui passe "mot" qui contient le nom du fichier à créer 
								// et "entree_std" que la fonction remplira avec le descripteur de fichier associé)
								mask_stdin( mot, &entree_std );
								restaurerEntree = 1;
							//}
							// cas où l'utilisateur redirige deux fois l'entrée d'une commande, une fois avec "|" et l'autre avec "<"
							// par exemple : cmd1 | cmd2 < fichier
							// dans ce cas, on ignore le pipe, on utilise le fichier spécifié (on masque fp[0] par le descripteur du fichier)
							// mais surtout, on prévient l'utilisateur
							//else
							//{
								//fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '<' (%d).\n",fp[0]);
								
								//mask_stdin( mot, &fp[0] );
								//mask_stdin( mot, &entree_std );
								//entree_std=1;
								//mask_stdin( mot, &entree_std );
								//restaurerEntree = 1;
							//}
							//fprintf( stderr, "DEBUG fichier entrée: '%s'.\n",mot);
						}
						else if( redirection_sortie_entree == 1 )
						{
							redirection_sortie_entree = 0;
							
							if( faire_la_redirection == 0 )
							{

								if( attention_redirection_sortie == 0 )
								{	
									pipe( fp );
									copieEcriture = dup( 1 );
									close( 1 );
									copieEcriturePipe = dup( fp[1] );
									fprintf( stderr, "MEGADEBUG copieEcriturePipe: '%d'.\n", copieEcriturePipe);
									close( fp[1] );
										
									list_origin = add_process_env( list_origin, commande, argument, entree_std, fp[1] );
									
									// On se divise !!
									processus = fork();
									if(processus == -1)
										fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
									// Si je suis le fils
									if(processus == 0)
									{
										// j'exécute la commande demandée
										if ( strcmp(argument,"") == 0 )
											execlExit= execl( commande, commande, NULL );
										else
										{
											fprintf( stderr, "TITAN DEBUG: '%s'\n", argument );
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
										processus=wait(&statusFils);
																		
										if ( restaurerSortie == 1 )
										{
											restore_stdout( sortie_std );
											restaurerSortie = 0;
										}
										if ( restaurerEntree == 1 )
										{
											restore_stdin( entree_std );
											restaurerEntree = 0;
										}

										close ( copieEcriturePipe );
										dup( copieEcriture );
										close( copieEcriture );
									}
								}
								else
								{
									fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '>'.\n");
									list_origin = add_process_env( list_origin, commande, argument, entree_std, sortie_std );
									
									// On se divise !!
									processus = fork();
									if(processus == -1)
										fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
									// Si je suis le fils
									if(processus == 0)
									{
										// j'exécute la commande demandée
										if ( strcmp(argument,"") == 0 )
											execlExit= execl( commande, commande, NULL );
										else
										{
											fprintf( stderr, "TITAN DEBUG: '%s'\n", argument );
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
										processus=wait(&statusFils);
																		
										if ( restaurerSortie == 1 )
										{
											restore_stdout( sortie_std );
											restaurerSortie = 0;
										}
										if ( restaurerEntree == 1 )
										{
											restore_stdin( entree_std );
											restaurerEntree = 0;
										}

										//close ( copieEcriturePipe );
										//dup( copieEcriture );
										//close( copieEcriture );
									}
								}
							}
							else
							{
								//~ fp_temporaire = fp[0];
								//~ pipe( fp );
								
								//~ copieLecture = dup( 0 );
								//~ close( 0 );
								//~ copieLecturePipe = dup( fp[0]);
								
								
								if( attention_redirection_sortie == 0 )
								{
									//list_origin = add_process_env( list_origin, commande, argument, fp_temporaire, fp[1] );

									copieLecture = dup( 0 );
									close( 0 );
									copieLecturePipe = dup( fp[0]);
									fprintf( stderr, "DEBUG if '%d'.\n",copieLecturePipe);
									close( fp[0] );
									
									pipe( fp );
									copieEcriture = dup( 1 );
									close( 1 );
									copieEcriturePipe = dup( fp[1] );
									fprintf( stderr, "MEGADEBUG copieEcriturePipe: '%d'.\n", copieEcriturePipe);
									close( fp[1] );
									
									list_origin = add_process_env( list_origin, commande, argument, fp[0], 1 );
									// On se divise !!
									processus = fork();
									if(processus == -1)
										fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
									// Si je suis le fils
									if(processus == 0)
									{
										// j'exécute la commande demandée
										if ( strcmp(argument,"") == 0 )
											execlExit= execl( commande, commande, NULL );
										else
											execlExit= execl( commande, commande, argument, NULL );
										// je sors en erreur si execl à un problème d'exécution
										if( execlExit < 0 ) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n" );
										exit( 0 );
									}		
									// Sinon, je suis le père
									else
									{
										// Et j'attends la fin de mes fils
										processus=wait(&statusFils);
																	
										if ( restaurerSortie == 1 )
										{
											restore_stdout( sortie_std );
											restaurerSortie = 0;
										}
										if ( restaurerEntree == 1 )
										{
											restore_stdin( entree_std );
											restaurerEntree = 0;
										}
										
										close ( copieLecturePipe );
										dup( copieLecture );
										close( copieLecture );
										
										close ( copieEcriturePipe );
										dup( copieEcriture );
										close( copieEcriture );
									}									
								}
								else
								{
									fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '>'.\n");
									//list_origin = add_process_env( list_origin, commande, argument, fp_temporaire, sortie_std );
									
									copieLecture = dup( 0 );
									close( 0 );
									copieLecturePipe = dup( fp[0]);
									fprintf( stderr, "DEBUG if '%d'.\n",copieLecturePipe);
									close( fp[0] );
									
									// On se divise !!
									processus = fork();
									if(processus == -1)
										fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
									// Si je suis le fils
									if(processus == 0)
									{
										// j'exécute la commande demandée
										if ( strcmp(argument,"") == 0 )
											execlExit= execl( commande, commande, NULL );
										else
											execlExit= execl( commande, commande, argument, NULL );
										// je sors en erreur si execl à un problème d'exécution
										if( execlExit < 0 ) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n" );
										exit( 0 );
									}		
									// Sinon, je suis le père
									else
									{
										// Et j'attends la fin de mes fils
										processus=wait(&statusFils);
																	
										if ( restaurerSortie == 1 )
										{
											restore_stdout( sortie_std );
											restaurerSortie = 0;
										}
										if ( restaurerEntree == 1 )
										{
											restore_stdin( entree_std );
											restaurerEntree = 0;
										}
										
										close ( copieLecturePipe );
										dup( copieLecture );
										close( copieLecture );
									}
									
								}
							}
							strcpy( argument, "" );
							strcpy( commande, mot );
							//fprintf( stderr, "DEBUG processus à lancer: '%s'.\n",commande);
							faire_la_redirection = 1;
							attention_redirection_sortie = 0;
							attention_redirection_entree = 0;
						}
						else
						{
							strcpy( commande, mot );
							//fprintf( stderr, "DEBUG processus à lancer: '%s'.\n",commande);
						}
						argumentEnCours = 1;
						//fprintf( stderr, "DEBUG argumentEnCours: '%d'.\n",argumentEnCours);
					}					
				}
				else
				{
					analyseEnCours = 1;
					if( faire_la_redirection == 0)
					{
						//fprintf( stderr, "TEST1.\n");
						//fprintf( stderr, "DEBUG commande: '%s'.\n", commande );
						//fprintf( stderr, "DEBUG argument: '%s'.\n", argument );
						//fprintf( stderr, "DEBUG stdin: '%d'.\n", entree_std );
						//fprintf( stderr, "DEBUG stdout: '%d'.\n", sortie_std );
						//fprintf( stderr, "DEBUG sortie_std: '%d'.\n", sortie_std );
						list_origin = add_process_env( list_origin, commande, argument, entree_std, sortie_std );
						//fprintf( stderr, "TEST1.\n");
						
						// On se divise !!
						processus = fork();
						if(processus == -1)
							fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
						// Si je suis le fils
						if(processus == 0)
						{
							// j'exécute la commande demandée
							if ( strcmp(argument,"") == 0 )
								execlExit= execl( commande, commande, NULL );
							else
								execlExit= execl( commande, commande, argument, NULL );
							// je sors en erreur si execl à un problème d'exécution
							if( execlExit < 0 ) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n" );
							exit( 0 );
						}		
						// Sinon, je suis le père
						else
						{
							// Et j'attends la fin de mes fils
							processus=wait(&statusFils);
							
							if ( restaurerSortie == 1 )
							{
								restore_stdout( sortie_std );
								restaurerSortie = 0;
							}
							if ( restaurerEntree == 1 )
							{
								restore_stdin( entree_std );
								restaurerEntree = 0;
							}

						}
					}
					else
					{
						//fprintf( stderr, "DEBUG attention_redirection_sortie: '%d'.\n", attention_redirection_sortie );
						//fprintf( stderr, "DEBUG sortie_std: '%d'.\n", sortie_std );
						//fprintf( stderr, "DEBUG fp[1]: '%d'.\n", fp[1] );
						//~ if( attention_redirection_sortie == 0 )
						//~ {
						if ( attention_redirection_entree == 0 )
						{
							copieLecture = dup( 0 );
							close( 0 );
							fprintf( stderr, "DEBUG ULTIME '%d'.\n", fp[0]);
							copieLecturePipe = dup( fp[0] );
							close( fp[0] );
							
							fprintf( stderr, "DEBUG ULTIME '%d' '%d'.\n", copieLecture, copieLecturePipe);
							
							fprintf( stderr, "DEBUG 2 if.\n");
							list_origin = add_process_env( list_origin, commande, argument, fp[0], 1 );
							// On se divise !!
							processus = fork();
							if(processus == -1)
								fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
							// Si je suis le fils
							if(processus == 0)
							{
								// j'exécute la commande demandée
								if ( strcmp(argument,"") == 0 )
									execlExit= execl( commande, commande, NULL );
								else
									execlExit= execl( commande, commande, argument, NULL );
								// je sors en erreur si execl à un problème d'exécution
								if( execlExit < 0 ) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n" );
								exit( 0 );
							}		
							// Sinon, je suis le père
							else
							{
								// Et j'attends la fin de mes fils
								processus=wait(&statusFils);
															
								if ( restaurerSortie == 1 )
								{
									restore_stdout( sortie_std );
									restaurerSortie = 0;
								}
								if ( restaurerEntree == 1 )
								{
									restore_stdin( entree_std );
									restaurerEntree = 0;
								}
								
								close ( copieLecturePipe );
								dup( copieLecture );
								close( copieLecture );
							}									
						}
						else
						{
							fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '<' (%d).\n",fp[0]);
							//close ( fp[0] );
							
							fprintf( stderr, "DEBUG ULTIME '%d'.\n", fp[1]);
							
							// On se divise !!
							processus = fork();
							if(processus == -1)
								fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n" );
							// Si je suis le fils
							if(processus == 0)
							{
								// j'exécute la commande demandée
								if ( strcmp(argument,"") == 0 )
									execlExit= execl( commande, commande, NULL );
								else
									execlExit= execl( commande, commande, argument, NULL );
								// je sors en erreur si execl à un problème d'exécution
								if( execlExit < 0 ) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n" );
								exit( 0 );
							}		
							// Sinon, je suis le père
							else
							{
								// Et j'attends la fin de mes fils
								processus=wait(&statusFils);
															
								if ( restaurerSortie == 1 )
								{
									restore_stdout( sortie_std );
									restaurerSortie = 0;
								}
								if ( restaurerEntree == 1 )
								{
									restore_stdin( entree_std );
									restaurerEntree = 0;
								}

							}
						}
						//~ }
						//~ else
						//~ {
							//~ //fprintf( stderr, "DEBUG else.\n");
							//~ list_origin = add_process_env( list_origin, commande, argument, fp[0], sortie_std );
						//~ }
						//faire_la_redirection = 0;
					}
				}
			}
			//~ fprintf( stderr, "TEST liste niveau 1.\n");
			//~ fprintf( stderr, "DEBUG command: '%s'.\n", list_origin->command );
			//~ fprintf( stderr, "DEBUG args: '%s'.\n", list_origin->args );
			//~ fprintf( stderr, "DEBUG stdin_fd: '%d'.\n", list_origin->stdin_fd );
			//~ fprintf( stderr, "DEBUG stdout_fd: '%d'.\n", list_origin->stdout_fd );
			//~ fprintf( stderr, "DEBUG next: '%p'.\n", list_origin->next );
			//~ 
			//~ fprintf( stderr, "TEST liste niveau 2.\n");
			//~ fprintf( stderr, "DEBUG command: '%s'.\n", (list_origin->next)->command );
			//~ fprintf( stderr, "DEBUG args: '%s'.\n", (list_origin->next)->args );
			//~ fprintf( stderr, "DEBUG stdin_fd: '%d'.\n", (list_origin->next)->stdin_fd );
			//~ fprintf( stderr, "DEBUG stdout_fd: '%d'.\n", (list_origin->next)->stdout_fd );
			//~ fprintf( stderr, "DEBUG next: '%p'.\n", (list_origin->next)->next );
			
			//restore_stdout( sortie_std );
			//restore_stdin( entree_std );
			
			//exit(0);
			
			
			
			//~ // AVEC FORK
			//~ // On se prépare à exécuter la commande demandée dans un processus fils !
			//~ processus = fork();
			//~ if(processus == -1)
				//~ fprintf( stderr, "Ouille... grosse erreur, je n'ai pas réussi à créer le processus fils !\n");
			//~ // Si je suis le fils
			//~ if(processus == 0)
			//~ {
	//~ 
				//~ 
				//~ // j'exécute la commande demandée
				//~ execlExit=execl(commande,commande,NULL);
				//~ // je sors en erreur si execl à un problème d'exécution
				//~ if(execlExit<0) fprintf( stderr, "Erreur d'exécution, la commande est peut-être inconnue !\n");
				//~ exit(0);
			//~ }		
			//~ // Sinon, je suis le père
			//~ else
			//~ {
				//~ // Et j'attends la fin de mes fils
				//~ processus=wait(&statusFils);
			//~ }
			//~ 
			//~ if ( redirection_sortie == 1 )
			//~ {
				//~ restore_stdout( sortie_std );
			//~ }	
			//~ if ( redirection_entree == 1 )
			//~ {
				//~ restore_stdin( entree_std );
			//~ }
			//~ //printf( "tigre\n" );
//~ 
		}
		//~ 
	    //~ redirection_sortie = 0;
	    //~ redirection_entree = 0;
	    //~ 
	    //~ //delete_process_env_list( list_origin );
	    //~ //list_origin = NULL;
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
	//fprintf( stderr, "passé par la fonction restore_stdout\n");
	
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
	//fprintf( stderr, "passé par la fonction restore_stdin\n");
	
	// on ferme le ficher
	close( 0 );
	
	dup( entree_std );	// il trouve tout seul 1 qui est le premier dispo ?
	close( entree_std );
}

