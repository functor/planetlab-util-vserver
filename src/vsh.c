/*
 * Marc E. Fiuczynski <mef@cs.princeton.edu>
 *
 * Copyright (c) 2004 The Trustees of Princeton University (Trustees).
 *
 * vsh is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * vsh is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Poptop; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>

//--------------------------------------------------------------------
#include "vserver.h"
#include "planetlab.h"

#undef CONFIG_VSERVER_LEGACY

/* Base for all vserver roots for chroot */
#define VSERVER_ROOT_BASE       "/vservers"

/* Change to root:root (before entering new context) */
static int setuidgid_root()
{
	if (setgid(0) < 0) {
		PERROR("setgid(0)");
		return -1;
	}
	if (setuid(0) < 0) {
		PERROR("setuid(0)");
		return -1;
	}
	return 0;
}

static void compute_new_root(char *base, char **root, const struct passwd *pwd)
{
	int             root_len;

	root_len = 
		strlen(base) + strlen("/") +
		strlen(pwd->pw_name)      + NULLBYTE_SIZE;
	(*root) = (char *)malloc(root_len);
	if ((*root) == NULL) {
		PERROR("malloc(%d)", root_len);
		exit(1);
	}
    
	sprintf((*root), "%s/%s", base, pwd->pw_name);
	(*root)[root_len - 1] = '\0';
}

static int sandbox_chroot(const struct passwd *pwd)
{
	char *sandbox_root = NULL;

	compute_new_root(VSERVER_ROOT_BASE,&sandbox_root, pwd);
	if (chroot(sandbox_root) < 0) {
		PERROR("chroot(%s)", sandbox_root);
		exit(1);
	}
	if (chdir("/") < 0) {
		PERROR("chdir(/)");
		exit(1);
	}
	return 0;
}

static int sandbox_processes(xid_t ctx, const char *context, const struct passwd *pwd)
{
#ifdef CONFIG_VSERVER_LEGACY
	int	flags;

	flags = 0;
	flags |= 1; /* VX_INFO_LOCK -- cannot request a new vx_id */
	/* flags |= 4; VX_INFO_NPROC -- limit number of procs in a context */

	(void) vc_new_s_context(ctx, 0, flags);

	/* use legacy dirty hack for capremove */
	if (vc_new_s_context(VC_SAMECTX, vc_get_insecurebcaps(), flags) == VC_NOCTX) {
		PERROR("vc_new_s_context(%u, 0x%16llx, 0x%08x)",
		       VC_SAMECTX, vc_get_insecurebcaps(), flags);
		exit(1);
	}
#else
	int  ctx_is_new;
	struct sliver_resources slr;
	char hostname[HOST_NAME_MAX+1];
	pl_get_limits(context,&slr);

	if (gethostname(hostname, sizeof hostname) == -1)
	  {
	    PERROR("gethostname(...)");
	    exit(1);
	  }

	/* check whether the slice has been suspended */
	if (slr.vs_cpu==0)
	  {
	    fprintf(stderr, "*** %s: %s has zero cpu resources and presumably it has been disabled/suspended ***\n", hostname, context);
	    exit(0);
	  }

	(void) (sandbox_chroot(pwd));

        if ((ctx_is_new = pl_chcontext(ctx, ~vc_get_insecurebcaps(),&slr)) < 0)
          {
            PERROR("pl_chcontext(%u)", ctx);
            exit(1);
          }
	if (ctx_is_new)
	  {
	    fprintf(stderr, " *** %s: %s has not been started yet, please check back later ***\n", hostname, context);
	    exit(1);
	  }
#endif
	return 0;
}


void runas_slice_user(struct passwd *pwd)
{
	char          *username = pwd->pw_name;
	char          *home_env, *logname_env, *mail_env, *shell_env, *user_env;
	int           home_len, logname_len, mail_len, shell_len, user_len;
	static char   *envp[10];

	if (setgid(pwd->pw_gid) < 0) {
		PERROR("setgid(%d)", pwd->pw_gid);
		exit(1);
	}

	if (setuid(pwd->pw_uid) < 0) {
		PERROR("setuid(%d)", pwd->pw_uid);
		exit(1);
	}

	if (chdir(pwd->pw_dir) < 0) {
		PERROR("chdir(%s)", pwd->pw_dir);
		exit(1);
	}

	home_len    = strlen("HOME=") + strlen(pwd->pw_dir) + NULLBYTE_SIZE;
	logname_len = strlen("LOGNAME=") + strlen(username) + NULLBYTE_SIZE;
	mail_len    = strlen("MAIL=/var/spool/mail/") + strlen(username) 
		+ NULLBYTE_SIZE;
	shell_len   = strlen("SHELL=") + strlen(pwd->pw_shell) + NULLBYTE_SIZE;
	user_len    = strlen("USER=") + strlen(username) + NULLBYTE_SIZE;

	home_env    = (char *)malloc(home_len);
	logname_env = (char *)malloc(logname_len);
	mail_env    = (char *)malloc(mail_len);
	shell_env   = (char *)malloc(shell_len);
	user_env    = (char *)malloc(user_len);

	if ((home_env    == NULL)  || 
	    (logname_env == NULL)  ||
	    (mail_env    == NULL)  ||
	    (shell_env   == NULL)  ||
	    (user_env    == NULL)) {
		PERROR("malloc");
		exit(1);
	}

	sprintf(home_env, "HOME=%s", pwd->pw_dir);
	sprintf(logname_env, "LOGNAME=%s", username);
	sprintf(mail_env, "MAIL=/var/spool/mail/%s", username);
	sprintf(shell_env, "SHELL=%s", pwd->pw_shell);
	sprintf(user_env, "USER=%s", username);
    
	home_env[home_len - 1]       = '\0';
	logname_env[logname_len - 1] = '\0';
	mail_env[mail_len - 1]       = '\0';
	shell_env[shell_len - 1]     = '\0';
	user_env[user_len - 1]       = '\0';

	envp[0] = home_env;
	envp[1] = logname_env;
	envp[2] = mail_env;
	envp[3] = shell_env;
	envp[4] = user_env;
	envp[5] = 0;

	if ((putenv(home_env)    < 0) ||
	    (putenv(logname_env) < 0) ||
	    (putenv(mail_env)    < 0) ||
	    (putenv(shell_env)   < 0) ||
	    (putenv(user_env)    < 0)) {
		PERROR("vserver: putenv error ");
		exit(1);
	}
}

void slice_enter(struct passwd *pwd)
{
	if (setuidgid_root() < 0) { /* For chroot, new_s_context */
		fprintf(stderr, "vsh: Could not become root, check that SUID flag is set on binary\n");
		exit(2);
	}

#ifdef CONFIG_VSERVER_LEGACY
	(void) (sandbox_chroot(pwd));
#endif

	if (sandbox_processes((xid_t) pwd->pw_uid, pwd->pw_name, pwd) < 0) {
		fprintf(stderr, "vsh: Could not change context to %d\n", pwd->pw_uid);
		exit(2);
	}
}

//--------------------------------------------------------------------

#define DEFAULT_SHELL "/bin/sh"

/* Exit statuses for programs like 'env' that exec other programs.
   EXIT_FAILURE might not be 1, so use EXIT_FAIL in such programs.  */
enum
{
  EXIT_CANNOT_INVOKE = 126,
  EXIT_ENOENT = 127
};

int main(int argc, char **argv)
{
    struct passwd   pwdd, *result, *prechroot, *postchroot = &pwdd;
    char            *context, *username, *shell, *pwdBuffer;
    long            pwdBuffer_len;
    uid_t           uid;
    int             index, i;

    if (argv[0][0]=='-') 
      index = 1;
    else
      index = 0;

    uid = getuid();
    if ((prechroot = getpwuid(uid)) == NULL) {
      PERROR("getpwuid(%d)", uid);
      exit(1);
    }

    context = (char*)strdup(prechroot->pw_name);
    if (!context) {
      PERROR("strdup");
      exit(2);
    }

    /* enter vserver "context" */
    slice_enter(prechroot);

    /* Get the /etc/passwd entry for this user, this time inside
     * the chroot.
     */
    username = context;

    pwdBuffer_len = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (pwdBuffer_len == -1) {
	    PERROR("sysconf(_SC_GETPW_R_SIZE_MAX");
	    exit(1);
    }
    pwdBuffer = (char*)malloc(pwdBuffer_len);
    if (pwdBuffer == NULL) {
	    PERROR("malloc(%d)", pwdBuffer_len);
	    exit(1);
    }

    errno = 0;
    if ((getpwnam_r(username,postchroot,pwdBuffer,pwdBuffer_len, &result) != 0) ||
        (errno != 0) || result != postchroot) {
        PERROR("getpwnam_r(%s)", username);
        exit(1);
    }

    /* Now run as username in this context. Note that for PlanetLab's
       vserver configuration the context name also happens to be the
       "default" username within the vserver context.
    */
    runas_slice_user(postchroot);

    /* Make sure pw->pw_shell is non-NULL.*/
    if (postchroot->pw_shell == NULL || postchroot->pw_shell[0] == '\0') {
      postchroot->pw_shell = (char *) DEFAULT_SHELL;
    }

    shell = (char *)strdup(postchroot->pw_shell);
    if (!shell) {
      PERROR("strdup");
      exit(2);
    }

    /* Check whether 'su' or 'sshd' invoked us as a login shell or
       not; did this above when testing argv[0]=='-'.
    */
    argv[0] = shell;
    if (index == 1) {
      char **args;
      args = (char**)malloc(sizeof(char*)*(argc+2));
      if (!args) {
	PERROR("malloc(%d)", sizeof(char*)*(argc+2));
	exit(1);
      }
      args[0] = argv[0];
      args[1] = "-l";
      for(i=1;i<argc+1;i++) {
	args[i+1] = argv[i];
      }
      argv = args;
    }
    (void) execvp(shell,argv);
    {
      int exit_status = (errno == ENOENT ? EXIT_ENOENT : EXIT_CANNOT_INVOKE);
      exit (exit_status);
    }

    return 0; /* shutup compiler */
}
