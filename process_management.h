/*
 * process_management.h
 * 
 * Copyright 2014 Porté Marie-Luc and Debenath-Redondo Olmo
 * 
 * 
 * IN321 : Systèmes d'exploitation
 * BE mini-shell
 * Ce programme constitue un header pour définir les fonctions utiles au
 * mini-shell Coquillage.
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
