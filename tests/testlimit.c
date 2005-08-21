// $Id: testlimit.c,v 1.3 2004/01/13 14:56:26 ensc Exp $

// Copyright (C) 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
// based on tests/testlimit.cc by Jacques Gelinas
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
	if (argc != 3){
		fprintf (stderr,"testlimit nbprocess malloc_size\n");
	}else{
		int nbproc = atoi(argv[1]);
		int size   = atoi(argv[2]);
		int i;
		int status;
		for (i=0; i<nbproc; i++){
			if (fork()==0){
				void *pt = malloc(size);
				if (pt == NULL){
					fprintf (stderr,"Child %d: can't malloc (%s)\n"
						,i,strerror(errno));
				}else{
					memset (pt,1,size);
				}
				sleep (100);
				_exit (0);
			}
		}
		while (wait(&status)!=-1);
	}
	return 0;
}

