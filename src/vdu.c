// $Id: vdu-new.c,v 1.2 2004/08/17 14:44:14 mef-pl_kernel Exp $

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

#include <assert.h>

#include "vdu.h"

HashTable tbl;

static int // boolean
INOPut(PHashTable tbl, ino_t* key, struct stat **val){
    return Put(tbl, key, val);
}

__extension__ typedef long long		longlong;
//__extension__ typedef long		longlong;

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
    char const *foo = path;
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
     * blocks, and disk space used. This code will recursively descend
     * down the directory structure. 
     */

    while ((ent=readdir(dir))!=NULL){
	if (lstat(ent->d_name,&st)==-1){
	    fprintf (stderr,"Can't stat %s/%s\n",path,ent->d_name);
	    warning("lstat failed");
	    continue;
	}
	
	dirinodes ++;

	if (S_ISREG(st.st_mode)){
	    if (st.st_nlink > 1){
		struct stat *val;
		int nlink;

		/* Check hash table if we've seen this inode
		 * before. Note that the hash maintains a
		 * (inode,struct stat) key value pair.
		 */

		val = &st;

		(void) INOPut(&tbl,&st.st_ino,&val);

		/* Note that after the INOPut call "val" refers to the
		 * value entry in the hash table --- not &st.  This
		 * means that if the inode has been put into the hash
		 * table before, val will refer to the first st that
		 * was put into the hashtable.  Otherwise, if it is
		 * the first time it is put into the hash table, then
		 * val will be equal to this &st.
		 */
		nlink = val->st_nlink;
		nlink --;

		/* val refers to value in hash tbale */
		if (nlink == 0) {

		    /* We saw all hard links to this particular inode
		     * as part of this sweep of vdu. So account for
		     * the size and blocks required by the file.
		     */

		    dirsize += val->st_size;
		    dirblocks += val->st_blocks;

		    /* Do not delete the (ino,val) tuple from the tbl,
		     * as we need to handle the case when we are
		     * double counting a file due to a bind mount.
		     */
		    val->st_nlink = 0;

		} else if (nlink > 0) {
		    val->st_nlink = nlink;
		} else /* if(nlink < 0) */ {
		    /* We get here when we are double counting nlinks
		       due a bind mount. */

		    /* DO NOTHING */
		}
	    } else {
		dirsize += st.st_size;
		dirblocks += st.st_blocks;
	    }

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
	    // dirsize += st.st_size;
	    // dirblocks += st.st_blocks;
	}
    }
    closedir (dir);
    close (dirfd);
    if (verbose)
	printf("%16lld %16lld %16lld %s\n",dirinodes, dirblocks, dirsize,foo);
    inodes += dirinodes;
    blocks += dirblocks;
    size   += dirsize;
}

static void
Count(ino_t* key, struct stat* val) {
    if(val->st_nlink) {
	blocks += val->st_blocks;
	size += val->st_size;
	printf("ino=%ld nlink=%d\n",val->st_ino, val->st_nlink);
    }
}

int
main (int argc, char **argv)
{
    int startdir, i;

    if (argc < 2){
	fprintf (stderr,"vdu version %s\n",VERSION);
	fprintf (stderr,"vdu directory ...\n\n");
	fprintf (stderr,"Compute the size of a directory tree.\n");
    }else{
	if ((startdir = open (".",O_RDONLY)) == -1) {
	    fprintf (stderr,"Can't open current working directory\n");
	    panic("open failed");
	}

	/* hash table support for hard link count */
	(void) Init(&tbl,0,0);

	for (i=1; i<argc; i++){
	    inodes = blocks = size = 0;
	    vdu_onedir (argv[i]);

	    printf("%16lld %16lld %16lld %s\n",
		   inodes, 
		   blocks>>1, 
		   size,
		   argv[i]);
	    if (fchdir (startdir) == -1) {
		panic("fchdir failed");
	    }
	}

	if(0) {
	    /* show size & blocks for files with nlinks from outside of dir */
	    inodes = blocks = size = 0;
	    Iterate(&tbl,Count);
	    printf("%16lld %16lld %16lld NOT COUNTED\n",
		   inodes, 
		   blocks, 
		   size);
	}

	// Dispose(&tbl); this fails to delete all entries 
	close(startdir);
    }
    return 0;
}

/*
 * Local variables:
 *  c-basic-offset: 4
 * End:
 */
