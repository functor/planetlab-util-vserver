// $Id: wrappers-unistd.hc 2467 2007-01-21 18:26:45Z dhozac $    --*- c -*--

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

#ifndef H_ENSC_IN_WRAPPERS_H
#  error wrappers_handler.hc can not be used in this way
#endif

inline static WRAPPER_DECL void
Eclose(int s)
{
  FatalErrnoError(close(s)==-1, "close()");
}

inline static WRAPPER_DECL void
Echdir(char const path[])
{
  FatalErrnoError(chdir(path)==-1, "chdir()");
}

inline static WRAPPER_DECL void
Efchdir(int fd)
{
  FatalErrnoError(fchdir(fd)==-1, "fchdir()");
}

inline static WRAPPER_DECL void
Echroot(char const path[])
{
  FatalErrnoError(chroot(path)==-1, "chroot()");
}

inline static WRAPPER_DECL NORETURN void
Eexecv(char const *path, char *argv[])
{
  execv(path,argv);
  FatalErrnoErrorFail("execv()");
}

inline static WRAPPER_DECL NORETURN void
Eexecvp(char const *path, char *argv[])
{
  execvp(path,argv);
  FatalErrnoErrorFail("execvp()");
}

inline static WRAPPER_DECL NORETURN void
EexecvpD(char const *path, char *argv[])
{
  execvp(path,argv);
  {
    ENSC_DETAIL1(msg, "execvp", path, 1);
    FatalErrnoErrorFail(msg);
  }
}

inline static WRAPPER_DECL void
Epipe(int filedes[2])
{
  FatalErrnoError(pipe(filedes)==-1, "pipe()");
}

inline static WRAPPER_DECL pid_t
Efork()
{
  pid_t		res;
  res = fork();
  FatalErrnoError(res==-1, "fork()");
  return res;
}

inline static WRAPPER_DECL size_t
Eread(int fd, void *ptr, size_t len)
{
  ssize_t	res = read(fd, ptr, len);
  FatalErrnoError(res==-1, "read()");

  return res;
}

inline static WRAPPER_DECL size_t
Ewrite(int fd, void const *ptr, size_t len)
{
  ssize_t	res = write(fd, ptr, len);
  FatalErrnoError(res==-1, "write()");

  return res;
}

inline static WRAPPER_DECL size_t
Ereadlink(const char *path, char *buf, size_t bufsiz)
{
  ssize_t	res = readlink(path, buf, bufsiz);
  FatalErrnoError(res==-1, "readlink()");

  return res;
}

inline static WRAPPER_DECL size_t
EreadlinkD(const char *path, char *buf, size_t bufsiz)
{
  ssize_t	res = readlink(path, buf, bufsiz);
  ENSC_DETAIL1(msg, "readlink", path, 1);
  FatalErrnoError((ssize_t)(res)==-1, msg);

  return res;
}

inline static WRAPPER_DECL void
Esymlink(const char *oldpath, const char *newpath)
{
  FatalErrnoError(symlink(oldpath, newpath)==-1, "symlink()");
}

inline static WRAPPER_DECL void
EsymlinkD(const char *oldpath, const char *newpath)
{
  ENSC_DETAIL2(msg, "symlink", oldpath, newpath, 1, 1);
  FatalErrnoError(symlink(oldpath, newpath)==-1, msg);
}

inline static WRAPPER_DECL void
Eunlink(char const *pathname)
{
  FatalErrnoError(unlink(pathname)==-1, "unlink()");
}

inline static WRAPPER_DECL void
Elink(char const *oldpath, char const *newpath)
{
  FatalErrnoError(link(oldpath, newpath)==-1, "link()");
}

inline static void
Esetuid(uid_t uid)
{
  FatalErrnoError(setuid(uid)==-1, "setuid()");
}

inline static void
Esetgid(gid_t gid)
{
  FatalErrnoError(setgid(gid)==-1, "setgid()");
}

#if defined(_GRP_H) && (defined(__USE_BSD) || defined(__dietlibc__))
inline static void
Esetgroups(size_t size, const gid_t *list)
{
  FatalErrnoError(setgroups(size, list)==-1, "setgroups()");
}

inline static void
Einitgroups(const char *user, gid_t group)
{
  FatalErrnoError(initgroups(user, group)==-1, "initgroups()");
}
#endif

inline static WRAPPER_DECL int
Edup2(int oldfd, int newfd)
{
  register int          res = dup2(oldfd, newfd);
  FatalErrnoError(res==-1, "dup2()");

  return res;
}

inline static WRAPPER_DECL int
Edup(int fd)
{
  register int          res = dup(fd);
  FatalErrnoError(res==-1, "dup()");

  return res;
}

inline static WRAPPER_DECL pid_t
Esetsid()
{
  register pid_t const  res = setsid();
  FatalErrnoError(res==-1, "setsid()");

  return res;
}

inline static WRAPPER_DECL int
Emkstemp(char *template)
{
  int		res = mkstemp(template);
  FatalErrnoError(res==-1, "mkstemp()");
  return res;
}

inline static WRAPPER_DECL off_t
Elseek(int fildes, off_t offset, int whence)
{
  off_t         res = lseek(fildes, offset, whence);
  FatalErrnoError(res==(off_t)-1, "lseek()");
  return res;
}

inline static WRAPPER_DECL void
Enice(int n)
{
  FatalErrnoError(nice(n)==-1, "nice()");
}

inline static WRAPPER_DECL void
Etruncate(const char *path, off_t length)
{
  FatalErrnoError(truncate(path,length)==-1, "truncate()");
}

inline static WRAPPER_DECL void
Eftruncate(int fd, off_t length)
{
  FatalErrnoError(ftruncate(fd,length)==-1, "ftruncate()");
}
