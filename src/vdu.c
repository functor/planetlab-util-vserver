// $Id: vdu.c,v 1.1 2003/09/29 22:01:57 ensc Exp $

// Copyright (C) 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
// based on vdu.cc by Jacques Gelinas
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "compat.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <sys/ioctl.h>
#include "ext2fs.h"

// Patch to help compile this utility on unpatched kernel source
#ifndef EXT2_IMMUTABLE_FILE_FL
	#define EXT2_IMMUTABLE_FILE_FL	0x00000010
	#define EXT2_IMMUTABLE_LINK_FL	0x00008000
#endif


//__extension__ typedef long long		longlong;
__extension__ typedef long 		longlong;

static longlong inodes;
static longlong blocks;
static longlong size;

static short verbose = 0;

static inline void warning(char *s) {
    fprintf(stderr,"%s (%s)\n",s,strerror(errno));    
}

void panic(char *s) {
    warning(s);
    exit(2);
}

static void vdu_onedir (char const *path)
{
    struct stat dirst, st;
    struct dirent *ent;
    char *name;
    DIR *dir;
    int dirfd;
    longlong dirsize, dirinodes, dirblocks;

    dirsize = dirinodes = dirblocks = 0;

    // A handle to speed up chdir
    if ((dirfd = open (path,O_RDONLY)) == -1) {
	fprintf (stderr,"Can't open directory %s\n",path);
	panic("open failed");
    }

    if (fchdir (dirfd) == -1) {
	fprintf (stderr,"Can't fchdir directory %s\n",path);
	panic("fchdir failed");
    }

    if (fstat (dirfd,&dirst) != 0) {
	fprintf (stderr,"Can't lstat directory %s\n",path);
	panic("lstat failed");
    }

    if ((dir = opendir (".")) == NULL) {
	fprintf (stderr,"Can't open (opendir) directory %s\n",path);
	panic("opendir failed");
    }


    /* Walk the directory entries and compute the sum of inodes,
       blocks, and disk space used. This code will recursively descend
       down the directory structure. */

    while ((ent=readdir(dir))!=NULL){
	if (lstat(ent->d_name,&st)==-1){
	    fprintf (stderr,"Can't stat %s/%s\n",path,ent->d_name);
	    warning("lstat failed");
	    continue;
	}
	
	dirinodes ++;

	if (S_ISREG(st.st_mode)){
	    if (st.st_nlink > 1){
		long flags;
		int fd, res;

		if ((fd = open(ent->d_name,O_RDONLY))==-1) {
		    fprintf (stderr,"Can't open file %s/%s\n",path,ent->d_name);		    
		    warning ("open failed");
		    continue;
		}

		flags = 0;
		res = ioctl(fd, EXT2_IOC_GETFLAGS, &flags);
		close(fd);

		if ((res == 0) && (flags & EXT2_IMMUTABLE_LINK_FL)){
		    if (verbose)
			printf ("Skipping %s\n",ent->d_name);		    
		    continue;
		}
	    }
	    dirsize += st.st_size;
	    dirblocks += st.st_blocks;

	} else if (S_ISDIR(st.st_mode)) {
	    if ((st.st_dev == dirst.st_dev) &&
		(strcmp(ent->d_name,".")!=0) &&
		(strcmp(ent->d_name,"..")!=0)) {

		dirsize += st.st_size;
		dirblocks += st.st_blocks;

		name = strdup(ent->d_name);
		if (name==0) {
		    panic("Out of memory\n");
		}
		vdu_onedir(name);
		free(name);
		fchdir(dirfd);
	    }
	} else {
	    dirsize += st.st_size;
	    dirblocks += st.st_blocks;
	}
    }
    closedir (dir);
    close (dirfd);
    if (verbose)
	printf("%8ld %8ld %8ld %s\n",dirinodes, dirblocks, dirsize>>10,path);
    inodes += dirinodes;
    blocks += dirblocks;
    size   += dirsize;
}



int main (int argc, char *argv[])
{
    int startdir, i;

    if (argc < 2){
	fprintf (stderr,"vdu version %s\n",VERSION);
	fprintf (stderr,"vdu directory ...\n\n");
	fprintf (stderr,"Compute the size of a directory tree.");
    }else{
	if ((startdir = open (".",O_RDONLY)) == -1) {
	    fprintf (stderr,"Can't open current working directory\n");
	    panic("open failed");
	}

	for (i=1; i<argc; i++){
	    inodes = blocks = size = 0;
	    vdu_onedir (argv[i]);
	    printf("%8ld %8ld %8ld %s TOTAL\n",inodes, blocks, size>>10,argv[i]);
	    if (fchdir (startdir) == -1) {
		panic("fchdir failed");
	    }
	}
	close(startdir);
    }
    return 0;
}

/*
 * Local variables:
 *  c-basic-offset: 4
 * End:
 */
