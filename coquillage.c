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
#define BUFFERSIZE 514	/* contient le nombre maximum de caractères entrés par l'utilisateur (entrée utilisateur + retour chariot + 1 pour tester le dépassement de cette valeur) */


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
    int analyseEnCours; 						/* prévient l'analyseur d'arrêter l'analyse de la l'entrée utilisateur */
    int argumentEnCours;						/* prévient l'analyseur que l'on analyse un argument */
	
	
	/* TRAITEMENT */
	
	/* premier affichage lorsqu'on ouvre le mini-shell */
	printf( "Bienvenue sur le coquillage ! :D\n" );
	printf( "Vous pouvez appeler des exécutables locaux avec un argument, et utiliser les symboles \"<\",\">\" et \"|\".\n" );
	
	/* Boucle d'attente des entrées utilisateur */
	while ( nbCarLus >= 0 )
	{
		
		/* On débuffurise stdout afin de pouvoir afficher immédiatement la ligne d'un printf qui ne contient pas un retour chariot */
		setbuf( stdout, NULL );
		/* Affichage d'un prompt */
		printf( "Coquillage $ " );
	
	
		/* Les caractères lus sont stockés dans tampon */
		nbCarLus = read( 0, tampon, BUFFERSIZE );

		/* On sort en erreur si read à un problème de lecture */
		if( nbCarLus < 0 ) 
		{
			fprintf( stderr, "Erreur de lecture de l'entrée utilisateur.\n" );
			exit( 1 );
		}
		/* on copie le pointeur vers le tampon afin de ne pas "décaler" le pointeur original */
		pointer = tampon;
		
		/* Ici on remplace le retour chariot à la fin de la chaine de caractère par une fin de chaine de caractère */
		tampon[nbCarLus-1] = '\0';

		/* On traite le cas où le tampon est trop rempli et qu'il risque de mal exécuter l'entrée utilisateur tronquée */
		if( nbCarLus == BUFFERSIZE )
		{
			fprintf( stderr, "La commande est ignorée car elle dépasse %d caractères.\n", BUFFERSIZE-2 );
		}
		/* On traite le cas où le tampon est vide, pas d'entrée utilisateur, alors on n'execute rien ! Et on recommence la boucle pour rendre le prompt. */
		else if( ! strcmp( tampon, "" ) );
		/* Sinon, on gère la demande de fermeture du shell coquillage avec la commande exit */
		else if( ! strcmp( tampon, "exit" ) )
		{
			printf( "Fermeture de coquillage... Au revoir !\n" );
			exit( 0 );
		}
		/* Et sinon, on traite l'entrée utilisateur demandée ! */
		else
		{
			/* ANALYSE DE LA LIGNE DE COMMANDES ENTRÉE PAR L'UTILISATEUR */
			
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
			/* on initialise les arguments à une chaine vide, au cas où il n'y en aurait pas du tout */
			if( strcpy( argument, "" ) == NULL )
			{
				fprintf( stderr, "Erreur d'initialisation d'argument à une chaîne vide.\n" );
				exit( 1 );
			}
			
			
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
					if( ! strcmp( mot, ">" ) )
					{
						/* on retient que le prochain mot sera le fichier vers où rediriger */
						redirection_sortie = 1;
						attention_redirection_sortie = 1;
						/* on prévient également l'analyseur qu'on en a fini des éventuels arguments de la commande */
						argumentEnCours = 0;
					} /* fin de la lecture du mot ">" */
					
					/* cas où l'utilisateur demande une redirection de l'entrée vers un fichier :
					 * on modifie les avertisseurs qui permettront d'agir en fonction au prochain mot */
					else if ( ! strcmp( mot, "<" ) )
					{
						/* on retient que le prochain mot sera le fichier vers où rediriger */
						redirection_entree = 1;
						attention_redirection_entree = 1;
						/* on prévient également qu'on en a fini des éventuels arguments de la commande */
						argumentEnCours = 0;
					} /* fin de la lecture du mot "<" */
					
					/* cas où l'utilisateur demande une redirection de la sortie d'une commande vers l'entrée d'une autre :
					 * on modifie les avertisseurs qui permettront d'agir en fonction au prochain mot */
					else if( ! strcmp( mot, "|" ) )
					{
						/* on retient que le prochain mot sera la deuxième commande de la paire qui s'échange des résultats */
						redirection_sortie_entree = 1;
						/* on prévient également qu'on en a fini des éventuels arguments de la première commande de la paire */
						argumentEnCours = 0;
					} /* fin de la lecture du mot "|" */
					
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
							if( strcpy( argument, mot ) == NULL )
							{
								fprintf( stderr, "Erreur de copie du mot dans argument.\n" );
								exit( 1 );
							}
						} /* fin de la lecture de l'argument qui suit une commande */
						
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
						} /* fin du traitement du nom de fichier qui suit un ">" */
						
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
						} /* fin du traitement du nom de fichier qui suit un "<" */
						
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
								if( attention_redirection_sortie == 0 )
								{	
									/* On utilise la fonction qui crée un pipe et redirige la sortie, tout en testant son retour */
									if( creationPipe( fp, &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de création du pipe.\n" );
										exit( 1 );
									}

									/* fork() puis execution de la commande courante avec son argument.
									 * On appelle la fonction tout en testant son retour */
									if( forkNexec( commande, argument ) )
									{
										fprintf( stderr, "Erreur de fork et exécution.\n" );
										exit( 1 );
									}
									
									//~
									//~ if ( restaurerSortie == 1 )
									//~ {
											//~ restore_stdout( sortie_std );
											//~ restaurerSortie = 0;
									//~ }
									
									/* Si l'entrée de la commande courante a été redirigée ("<" avant le "|"), 
									 * il faut restaurer l'entrée standard après exécution de la commande. */
									if ( restaurerEntree == 1 )
									{
										/* On restaure l'entrée standard clavier. */
										restore_stdin( entree_std );
										/* Et on désactive l'avertisseur car le traitement est fait. */
										restaurerEntree = 0;
									}

									/* On ferme le côté écriture du pipe car il est rempli, on n'a plus besoin de ce côté.
									 * Au passage, la sortie standard écran est rétablie pour la prochaine commande.
									 * Comme toujours, la sortie d'erreur est testée en même temps que la fonction est appelée. */
									if( fermerPipe( &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}
								} /* fin du traitement de la commande à gauche du 1er "|" qui n'a pas de redirection de sortie (>) */
								
								/* Cas dégradé : la commande à gauche du "|" a précédemment subi une redirection de sortie (>) */
								else
								{
									/* On avertit l'utilisateur que la sortie de la commande sera dans le fichier,
									 * et que le contenu du pipe sera donc vide pour la prochaine commande. */
									fprintf( stderr, "Warning: Attention le pipe ne devrait pas prendre le dessus sur '>'.\n" );
									
									/* fork() puis execution de la commande courante avec son argument.
									 * On appelle la fonction tout en testant son retour */
									if( forkNexec( commande, argument ) )
									{
										fprintf( stderr, "Erreur de fork et exécution.\n" );
										exit( 1 );
									}
								
									/* Une fois la commande exécutée, la sortie standard écran doit être restaurée.
									 * On rappelle que cette sortie était le fichier spécifié derrière le ">" */
									//~ if ( restaurerSortie == 1 )
									//~ {
										restore_stdout( sortie_std );
										restaurerSortie = 0;
									//~ }
									
									/* Si l'entrée de la commande courante a été redirigée ("<" avant le "|"), 
									 * il faut restaurer l'entrée standard après exécution de la commande. */
									if ( restaurerEntree == 1 )
									{
										restore_stdin( entree_std );
										restaurerEntree = 0;
									}
									
									/* Le pipe n'avait pas lieu d'être pour cette commande, car la sortie devait être le fichier.
									 * Cependant, l'entrée de la commande suivante doit quand même être un pipe, même vide !
									 * Ainsi, on crée un pipe après l'exécution, qui sera vide, destiné à l'entrée de la commande suivante. */
									if( creationPipe( fp, &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de création du pipe.\n" );
										exit( 1 );
									}

									/* Et on referme immédiatement le côté écriture du pipe, qui ne sert à rien dans ce cas. */
									if( fermerPipe( &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}
								} /* fin du traitement de la commande à gauche du 1er "|" qui a précédemment subi une redirection de sortie (>) */				
							} /* fin du traitement de la toute première commande, à gauche du premier "|" */

							/* On est toujours dans le cas où le dernier mot lu était "|" ;
							 * on dispose donc de quoi exécuter la commande courante.
							 * Cette fois, la commande suit un "|", il faut donc dès le départ rediriger son entrée standard,
							 * grâce à un pipe pré-existant. 
							 * De plus, il faudra créer un nouveau pipe de sortie, car la commande courante est appelée entre deux "|" */
							else
							{
																		
								/* Cas nominal : aucune redirection d'entrée (<) ou de sortie (>) concurrente n'a été détectée */
								if( attention_redirection_sortie == 0 && attention_redirection_entree == 0 )
								{
									/* On commence par appeler la fonction qui récupère le pipe existant et change l'entrée standard,
									 * tout en testant la valeur de retour. */
									if( recuperationPipe( fp, &copieLecture, &copieLecturePipe ) )
									{
										fprintf( stderr, "Erreur de récupération du pipe.\n" );
										exit( 1 );
									}
								
									/* Ensuite, on appelle la fonction qui créer un nouveau pipe et redirige la sortie. */
									if( creationPipe( fp, &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de création du pipe.\n" );
										exit( 1 );
									}
													
									/* fork() puis execution de la commande courante avec son argument.
									 * On appelle la fonction tout en testant son retour */	
									if( forkNexec( commande, argument ) )
									{
										fprintf( stderr, "Erreur de fork et exécution.\n" );
										exit( 1 );
									}
																										
									//~ if ( restaurerSortie == 1 )
									//~ {
										//~ restore_stdout( sortie_std );
										//~ restaurerSortie = 0;
									//~ }
									//~ if ( restaurerEntree == 1 )
									//~ {
										//~ restore_stdin( entree_std );
										//~ restaurerEntree = 0;
									//~ }
									
									/* On ferme le côté lecture du pipe précédent la commande,
									 * car il est vidé, on n'a plus besoin de ce côté.
									 * Au passage, l'entrée standard écran est rétablie pour la prochaine commande.
									 * Comme toujours, la sortie d'erreur est testée en même temps que la fonction est appelée. */
									if( fermerPipe( &copieLecture, &copieLecturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}
								
									/* On ferme également le côté écriture du pipe suivant la commande,
									 * car il est rempli, on n'a plus besoin de ce côté.
									 * Au passage, la sortie standard écran est rétablie pour la prochaine commande. */	
									if( fermerPipe( &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}		
								} /* fin du traitement d'une commande en sandwich entre deux "|", sans redirection d'entrée (<) ni de sortie (>) */
								
							
								/* Cas dégradé 1 : redirection de sortie (>) concurrente sans redirection d'entrée (<) */
								else if( attention_redirection_sortie == 1 && attention_redirection_entree == 0 )
								{
									/* On avertit l'utilisateur que la sortie de la commande sera dans le fichier,
									 * et que le contenu du pipe sera donc vide pour la prochaine commande. */
									fprintf( stderr, "Warning: Attention le pipe ne devrait pas prendre le dessus sur '>'.\n" );
									
									/* On appelle la fonction qui récupère le pipe existant et change l'entrée standard,
									 * tout en testant la valeur de retour. */
									if( recuperationPipe( fp, &copieLecture, &copieLecturePipe ) )
									{
										fprintf( stderr, "Erreur de récupération du pipe.\n" );
										exit( 1 );
									}
									
									/* fork() puis execution de la commande courante avec son argument.
									 * On appelle la fonction tout en testant son retour */	
									if( forkNexec( commande, argument ) )
									{
										fprintf( stderr, "Erreur de fork et exécution.\n" );
										exit( 1 );
									}
	
									/* A cause de la redirection de la sortie vers un fichier, il faut (provisoirement) rétablir la sortie standard écran */									
									//~ if ( restaurerSortie == 1 )
									//~ {
										restore_stdout( sortie_std );
										restaurerSortie = 0;
									//~ }
									//~ if ( restaurerEntree == 1 )
									//~ {
										//~ restore_stdin( entree_std );
										//~ restaurerEntree = 0;
									//~ }
								
									/* On ferme le côté lecture du pipe précédent la commande,
									 * car il est vidé, on n'a plus besoin de ce côté.
									 * Au passage, l'entrée standard écran est rétablie pour la prochaine commande.
									 * Comme toujours, la sortie d'erreur est testée en même temps que la fonction est appelée. */
									if( fermerPipe( &copieLecture, &copieLecturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}
																		
									/* Le pipe n'avait pas lieu d'être pour cette commande, car la sortie devait être le fichier.
									 * Cependant, l'entrée de la commande suivante doit quand même être un pipe, même vide !
									 * Ainsi, on crée un pipe après l'exécution, qui sera vide, destiné à l'entrée de la commande suivante. */
									if( creationPipe( fp, &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de création du pipe.\n" );
										exit( 1 );
									}
									
									/* Et on referme immédiatement le côté écriture du pipe, qui ne sert à rien dans ce cas. */
									if( fermerPipe( &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}
								
								}/* fin du traitement d'une commande en sandwich entre deux "|", avec redirection de sortie (>) mais sans redirection d'entrée (<) */
									
								/* Cas dégradé 2 : redirection d'entrée (<) concurrente sans redirection de sortie (>) */
								else if( attention_redirection_sortie == 0 && attention_redirection_entree == 1 )
								{
									/* On avertit l'utilisateur que l'entrée de la commande sera le fichier,
									 * et que le contenu du pipe précédent sera donc ignoré pour la commande courante. */
									fprintf( stderr, "Warning: Attention le pipe ne devrait pas prendre le dessus sur '<'.\n");
									
									// A VERIFIER ABSOLUMENT
									/* On commence par appeler la fonction qui récupère le pipe existant et change l'entrée standard,
									 * tout en testant la valeur de retour. */
									if( recuperationPipe( fp, &copieLecture, &copieLecturePipe ) )
									{
										fprintf( stderr, "Erreur de récupération du pipe.\n" );
										exit( 1 );
									}
									/* On ferme le côté lecture du pipe précédent la commande,
									 * car il est vidé, on n'a plus besoin de ce côté.
									 * Au passage, l'entrée standard écran est rétablie pour la prochaine commande.
									 * Comme toujours, la sortie d'erreur est testée en même temps que la fonction est appelée. */
									if( fermerPipe( &copieLecture, &copieLecturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}
									
									
									/* On appelle immédiatement la fonction qui créer un nouveau pipe et redirige la sortie. 
									 * Il n'est pas nécessaire de récupérer le pipe précédent car il est ignoré. */
									if( creationPipe( fp, &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de création du pipe.\n" );
										exit( 1 );
									}
									
									/* fork() puis execution de la commande courante avec son argument.
									 * On appelle la fonction tout en testant son retour */	
									if( forkNexec( commande, argument ) )
									{
										fprintf( stderr, "Erreur de fork et exécution.\n" );
										exit( 1 );
									}
									
// STOP										
									//~ if ( restaurerSortie == 1 )
									//~ {
										//~ restore_stdout( sortie_std );
										//~ restaurerSortie = 0;
									//~ }
									//~ if ( restaurerEntree == 1 )
									//~ {
										restore_stdin( entree_std );
										restaurerEntree = 0;
									//~ }
									
									if( fermerPipe( &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}
								}
								// s'il y a une redirection de sortie et une redirection d'entrée
								else if(attention_redirection_sortie == 1 && attention_redirection_entree == 1)
								{
									fprintf( stderr, "Warning: Attention le pipe ne devrait prendre le dessus ni sur '>' ni sur '<'.\n" );
									
									// fork() puis execution de la commande avec son argument
									if( forkNexec( commande, argument ) )
									{
										fprintf( stderr, "Erreur de fork et exécution.\n" );
										exit( 1 );
									}
																			
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
									if( creationPipe( fp, &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de création du pipe.\n" );
										exit( 1 );
									}
									if( fermerPipe( &copieEcriture, &copieEcriturePipe ) )
									{
										fprintf( stderr, "Erreur de fermeture du pipe.\n" );
										exit( 1 );
									}
								}
							}
							if( strcpy( argument, "" ) == NULL )
							{
								fprintf( stderr, "Erreur de ré-initialisation d'argument à une chaîne vide.\n" );
								exit( 1 );
							}
							if( strcpy( commande, mot ) == NULL )
							{
								fprintf( stderr, "Erreur de copie du mot dans argument.\n" );
								exit( 1 );
							}
							faire_la_redirection = 1;
							attention_redirection_sortie = 0;
							attention_redirection_entree = 0;
						}
						else
						{
							if( strcpy( commande, mot ) == NULL )
							{
								fprintf( stderr, "Erreur de copie du mot dans argument.\n" );
								exit( 1 );
							}
						}
						argumentEnCours = 1;
					}					
				}
				else
				{
					analyseEnCours = 1;
					// on ne passe par ici que dans le cas où la l'entrée utilisateur ne contenait qu'une commande avec ou sans argument avec ou sans redirection de sortie et/ou entrée
					if( faire_la_redirection == 0 )
					{
						// fork() puis execution de la commande avec son argument
						if( forkNexec( commande, argument ) )
						{
							fprintf( stderr, "Erreur de fork et exécution.\n" );
							exit( 1 );
						}
									
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
					// sinon on passe par là, car il y a eu un pipe avant la commande finale de l'entrée utilisateur.
					else
					{
						if ( attention_redirection_entree == 0 )
						{
							if( recuperationPipe( fp, &copieLecture, &copieLecturePipe ) )
							{
								fprintf( stderr, "Erreur de récupération du pipe.\n" );
								exit( 1 );
							}

							// fork() puis execution de la commande avec son argument
							if( forkNexec( commande, argument ) )
							{
								fprintf( stderr, "Erreur de fork et exécution.\n" );
								exit( 1 );
							}
																						
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

							if( fermerPipe( &copieLecture, &copieLecturePipe ) )
							{
								fprintf( stderr, "Erreur de fermeture du pipe.\n" );
								exit( 1 );
							}
						}
						else
						{
							
							// A VERIFIER ABSOLUMENT
							/* On commence par appeler la fonction qui récupère le pipe existant et change l'entrée standard,
							 * tout en testant la valeur de retour. */
							if( recuperationPipe( fp, &copieLecture, &copieLecturePipe ) )
							{
								fprintf( stderr, "Erreur de récupération du pipe.\n" );
								exit( 1 );
							}							
							/* On ferme le côté lecture du pipe précédent la commande,
							 * car il est vidé, on n'a plus besoin de ce côté.
							 * Au passage, l'entrée standard écran est rétablie pour la prochaine commande.
							 * Comme toujours, la sortie d'erreur est testée en même temps que la fonction est appelée. */
							if( fermerPipe( &copieLecture, &copieLecturePipe ) )
							{
								fprintf( stderr, "Erreur de fermeture du pipe.\n" );
								exit( 1 );
							}
							
							// fork() puis execution de la commande avec son argument
							if( forkNexec( commande, argument ) )
							{
								fprintf( stderr, "Erreur de fork et exécution.\n" );
								exit( 1 );
							}
							
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
