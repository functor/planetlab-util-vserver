/*
 * Marc E. Fiuczynski <mef@cs.princeton.edu>
 *
 * Copyright (c) 2004  The Trustees of Princeton University (Trustees).
 *
 * Portions of this file are derived from the Paul Brett's vserver.c
 * modification to bash (vsh).
 *
 * It is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * It is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with  Poptop; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "linuxcaps.h"
#include "vserver.h"

/* Null byte made explicit */
#define NULLBYTE_SIZE                    1

/* Base for all vserver roots for chroot */
#define VSERVER_ROOT_BASE       "/vservers"


/* Change to root:root (before entering new context) */
static int setuidgid_root()
{
    if (setgid(0) < 0) {
        fprintf(stderr, "setgid error\n");
        return -1;
    }
    if (setuid(0) < 0) {
        fprintf(stderr, "setuid error\n");
        return -1;
    }
    return 0;
}

static void compute_new_root(char *base, char **root, uid_t uid)
{
    int             root_len;
    struct passwd   *pwd;

    if ((pwd = getpwuid(uid)) == NULL) {
        perror("vserver: getpwuid error ");
        exit(1);
    }

    root_len = 
        strlen(base) + strlen("/") +
        strlen(pwd->pw_name)      + NULLBYTE_SIZE;
    (*root) = (char *)malloc(root_len);
    if ((*root) == NULL) {
        perror("vserver: malloc error ");
        exit(1);
    }
    
    sprintf((*root), "%s/%s", base, pwd->pw_name);
    (*root)[root_len - 1] = '\0';
}

/* Example: sandbox_root = /vservers/bnc, relpath = /proc/1 */
static int sandbox_file_exists(char *sandbox_root, char *relpath)
{
    struct stat stat_buf;
    char   *file;
    int    len, exists = 0;

    len = strlen(sandbox_root) + strlen(relpath) + NULLBYTE_SIZE;
    if ((file = (char *)malloc(len)) == NULL) {
        perror("vserver: malloc error ");
        exit(1);
    }
    sprintf(file, "%s%s", sandbox_root, relpath);
    file[len - 1] = '\0';
    if (stat(file, &stat_buf) == 0) {
        exists = 1;
    }


    free(file);
    return exists;
}

static int proc_mounted(char *sandbox_root)
{
    return sandbox_file_exists(sandbox_root, "/proc/1");
}

static int devpts_mounted(char *sandbox_root)
{
    return sandbox_file_exists(sandbox_root, "/dev/pts/0");
}

static void mount_proc(char *sandbox_root,uid_t uid)
{
    char        *source = "/proc";
    char        *target;
    int         len;

    len = strlen(sandbox_root) + strlen("/") + strlen("proc") + NULLBYTE_SIZE;
    if ((target = (char *)malloc(len)) == NULL) {
        perror("vserver: malloc error ");
        exit(1);
    }

    sprintf(target, "%s/proc", sandbox_root);
    target[len - 1] = '\0';
    if (!proc_mounted(sandbox_root))
        mount(source, target, "proc", MS_RDONLY, NULL);

    free(target);
}

static void mount_devpts(char *sandbox_root)
{
    char        *source = "/dev/pts";
    char        *target;
    int         len;
    
    len = strlen(sandbox_root) + strlen("/") + strlen("dev/pts") + NULLBYTE_SIZE;
    if ((target = (char *)malloc(len)) == NULL) {
        perror("vserver: malloc error ");
        exit(1);
    }

    sprintf(target, "%s/dev/pts", sandbox_root);
    target[len - 1] = '\0';
    if (!devpts_mounted(sandbox_root))
        mount(source, target, "devpts", 0, NULL);

    free(target);
}

static int sandbox_chroot(uid_t uid)
{
    char *sandbox_root = NULL;

    compute_new_root(VSERVER_ROOT_BASE,&sandbox_root, uid);
    mount_proc(sandbox_root,uid);
    mount_devpts(sandbox_root);
    if (chroot(sandbox_root) < 0) {
        fprintf(stderr,"vserver: chroot error (%s): ",sandbox_root);
	perror("");
        exit(1);
    }
    if (chdir("/") < 0) {
        perror("vserver: chdir error ");
        exit(1);
    }
    return 0;
}

static int sandbox_processes(uid_t uid)
{
	int      context;
	int      flags;
	unsigned remove_cap;

	/* Unique context */
	context = uid;

	flags = 0;
	flags |= 1; /* VX_INFO_LOCK -- cannot request a new vx_id */
	flags |= 4; /* VX_INFO_NPROC -- limit number of procs in a context */

	remove_cap = /* NOTE: keep in sync with chcontext.c */
	  (1<<CAP_LINUX_IMMUTABLE)|
	  (1<<CAP_NET_BROADCAST)|
	  (1<<CAP_NET_ADMIN)|
	  (1<<CAP_NET_RAW)|
	  (1<<CAP_IPC_LOCK)|
	  (1<<CAP_IPC_OWNER)|
	  (1<<CAP_SYS_MODULE)|
	  (1<<CAP_SYS_RAWIO)|
	  (1<<CAP_SYS_PACCT)|
	  (1<<CAP_SYS_ADMIN)|
	  (1<<CAP_SYS_BOOT)|
	  (1<<CAP_SYS_NICE)|
	  (1<<CAP_SYS_RESOURCE)|
	  (1<<CAP_SYS_TIME)|
	  (1<<CAP_MKNOD)|
#ifdef CAP_QUOTACTL
	  (1<<CAP_QUOTACTL)|
#endif
#ifdef CAP_CONTEXT
	  (1<<CAP_CONTEXT)|
#endif
	  0
	  ;

    if (vc_new_s_context(context,remove_cap,flags) < 0) {
        perror("vserver: new_s_context error ");
        exit(1);
    }
    return 0;
}


void runas_slice_user(char *username)
{
    struct passwd *pwd;
    char          *home_env, *logname_env, *mail_env, *shell_env, *user_env;
    int           home_len, logname_len, mail_len, shell_len, user_len;
    static char   *envp[10];

    if ((pwd = getpwnam(username)) == NULL) {
        perror("vserver: getpwnam error ");
        exit(1);
    }

    if (setgid(pwd->pw_gid) < 0) {
        perror("vserver: setgid error ");
        exit(1);
    }

    if (setuid(pwd->pw_uid) < 0) {
        perror("vserver: setuid error ");
        exit(1);
    }

    if (chdir(pwd->pw_dir) < 0) {
        perror("vserver: chdir error ");
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
        perror("vserver: malloc error ");
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
        perror("vserver: putenv error ");
        exit(1);
    }
}



void slice_enter(char *context)
{
    struct passwd   *pwd;
    uid_t uid;

    if ((pwd = getpwnam(context)) == NULL) {
        fprintf(stderr,"vserver: getpwname(%s) failed",context);
        exit(2);
    }

    context = (char*)malloc(strlen(pwd->pw_name)+NULLBYTE_SIZE);
    if (!context) {
        perror("vserver: malloc failed");
        exit(2);
    }
    strcpy(context,pwd->pw_name);

    if (setuidgid_root() < 0) { /* For chroot, new_s_context */
        fprintf(stderr,"vserver: Could not setuid/setguid to root:root\n");
        exit(2);
    }

    uid = pwd->pw_uid;
    if (sandbox_chroot(uid) < 0) {
	fprintf(stderr, "vserver: Could not chroot to vserver root\n");
        exit(2);
    }

    if (sandbox_processes(uid) < 0) {
	fprintf(stderr, "vserver: Could not sandbox processes in vserver\n");
        exit(2);
    }
}
