/*
 * lire.c
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFERSIZE 512

int main(int argc, char **argv)
{
	// Variables
	int fd;
	char name[BUFFERSIZE];
	int modeLecture = 0;  // mode de lecture : 0 = lecture, 1 = écriture, 2 = lecture et écriture
	char caractere;
	
	// Traitement
	
	// On récupère le nom du fichier en argument, sinon on lit l'entrée standard
	if(argc == 1)
		fd = 0;
	else
	{
		strcpy(name, argv[1]);
		// On essaye d'ouvrir le fichier
		fd = open(name,modeLecture);
		// On vérifie la réussite de l'ouverture
		if(fd == -1)
		{
			printf("Erreur d'ouverture du fichier '%s'\n",name);
			exit(1);
		}
	}

	
	// On débuffurise stdout afin de pouvoir afficher immédiatement la ligne d'un printf qui ne contient pas un retour chariot
	setbuf(stdout, NULL);
	
	// On affiche le contenu du fichier
	while(read(fd,&caractere,1) > 0)
	{
		if( caractere != '\n' )
			printf("%c",caractere+1);
	}
	
	printf("\n");
	
	// On essaye de fermer le fichier
	close(fd);
	
	return 0;
}

