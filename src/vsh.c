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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_SHELL "/bin/sh"

/* Exit statuses for programs like 'env' that exec other programs.
   EXIT_FAILURE might not be 1, so use EXIT_FAIL in such programs.  */
enum
{
  EXIT_CANNOT_INVOKE = 126,
  EXIT_ENOENT = 127
};

extern void slice_enter(char *);
extern void runas_slice_user(char *);

int main(int argc, char **argv)
{
    char *context, *username, *shell;
    struct passwd   *pwd;
    uid_t           uid;
    int index, i;

    if (argv[0][0]=='-') 
      index = 1;
    else
      index = 0;

    uid = getuid();
    if ((pwd = getpwuid(uid)) == NULL) {
      fprintf(stderr,"vsh: getpwnam error failed for %d\n",uid); 
      exit(1);
    }

    context = (char*)strdup(pwd->pw_name);
    if (!context) {
      perror("vsh: strdup failed");
      exit(2);
    }

    /* enter vserver "context" */
    slice_enter(context);

    /* Now run as username in this context. Note that for PlanetLab's
       vserver configuration the context name also happens to be the
       "default" username within the vserver context.
    */
    username = context;
    runas_slice_user(username);

    /* With the uid/gid appropriately set. Let's figure out what the
     * shell in the vserver's /etc/passwd is for the given username.
     */
    if ((pwd = getpwnam(username)) == NULL) {
        fprintf(stderr,"vsh: getpwnam error failed for %s\n",username); 
        exit(1);
    }

    /* Make sure pw->pw_shell is non-NULL.*/
    if (pwd->pw_shell == NULL || pwd->pw_shell[0] == '\0') {
      pwd->pw_shell = (char *) DEFAULT_SHELL;
    }

    shell = (char *)strdup(pwd->pw_shell);
    if (!shell) {
      perror("vsh: strdup failed");
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
	perror("vsh: malloc failed");
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
