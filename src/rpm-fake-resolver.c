// $Id: rpm-fake-resolver.c 2403 2006-11-24 23:06:08Z dhozac $    --*- c -*--

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


// Protocol:
// 1. startup
// 2. initialize (setuid, ctx-migrate, chroot, ...)
// 3. send "." token to fd 3
// 4. wait one character on fd 1
// 5. process this character and consume further characters from fd 1 as far
//    as needed
// 6. go to 3) (or exit)

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "internal.h"
#include "vserver.h"
#include "util.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <grp.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>

#define ENSC_WRAPPERS_PREFIX	"rpm-fake-resolver: "
#define ENSC_WRAPPERS_VSERVER	1
#define ENSC_WRAPPERS_UNISTD	1
#define ENSC_WRAPPERS_IO	1
#define ENSC_WRAPPERS_FCNTL	1
#include <wrappers.h>

#define MAX_RQSIZE	0x1000

int wrapper_exit_code = 1;

struct ArgInfo {
    xid_t		ctx;
    uid_t		uid;
    gid_t		gid;
    bool		do_fork;
    bool		in_ctx;
    char const *	pid_file;
    char const *	chroot;
    uint32_t		caps;
    int			flags;
};

static struct option const
CMDLINE_OPTIONS[] = {
  { "help",     no_argument,  0, 'h' },
  { "version",  no_argument,  0, 'v' },
  { 0,0,0,0 }
};

static void
showHelp(int fd, char const *cmd, int res)
{
  WRITE_MSG(fd, "Usage:  ");
  WRITE_STR(fd, cmd);
  WRITE_MSG(fd,
	    " [-c <ctx>] [-u <uid>] [-g <gid>] [-r <chroot>] [-s] [-n]\n"
	    "Please report bugs to " PACKAGE_BUGREPORT "\n");
  exit(res);
}

static void
showVersion()
{
  WRITE_MSG(1,
	    "rpm-fake-resolver " VERSION " -- NSS resovler for rpm-fake\n"
	    "This program is part of " PACKAGE_STRING "\n\n"
	    "Copyright (C) 2003 Enrico Scholz\n"
	    VERSION_COPYRIGHT_DISCLAIMER);
  exit(0);
}


inline static void
parseArgs(struct ArgInfo *args, int argc, char *argv[])
{
  while (1) {
    int		c = getopt_long(argc, argv, "F:C:c:u:g:r:ns", CMDLINE_OPTIONS, 0);
    if (c==-1) break;

    switch (c) {
      case 'h'		:  showHelp(1, argv[0], 0);
      case 'v'		:  showVersion();

      case 'c'		:  args->ctx     = atoi(optarg); break;
      case 'u'		:  args->uid     = atoi(optarg); break;
      case 'g'		:  args->gid     = atoi(optarg); break;
      case 'F'		:  args->flags   = atoi(optarg); break;
      case 'C'		:  args->caps    = atoi(optarg); break;
      case 'r'		:  args->chroot  = optarg;   break;
      case 'n'		:  args->do_fork = false;    break;
      case 's'		:  args->in_ctx  = true;     break;
      default		:
	WRITE_MSG(2, "Try '");
	WRITE_STR(2, argv[0]);
	WRITE_MSG(2, " --help' for more information.\n");
	exit(1);
	break;
    }
  }

  if (optind!=argc) {
    WRITE_MSG(2, "No further options allowed; aborting ...\n");
    exit(1);
  }
  
  if (args->chroot==0) {
    WRITE_MSG(2, "No chroot specified; aborting...\n");
    exit(1);
  }
}

static void
sendResult(bool state, uint32_t res)
{
  if (state) {
    static uint8_t	ONE = 1;
    Ewrite(1, &ONE, sizeof ONE);
  }
  else {
    static uint8_t	ZERO = 0;
    Ewrite(1, &ZERO, sizeof ZERO);
  }

  Ewrite(1, &res, sizeof res);
}

static void
do_getpwnam()
{
  uint32_t	len;

  if (EreadAll(0, &len, sizeof len) &&
      len<MAX_RQSIZE) {
    char		buf[len+1];
    struct passwd *	res = 0;
    
    if (EreadAll(0, buf, len)) {
      buf[len] = '\0';
      res = getpwnam(buf);
    }
    
    if (res!=0) sendResult(true,  res->pw_uid);
    else        sendResult(false, -1);
  }
  // TODO: logging
}

static void
do_getgrnam()
{
  uint32_t	len;

  if (EreadAll(0, &len, sizeof len) &&
      len<MAX_RQSIZE) {
    char		buf[len+1];
    struct group *	res = 0;
    
    if (EreadAll(0, buf, len)) {
      buf[len] = '\0';
      res = getgrnam(buf);
    }
    
    if (res!=0) sendResult(true,  res->gr_gid);
    else        sendResult(false, -1);
  }
  // TODO: logging
}

static void
do_closenss()
{
  uint8_t	what;

  if (EreadAll(0, &what, sizeof what)) {
    switch (what) {
      case 'p'	:  endpwent(); break;
      case 'g'	:  endgrent(); break;
      default	:  break;
    }
  }
}

static void
run()
{
  uint8_t	c;

  while (EwriteAll(3, ".", 1),
	 EreadAll (0, &c, sizeof c)) {
    switch (c) {
      case 'P'	:  do_getpwnam(); break;
      case 'G'	:  do_getgrnam(); break;
      case 'Q'	:  exit(0);
      case 'C'	:  do_closenss(); break;
      case '.'	:  Ewrite(1, ".", 1); break;
      default	:  Ewrite(1, "?", 1); break;
    }
  }
}

static void
daemonize(struct ArgInfo const UNUSED * args, int pid_fd)
{
  int		p[2];
  pid_t		pid;
  char		c;
  
  Epipe(p);
  pid = Efork();
  
  if (pid!=0) {
    if (pid_fd!=-1) {
      char	buf[sizeof(id_t)*3 + 2];
      size_t	l;

      l = utilvserver_fmt_uint(buf, pid);
      Ewrite(pid_fd, buf, l);
      Ewrite(pid_fd, "\n", 1);
    }
    _exit(0);
  }
  Eclose(p[1]);
  TEMP_FAILURE_RETRY(read(p[0], &c, 1));
  Eclose(p[0]);
}

static void
activateContext(xid_t xid, bool in_ctx,
		uint32_t UNUSED xid_caps, int UNUSED xid_flags)
{
  if (in_ctx) {
    struct vc_ctx_flags		flags = {
      .flagword = 0,
      .mask     = VC_VXF_STATE_SETUP,
    };

    Evc_set_cflags(xid, &flags);
  }
  else if (vc_isSupported(vcFEATURE_MIGRATE))
      Evc_ctx_migrate(xid);
  else {
#ifdef VC_ENABLE_API_COMPAT
    Evc_new_s_context(xid, xid_caps, xid_flags);
#else
    WRITE_MSG(2, ENSC_WRAPPERS_PREFIX "can not change context: migrate kernel feature missing and 'compat' API disabled\n");
    exit(wrapper_exit_code);
#endif
  }
}

int main(int argc, char * argv[])
{
  struct ArgInfo	args = {
    .ctx      = VC_DYNAMIC_XID,
    .uid      = 99,
    .gid      = 99,
    .do_fork  = true,
    .pid_file = 0,
    .chroot   = 0,
    .in_ctx   = false,
    .flags    = S_CTX_INFO_LOCK,
  };
  int			pid_fd = -1;

#ifndef __dietlibc__
#  warning  *** rpm-fake-resolver is built against glibc; please do not report errors before trying a dietlibc version ***
  WRITE_MSG(2,
	    "***  rpm-fake-resolver was built with glibc;  please do  ***\n"
	    "***  not report errors before trying a dietlibc version. ***\n");
#endif

  parseArgs(&args, argc, argv);
  if (args.pid_file && args.do_fork)
    pid_fd = EopenD(args.pid_file, O_CREAT|O_WRONLY, 0644);
  
  if (args.chroot) Echroot(args.chroot);
  Echdir("/");

  activateContext(args.ctx, args.in_ctx, args.caps, args.flags);
  Esetgroups(0, &args.gid);
  Esetgid(args.gid);
  Esetuid(args.uid);

  if (args.do_fork) daemonize(&args, pid_fd);
  if (pid_fd!=-1)   close(pid_fd);
  run();
}
