// $Id: unify-deunify.c,v 1.2 2004/02/18 04:48:24 ensc Exp $    --*- c -*--

// Copyright (C) 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
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

#include "unify.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>
#include <fcntl.h>
#include <errno.h>

#define ENSC_WRAPPERS_IO	1
#include <wrappers.h>

bool
Unify_deUnify(char const *dst)
{
  size_t		l = strlen(dst);
  char			tmpfile[l + sizeof(";XXXXXX")];
  int			fd_src, fd_tmp;
  struct stat		st;
  struct utimbuf	utm;

  fd_src = open(dst, O_RDONLY);
  if (fd_src==-1) {
    perror("open()");
    return false;
  }

  if (fstat(fd_src, &st)==-1) {
    perror("fstat()");
    close(fd_src);
    return false;
  }
  
  memcpy(tmpfile,   dst, l);
  memcpy(tmpfile+l, ";XXXXXX", 8);
  fd_tmp = mkstemp(tmpfile);

  if (fd_tmp==-1) {
    perror("mkstemp()");
    tmpfile[0] = '\0';
    goto err;
  }

  if (fchown(fd_tmp, st.st_uid, st.st_gid)==-1 ||
      fchmod(fd_tmp, st.st_mode)==-1) {
    perror("fchown()/fchmod()");
    goto err;
  }

  // todo: acl?

  for (;;) {
    char	buf[0x4000];
    ssize_t	len = read(fd_src, buf, sizeof buf);
    if (len==-1) {
      perror("read()");
      goto err;
    }
    if (len==0) break;

    if (!WwriteAll(fd_tmp, buf, len, 0)) goto err;
  }

  if (close(fd_src)==-1) {
    perror("close()");
    goto err;
  }
  if (close(fd_tmp)==-1) {
    perror("close()");
    goto err;
  }
  
  utm.actime  = st.st_atime;
  utm.modtime = st.st_mtime;

  // ALERT: race !!!
  if (utime(tmpfile, &utm)==-1) {
    perror("utime()");
    goto err1;
  }

  if (unlink(dst)==-1) {
    perror("unlink()");
    goto err1;
  }
  
  // ALERT: race !!!
  if (rename(tmpfile, dst)==-1) {
    perror("FATAL error in rename()");
    _exit(1);
  }

  return true;
  
  err:
  close(fd_src);
  close(fd_tmp);
  err1:
  if (tmpfile[0]) unlink(tmpfile);

  return false;
}
