/*
 * coquillage.c
 * 
 * Copyright 2014 Porté Marie-Luc and Debenath-Redondo Olmo
 * 
 * 
 * IN321 : Systèmes d'exploitation
 * BE mini-shell
 * Ce programme constitue un mini-shell, intitulé "coquillage", capable 
 * d'exécuter des programmes dont l'exécutable se situe dans le dossier
 * courant, avec un argument ou sans, et de gérer les redirections de 
 * type ">", "<" et "|".
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


/* bibliothèques standards */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* bibliothèque pour gérer l'ouverture/fermeture d'entrée/sortie standard et de pipe,
 * ainsi que le fork et exécution de processus */
#include "process_management.h"

/* constantes utiles */
#define BUFFERSIZE 512	/* contient le nombre maximum de caractères entrés par l'utilisateur */


int main( int argc, char** argv )
{
	/* VARIABLES */
	
	int nbCarLus = 0;							/* permet de connaitre le nombre de caractères lus */
	char tampon[BUFFERSIZE];					/* tableau qui contient l'entrée utilisateur tapée */
	int sortie_std = 1;							/* initialisation de la sortie standard à 1 (écran) */
	int entree_std = 0;							/* initialisation de l'entrée standard à 0 (clavier) */
	char* pointer;								/* pointeur sur le tampon afin de travailler avec par la suite */
    char mot[BUFFERSIZE];						/* récupère le "mot" (commande, argument, ">", "<", "|", nom_fic) qui va être analysé */
    char commande[BUFFERSIZE];					/* récupère la commande (nom du binaire à exécuter) analysée */
    char argument[BUFFERSIZE];					/* récupère l'argument de la commande analysée */
    int redirection_sortie;						/* prévient l'analyseur qu'au prochain "mot" il faudra traiter une redirection de la sortie de la "commande" vers le fichier "mot" */
	int redirection_entree;						/* prévient l'analyseur qu'au prochain "mot" il faudra traiter une redirection de l'entrée de la "commande" depuis le fichier "mot" */
	int redirection_sortie_entree;				/* prévient l'analyseur qu'au prochain "mot" il faudra traiter une redirection de la sortie de la "commande" précédente vers le prochain "mot" (prochaine commande) */
	int faire_la_redirection;					/* prévient l'analyseur qu'au prochain "mot" il faudra traiter une redirection de l'entrée de la "commande" depuis le pipe de lecture */
	int attention_redirection_sortie;			/* prévient l'analyseur qu'il y a eu un ">" avant un "|" et qu'il devra prendre en compte en priorité le ">" */
	int	attention_redirection_entree;			/* prévient l'analyseur qu'il y a eu un "|" avant un "<" et qu'il devra prendre en compte en priorité le "<" */
	int restaurerSortie;						/* prévient l'analyseur qu'il faut restaurer la sortie standard */
	int restaurerEntree;						/* prévient l'analyseur qu'il faut restaurer l'entrée standard */
    int fp[2];									/* contient les 2 descripteurs pour faire des "|" */
	int copieEcriture;							/* contient la copie temporaire de la sortie standard */
	int copieEcriturePipe;						/* contient la copie temporaire du côté écriture du pipe */
    int copieLecture;							/* contient la copie temporaire de l'entrée standard */
    int copieLecturePipe;						/* contient la copie temporaire du côté lecture du pipe */
    int analyseEnCours; 						/* prévient l'analyseur d'arrêter l'analyse de la ligne de commandes */
    int argumentEnCours;						/* prévient l'analyseur que l'on analyse un argument */
	
	
	/* TRAITEMENT */
	
	/* premier affichage lorsqu'on ouvre le mini-shell */
	printf("Bienvenue sur le coquillage ! :D\n");
	printf("Vous pouvez appeler des exécutables locaux avec un argument, et utiliser les symboles \"<\",\">\" et \"|\".\n");
	
	/* Boucle d'attente des commandes */
	while (nbCarLus >= 0)
	{
		
		/* On débuffurise stdout afin de pouvoir afficher immédiatement la ligne d'un printf qui ne contient pas un retour chariot */
		setbuf(stdout, NULL);
		/* Affichage d'un prompt */
		printf("Coquillage $ ");
	
	
		/* Les caractères lus sont stockés dans tampon */
		nbCarLus=read(0,tampon,BUFFERSIZE);
		/* On sort en erreur si read à un problème de lecture */
		if(nbCarLus<0) 
		{
			fprintf( stderr, "Erreur de lecture de l'entrée utilisateur.\n");
			exit(1);
		}
		/* on copie le pointeur vers le tampon afin de ne pas "décaler" le pointeur original */
		pointer = tampon;
		
		/* Ici on remplace le retour chariot à la fin de la chaine de caractère par une fin de chaine de caractère */
		tampon[nbCarLus-1]='\0';
		
		// DEBUG
		// Test d'affichage du nombre de caractères lus et du tampon
		//fprintf( stderr, "J'ai lu %d caractères, et la chaine est '%s'.\n",nbCarLus-1,tampon);

		/* On traite le cas où le tampon est vide, pas de commande, alors on execute rien ! Et on recommence la boucle pour rendre le prompt. */
		if(!strcmp(tampon,""));
		/* Sinon, on gère la demande de fermeture du shell coquillage avec la commande exit */
		else if(!strcmp(tampon,"exit"))
		{
			printf("Fermeture de coquillage... Au revoir !\n");
			exit(0);
		}
		/* Et sinon, on exécute la commande demandée ! */
		else
		{

			/* ANALYSE DE LA LIGNE DE COMMANDE ENTRÉE PAR L'UTILISATEUR */
			
			/* on initialise tous les avertisseurs à zéro, car on n'a encore rien fait */
			/* ces avertisseurs serviront à se souvenir que des redirections ont été demandées au fur et à mesure de la lecture */
			analyseEnCours = 0;
			argumentEnCours = 0;
			redirection_sortie = 0;
			redirection_entree = 0;
			redirection_sortie_entree = 0;
			attention_redirection_sortie = 0;
			attention_redirection_entree = 0;
			faire_la_redirection = 0;
			restaurerSortie = 0;
			restaurerEntree = 0;
			fp[0] = 0;
			fp[1] = 0;
			// on initialise les arguments à une chaine vide, au cas où il n'y en aurait pas du tout
			if (strcpy( argument, "" )==NULL) exit(1);
			
			
			/* La boucle suivante constitue le coeur de ce mini-shell.
			 * Tout d'abord, on analyse mot par mot ce qui a été écrit,
			 * et on agit en fonction.
			 * Ensuite, la ou les commandes sont exécutées. */

			while( analyseEnCours == 0 )
			{
				
				/* La fonction read_and_move_forward enregistre un mot (séparateur = espace) dans "mot" et fait avancer le pointeur jusqu'au prochain mot ;
				 * on sort de ce if en arrivant à la fin de la commande tapée par l'utilisateur */
				if( read_and_move_forward( &pointer, mot ) != 0 )
				{
					/* cas où l'utilisateur demande une redirection de la sortie vers un fichier :
					 * on modifie les avertisseurs qui permettront d'agir en fonction au prochain mot */
					if( !strcmp( mot, ">" ) )
					{
						/* on retient que le prochain mot sera le fichier vers où rediriger */
						redirection_sortie = 1;
						attention_redirection_sortie = 1;
						// on prévient également qu'on en a fini des éventuels arguments de la commande
						argumentEnCours = 0;
					}
					
					/* cas où l'utilisateur demande une redirection de l'entrée vers un fichier :
					 * on modifie les avertisseurs qui permettront d'agir en fonction au prochain mot */
					else if ( !strcmp( mot, "<" ) )
					{
						/* on retient que le prochain mot sera le fichier vers où rediriger */
						redirection_entree = 1;
						attention_redirection_entree = 1;
						/* on prévient également qu'on en a fini des éventuels arguments de la commande */
						argumentEnCours = 0;
					}
					
					/* cas où l'utilisateur demande une redirection de la sortie d'une commande vers l'entrée d'une autre :
					 * on modifie les avertisseurs qui permettront d'agir en fonction au prochain mot */
					else if( !strcmp( mot, "|" ) )
					{
						/* on retient que le prochain mot sera la deuxième commande de la paire qui s'échange des résultats */
						redirection_sortie_entree = 1;
						/* on prévient également qu'on en a fini des éventuels arguments de la première commande de la paire */
						argumentEnCours = 0;
					}
					
					/* Si le mot lu n'est pas un symbole, il n'annonce pas une redirection des flux de données 
					 * Grâce aux avertisseurs, on sait s'il s'agit d'une commande, d'un argument ou d'un nom de fichier */
					else
					{
						
						/* Si le dernier mot lu était une commande ou un argument, le suivant est forcément un argument ;
						* cela est annoncé par l'avertisseur "argumentEnCours".
						* Attention, dans cette version on ne gère qu'un seul argument. */
						if( argumentEnCours == 1 )
						{
							/* On copie le mot dans une variable intermédiaire, pour l'utiliser lors de l'exécution de la commande. */
							strcpy( argument, mot );
						}
						
						/* Si le dernier mot lu était ">", le mot suivant est le nom du fichier dans lequel rediriger la sortie.
						 * On modifie immédiatement la sortie standard. */
						else if( redirection_sortie == 1 )
						{
							/* On remet cet avertisseur à 0, car la redirection est traitée, on passe à autre chose */
							redirection_sortie = 0;
							/* On utilise la fonction qui redirige la sortie :
							* on lui passe "mot" qui contient le nom du fichier à créer (ou écraser)
							* et "sortie_std" que la fonction remplira avec le descripteur de fichier associé */
							mask_stdout( mot, &sortie_std );
							/* On prévient l'analyseur qu'il faudra restaurer la sortie standard écran après l'exécution. */
							restaurerSortie = 1;
						}
						
						/* Si le dernier mot lu était "<", le mot suivant est le nom du fichier dans lequel rediriger l'entrée.
						 * On modifie immédiatement l'entrée standard. */
						else if( redirection_entree == 1 )
						{
							/* on remet cet avertisseur à 0, car la redirection est traitée, on passe à autre chose */
							redirection_entree = 0;
							/* On utilise la fonction qui redirige l'entrée :
							* on lui passe "mot" qui contient le nom du fichier à lire (si existant)
							* et "entree_std" que la fonction remplira avec le descripteur de fichier associé */	
							mask_stdin( mot, &entree_std );
							/* On prévient l'analyseur qu'il faudra restaurer l'entrée standard écran après l'exécution. */
							restaurerEntree = 1;
						}
						
						/* Si le dernier mot lu était "|", le mot suivant est la prochaine commande,
						 * dont l'entrée sera le pipe qui sert de sortie à la commande en cours.
						 * Dans cette partie, on gère immédiatement le pipe, et on exécute la commande. */
						else if( redirection_sortie_entree == 1 )
						{
							/* on remet cet avertisseur à 0, car la redirection est traitée, on passe à autre chose */
							redirection_sortie_entree = 0;
							
							/* Cas où ce "|" est le premier rencontré :
							 * dans ce cas, aucun pipe ne pré-existe, il est à créer.
							 * Ce cas n'arrive qu'une seule fois : à la fin de la toute première commande.
							 * Les commandes suivantes passeront dans le else suivant. */
							if( faire_la_redirection == 0 )
							{

								/* Cas nominal : la commande à gauche du "|" n'a pas de redirection de sortie (>) */
								// STOP
								if( attention_redirection_sortie == 0 )
								{	
									creationPipe( fp, &copieEcriture, &copieEcriturePipe );

									// fork() puis execution de la commande avec son argument
									forkNexec( commande, argument );
									
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

									fermerPipe( &copieEcriture, &copieEcriturePipe );
								}
								else
								{
									fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '>'.\n");
			
									// fork() puis execution de la commande avec son argument
									forkNexec( commande, argument );
																					
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
									
									// même si '>' prend le dessus sur "|" il ne faut pas oublié de créer quand même le pipe !
									creationPipe( fp, &copieEcriture, &copieEcriturePipe );
									fermerPipe( &copieEcriture, &copieEcriturePipe );
								}
							}
							else
							{
								// s'il n'y a ni une redirection de sortie ni une redirection d'entrée
								if( attention_redirection_sortie == 0 && attention_redirection_entree == 0 )
								{
									recuperationPipe( fp, &copieLecture, &copieLecturePipe );
								
									creationPipe( fp, &copieEcriture, &copieEcriturePipe );
																		
									// fork() puis execution de la commande avec son argument
									forkNexec( commande, argument );
																										
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
									
									fermerPipe( &copieLecture, &copieLecturePipe );

									fermerPipe( &copieEcriture, &copieEcriturePipe );		
								}
								// s'il n'y a qu'une redirection de sortie
								else if( attention_redirection_sortie == 1 && attention_redirection_entree == 0 )
								{
									fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '>'.\n");
									
									recuperationPipe( fp, &copieLecture, &copieLecturePipe );
									
									// fork() puis execution de la commande avec son argument
									forkNexec( commande, argument );
																			
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
									
									fermerPipe( &copieLecture, &copieLecturePipe );
									
									// même si '>' prend le dessus sur "|" il ne faut pas oublié de créer quand même le pipe !
									creationPipe( fp, &copieEcriture, &copieEcriturePipe );
									fermerPipe( &copieEcriture, &copieEcriturePipe );
									
								}
								// s'il n'y a qu'une redirection d'entrée
								else if( attention_redirection_sortie == 0 && attention_redirection_entree == 1 )
								{
									fprintf( stderr, "DEBUG: Attention le pipe ne devrait pas prendre le dessus sur '<'.\n");
									
									creationPipe( fp, &copieEcriture, &copieEcriturePipe );
									
									// fork() puis execution de la commande avec son argument
									forkNexec( commande, argument );
									
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
									
									fermerPipe( &copieEcriture, &copieEcriturePipe );
								}
								// s'il y a une redirection de sortie et une redirection d'entrée
								else if(attention_redirection_sortie == 1 && attention_redirection_entree == 1)
								{
									fprintf( stderr, "DEBUG: Attention le pipe ne devrait prendre le dessus ni sur '>' ni sur '<'.\n");
									
									// fork() puis execution de la commande avec son argument
									forkNexec( commande, argument );
																			
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
									
									// même si '>' prend le dessus sur "|" il ne faut pas oublié de créer quand même le pipe !
									creationPipe( fp, &copieEcriture, &copieEcriturePipe );
									fermerPipe( &copieEcriture, &copieEcriturePipe );
								}
							}
							strcpy( argument, "" );
							strcpy( commande, mot );
							faire_la_redirection = 1;
							attention_redirection_sortie = 0;
							attention_redirection_entree = 0;
						}
						else
						{
							strcpy( commande, mot );
						}
						argumentEnCours = 1;
					}					
				}
				else
				{
					analyseEnCours = 1;
					// on ne passe par ici que dans le cas où la ligne de commandes ne contenait qu'une commande avec ou sans argument avec ou sans redirection de sortie et/ou entrée
					if( faire_la_redirection == 0)
					{
						// fork() puis execution de la commande avec son argument
						forkNexec( commande, argument );
									
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
					// sinon on passe par là, car il y a eu un pipe avant la commande finale de la ligne de commandes.
					else
					{
						if ( attention_redirection_entree == 0 )
						{
							recuperationPipe( fp, &copieLecture, &copieLecturePipe );

							// fork() puis execution de la commande avec son argument
							forkNexec( commande, argument );
																						
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

							fermerPipe( &copieLecture, &copieLecturePipe );						
						}
						else
						{
							// fork() puis execution de la commande avec son argument
							forkNexec( commande, argument );
							
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
				}
			}
		}
	}
	return 0;
}



