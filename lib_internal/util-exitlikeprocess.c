// $Id: util-exitlikeprocess.c,v 1.4 2005/03/22 14:59:46 ensc Exp $    --*- c -*--

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

#include "util.h"
#include <lib/internal.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <stdio.h>

void
exitLikeProcess(int pid, char const *cmd, int ret)
{
  int			status;
  
  if (wait4(pid, &status, 0,0)==-1) {
    
    perror("wait()");
    exit(ret);
  }

  if (WIFEXITED(status))
    exit(WEXITSTATUS(status));

  if (WIFSIGNALED(status)) {
    struct rlimit	lim = { 0,0 };

    if (cmd) {
      char		buf[sizeof(int)*3 + 2];
      size_t		l = utilvserver_fmt_uint(buf, pid);
      
      WRITE_MSG(2, "command '");
      WRITE_STR(2, cmd);
      WRITE_MSG(2, "' (pid ");
      Vwrite   (2, buf, l);
      WRITE_MSG(2, ") exited with signal ");
      l = utilvserver_fmt_uint(buf, WTERMSIG(status));      
      Vwrite   (2, buf, l);
      WRITE_MSG(2, "; following it...\n");
    }

    // prevent coredumps which might override the real ones
    setrlimit(RLIMIT_CORE, &lim);
      
    kill(getpid(), WTERMSIG(status));
    exit(1);
  }
  else {
    char		buf[sizeof(int)*3 + 2];
    size_t		l = utilvserver_fmt_uint(buf, WTERMSIG(status));

    WRITE_MSG(2, "Unexpected status ");
    Vwrite   (2, buf, l);
    WRITE_MSG(2, " from '");
    if (cmd) {
      WRITE_STR(2, cmd);
      WRITE_MSG(2, " (pid ");
    }
    l = utilvserver_fmt_uint(buf, pid);
    Vwrite   (2, buf, l);
    if (cmd) WRITE_MSG(2, ")\n");
    else     WRITE_MSG(2, "\n");

    exit(ret);
  }
}
