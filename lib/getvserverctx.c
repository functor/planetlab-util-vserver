// $Id: getvserverctx.c,v 1.8 2004/06/27 13:02:07 ensc Exp $    --*- c -*--

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
#include "pathconfig.h"
#include "compat-c99.h"
#include "lib_internal/util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#ifdef VC_ENABLE_API_COMPAT
#include <fcntl.h>

static xid_t
extractLegacyXID(char const *dir, char const *basename)
{
  size_t	l1 = strlen(dir);
  size_t	l2 = strlen(basename);
  char		path[l1 + l2 + sizeof("/.ctx")];
  char *	ptr = path;
  int		fd;
  ssize_t	len;
  xid_t		result = VC_NOXID;

  ptr    = Xmemcpy(ptr, dir, l1);
  *ptr++ = '/';
  ptr    = Xmemcpy(ptr, basename, l2);
  ptr    = Xmemcpy(ptr, ".ctx",    5);

  fd = open(path, O_RDONLY);
  if (fd==-1) return VC_NOXID;

  len = lseek(fd, 0, SEEK_END);

  if (len!=-1 && lseek(fd, 0, SEEK_SET)!=-1) {
    char	buf[len+2];
    char const	*pos = 0;

    buf[0] = '\n';
    
    if (read(fd, buf+1, len+1)==len) {
      buf[len+1] = '\0';
      pos        = strstr(buf, "\nS_CONTEXT=");
    }

    if (pos) pos += 11;
    if (*pos>='1' && *pos<='9')
      result = atoi(pos);
  }

  close(fd);
  return result;
}
#else
static xid_t
extractLegacyXID(char const UNUSED *dir, char const UNUSED *basename)
{
  return VC_NOXID;
}
#endif


static xid_t
getCtxFromFile(char const *pathname)
{
  int		fd;
  off_t		len;

  fd = open(pathname, O_RDONLY);

  if (fd==-1 ||
      (len=lseek(fd, 0, SEEK_END))==-1 ||
      (len>50) ||
      (lseek(fd, 0, SEEK_SET)==-1))
    return VC_NOCTX;

  {
  char		buf[len+1];
  char		*errptr;
  xid_t		res;
  
  if (TEMP_FAILURE_RETRY(read(fd, buf, len+1))!=len)
    return VC_NOCTX;

  res = strtol(buf, &errptr, 10);
  if (*errptr!='\0' && *errptr!='\n') return VC_NOCTX;

  return res;
  }
}

xid_t
vc_getVserverCtx(char const *id, vcCfgStyle style, bool honor_static, bool *is_running)
{
  size_t		l1 = strlen(id);
  char			buf[sizeof(CONFDIR "//") + l1 + sizeof("/context")];
			    
  if (style==vcCFG_NONE || style==vcCFG_AUTO)
    style = vc_getVserverCfgStyle(id);

  if (is_running) *is_running = false;

  switch (style) {
    case vcCFG_NONE		:  return VC_NOCTX;
    case vcCFG_LEGACY		:
      return extractLegacyXID(DEFAULT_PKGSTATEDIR, id);
    case vcCFG_RECENT_SHORT	:
    case vcCFG_RECENT_FULL	: {
      size_t		idx = 0;
      xid_t		res = 0;

      if (style==vcCFG_RECENT_SHORT) {
	memcpy(buf, CONFDIR "/", sizeof(CONFDIR "/")-1);
	idx  = sizeof(CONFDIR "/") - 1;
      }
      memcpy(buf+idx, id, l1);    idx += l1;
      memcpy(buf+idx, "/run", 5);	// appends '\0' too
      
      res = getCtxFromFile(buf);
      if (is_running) *is_running = res!=VC_NOCTX;
      
      if (res==VC_NOCTX && honor_static) {
	memcpy(buf+idx, "/context", 9);	// appends '\0' too

	res = getCtxFromFile(buf);
      }
      
      return res;
    }
    default			:  return VC_NOCTX;
  }
}
