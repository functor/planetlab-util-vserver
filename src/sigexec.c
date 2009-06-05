// $Id: sigexec.c 1562 2004-05-11 18:32:58Z ensc $    --*- c -*--

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
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define ENSC_WRAPPERS_PREFIX	"sigexec: "
#define ENSC_WRAPPERS_UNISTD	1
#include <wrappers.h>

int	wrapper_exit_code = 255;

static void
showHelp(int fd, char const *cmd, int res)
{
  WRITE_MSG(fd, "Usage:  ");
  WRITE_STR(fd, cmd);
  WRITE_MSG(fd,
	    " [--] <cmd> <args>*\n\n"
	    "Please report bugs to " PACKAGE_BUGREPORT "\n");
  exit(res);
}

static void
showVersion()
{
  WRITE_MSG(1,
	    "sigexec " VERSION " -- executes commands with defined sigmasks\n"
	    "This program is part of " PACKAGE_STRING "\n\n"
	    "Copyright (C) 2004 Enrico Scholz\n"
	    VERSION_COPYRIGHT_DISCLAIMER);
  exit(0);
}


int main(int argc, char *argv[])
{
  int		idx = 1;
  
  if (argc>=2) {
    if (strcmp(argv[1], "--help")   ==0) showHelp(1, argv[0], 0);
    if (strcmp(argv[1], "--version")==0) showVersion();
    if (strcmp(argv[1], "--")       ==0) ++idx;
  }

  if (argc<idx+1) {
    WRITE_MSG(2, "Not enough parameters; use '--help' for more information\n");
   return wrapper_exit_code;
  }

  signal(SIGCHLD, SIG_DFL);

  Eexecv(argv[idx], argv+idx);
}
