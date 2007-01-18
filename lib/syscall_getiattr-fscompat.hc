// $Id: syscall_getiattr-fscompat.hc 2151 2005-07-15 18:06:27Z ensc $    --*- c -*--

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

#include "ioctl-getext2flags.hc"
#include "ioctl-getfilecontext.hc"
#include "ioctl-getxflg.hc"

#include <fcntl.h>
static inline ALWAYSINLINE int
vc_get_iattr_fscompat(char const *filename,
		      xid_t    * /*@null@*/ xid,
		      uint32_t * /*@null@*/ flags,
		      uint32_t * mask)
{
  struct stat		st;
  int			stat_rc;
  int			fd;
  int			old_mask = *mask;

  *mask = 0;

  if (lstat(filename, &st)==-1) return -1;
  if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) return 0;

  fd = open(filename, O_RDONLY|O_NONBLOCK);
  if (fd==-1) return -1;

  stat_rc = fstat(fd, &st);
  if (stat_rc==-1) goto err;

  if ( old_mask&VC_IATTR_XID ) {
    *xid = vc_X_get_filecontext(fd);
    if (*xid!=VC_NOCTX) *mask |= VC_IATTR_XID;
  }

  if ( old_mask&VC_IATTR_IUNLINK ) {
    long		tmp;
    int		rc = vc_X_get_ext2flags(fd, &tmp);

    if (rc!=-1) {
      *mask |= VC_IATTR_IUNLINK;
      if (tmp & (VC_IMMUTABLE_FILE_FL|VC_IMMUTABLE_LINK_FL))
	*flags |= VC_IATTR_IUNLINK;
    }
  }

  if ( (old_mask&VC_IATTR_BARRIER) && S_ISDIR(st.st_mode)) {
    long		ext2_flags;
    
    *mask  |= VC_IATTR_BARRIER;
    if ((st.st_mode&0777)==0 &&
	vc_X_get_ext2flags(fd, &ext2_flags)!=-1 &&
	(ext2_flags & VC_IMMUTABLE_LINK_FL))
      *flags |= VC_IATTR_BARRIER;
  }

  if ( (old_mask&(VC_IATTR_WATCH|VC_IATTR_HIDE)) ){
    long		tmp;
    int		rc = vc_X_get_xflg(fd, &tmp);
    if (rc!=-1) {
      *mask |= (VC_IATTR_WATCH|VC_IATTR_HIDE);
      if (tmp&1) *flags |= VC_IATTR_HIDE;
      if (tmp&2) *flags |= VC_IATTR_WATCH;
    }
  }

  close(fd);
  return 0;
  err:
  {
    int	old_errno = errno;
    close(fd);
    errno = old_errno;
    return -1;
  } 
}
