// $Id: vkill.c,v 1.8 2004/08/19 14:30:50 ensc Exp $    --*- c -*--

// Copyright (C) 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
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

#include "vserver.h"
#include "linuxvirtual.h"
#include "util.h"

#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define ENSC_WRAPPERS_VSERVER	1
#define ENSC_WRAPPERS_UNISTD	1
#include <wrappers.h>

#define CMD_HELP	0x8000
#define CMD_VERSION	0x8001

int		wrapper_exit_code = 1;

static struct option const
CMDLINE_OPTIONS[] = {
  { "help",     no_argument,        0, CMD_HELP },
  { "version",  no_argument,        0, CMD_VERSION },
  { "xid",      required_argument,  0, 'c' },
  { 0,0,0,0 }
};

struct Arguments
{
    xid_t		xid;
    int			sig;
};

static char const * const SIGNALS[] = {
  // 0      1      2          3       4       5       6       7
  "UNUSED", "HUP", "INT",     "QUIT", "ILL",  "TRAP", "ABRT", "UNUSED",
  "FPE",    "KILL", "USR1",   "SEGV", "USR2", "PIPE", "ALRM", "TERM",
  "STKFLT", "CHLD", "CONT",   "STOP", "TSTP", "TTIN", "TTOU", "IO",
  "XCPU",   "XFSZ", "VTALRM", "PROF", "WINCH",
  0,
};

static void
showHelp(int fd, char const *cmd, int res)
{
  WRITE_MSG(fd, "Usage:  ");
  WRITE_STR(fd, cmd);
  WRITE_MSG(fd,
	    " [--xid|-c <xid>] [-s <signal>] [--] <pid>*\n"
	    "Please report bugs to " PACKAGE_BUGREPORT "\n");
  exit(res);
}

static void
showVersion()
{
  WRITE_MSG(1,
	    "vkill " VERSION " -- sends signals to processes within other contexts\n"
	    "This program is part of " PACKAGE_STRING "\n\n"
	    "Copyright (C) 2003 Enrico Scholz\n"
	    VERSION_COPYRIGHT_DISCLAIMER);
  exit(0);
}

static int
str2sig(char const *str)
{
  char	*errptr;
  int	res = strtol(str, &errptr, 10);
  
  if (*errptr!='\0') res=-1;
  if (res==-1 && strncmp(str,"SIG",3)==0) str+=3;
  if (res==-1) {
    char const * const	*ptr = SIGNALS;
    for (;*ptr!=0; ++ptr) {
      if (strcmp(*ptr,str)!=0) continue;
      res = ptr-SIGNALS;
      break;
    }
  }

  return res;
}

#if defined(VC_ENABLE_API_LEGACY)
inline static ALWAYSINLINE int
kill_wrapper_legacy(xid_t UNUSED xid, char const *proc, int UNUSED sig)
{
  pid_t		pid;

  signal(SIGCHLD, SIG_DFL);
  pid = Efork();

  if (pid==0) {
    int		status;
    int		res;
    while ((res=wait4(pid, &status, 0,0))==-1 &&
	   (errno==EAGAIN || errno==EINTR)) {}

    return (res==0 && WIFEXITED(status) && WEXITSTATUS(status)) ? 0 : 1;
  }

  execl(LEGACYDIR "/vkill", "legacy/vkill", proc, (void *)(0));
  perror("vkill: execl()");
  exit(1);
}

static int
kill_wrapper(xid_t xid, char const *pid, int sig)
{
  //printf("kill_wrapper(%u, %s, %i)\n", xid, pid, sig);
  if (vc_ctx_kill(xid,atoi(pid),sig)==-1) {
    int		err = errno;
    if (vc_get_version(VC_CAT_COMPAT)==-1)
      return kill_wrapper_legacy(xid, pid, sig);
    else {
      errno = err;
      perror("vkill: vc_ctx_kill()");
      return 1;
    }
  }
  
  return 0;
}
#else // VC_ENABLE_API_LEGACY
inline static int
kill_wrapper(xid_t xid, char const *pid, int sig)
{
  if (vc_ctx_kill(xid,atoi(pid),sig)==-1) {
    perror("vkill: vc_ctx_kill()");
    return 1;
  }
  return 0;
}
#endif


int main(int argc, char *argv[])
{
  int			fail = 0;
  struct Arguments	args = {
    .xid = VC_NOCTX,
    .sig = SIGTERM,
  };
  
  while (1) {
    int		c = getopt_long(argc, argv, "c:s:", CMDLINE_OPTIONS, 0);
    if (c==-1) break;

    switch (c) {
      case CMD_HELP	:  showHelp(1, argv[0], 0);
      case CMD_VERSION	:  showVersion();
      case 'c'		:  args.xid = Evc_xidopt2xid(optarg,true); break;
      case 's'		:  args.sig = str2sig(optarg);             break;
      default		:
	WRITE_MSG(2, "Try '");
	WRITE_STR(2, argv[0]);
	WRITE_MSG(2, " --help\" for more information.\n");
	return EXIT_FAILURE;
	break;
    }
  }

  if (args.sig==-1) {
    WRITE_MSG(2, "Invalid signal specified\n");
    return EXIT_FAILURE;
  }

  if (args.xid==VC_NOCTX && optind==argc) {
    WRITE_MSG(2, "No pid specified\n");
    return EXIT_FAILURE;
  }

  if (optind==argc)
    fail += kill_wrapper(args.xid, "0", args.sig);
  else for (;optind<argc;++optind)
    fail += kill_wrapper(args.xid, argv[optind], args.sig);

  return fail==0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#ifdef TESTSUITE
void
vkill_test()
{
  assert(str2sig("0") ==0 );
  assert(str2sig("1") ==1 );
  assert(str2sig("10")==10);
  assert(str2sig("SIGHUP")==1);
  assert(str2sig("HUP")   ==1);
  assert(str2sig("SIGCHLD")==17);
  assert(str2sig("CHLD")   ==17);
  assert(str2sig("x")==-1);
  assert(str2sig("1 0")==-1);

  return 0;
}
#endif
