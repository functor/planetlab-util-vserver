// $Id: syscall_setiattr-fscompat.hc 2151 2005-07-15 18:06:27Z ensc $    --*- c -*--

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

#include "ioctl-setext2flags.hc"
#include "ioctl-setfilecontext.hc"
#include "ioctl-setxflg.hc"
#include "ioctl-getxflg.hc"

#include <fcntl.h>

static inline ALWAYSINLINE int
vc_set_iattr_fscompat(char const *filename,
		      xid_t xid,
		      uint32_t flags, uint32_t mask)
{
  int			fd;
  struct stat		st;
  int			stat_rc;

  fd = open(filename, O_RDONLY|O_NONBLOCK);
  if (fd==-1) return -1;
    
  stat_rc = fstat(fd, &st);
  if (stat_rc==-1) goto err;

  if ( (mask&VC_IATTR_IUNLINK) ) {
    unsigned int const	tmp = VC_IMMUTABLE_FILE_FL|VC_IMMUTABLE_LINK_FL;
    if (vc_X_set_ext2flags(fd,
			   (flags&VC_IATTR_IUNLINK) ? tmp : 0,
			   (flags&VC_IATTR_IUNLINK) ? 0   : tmp)==-1)
      goto err;
  }

  if ( (mask&VC_IATTR_BARRIER) ) {
    if ((flags&VC_IATTR_BARRIER)) {
      if (vc_X_set_ext2flags(fd, VC_IMMUTABLE_LINK_FL, 0)==-1 ||
	  fchmod(fd, 0))
	goto err;
    }
    else {
      if (vc_X_set_ext2flags(fd, 0, VC_IMMUTABLE_LINK_FL)==-1 ||
	  fchmod(fd, 0500))
	goto err;
    }      
  }

  if ( (mask&VC_IATTR_XID) &&
       vc_X_set_filecontext(fd, xid)==-1)
    goto err;

  if ( (mask&(VC_IATTR_HIDE|VC_IATTR_WATCH)) ) {
    long		tmp;
    if (vc_X_get_xflg(fd, &tmp)==-1) goto err;

    tmp &= ~( ((mask&VC_IATTR_HIDE)   ? 1 : 0) |
	      ((mask&VC_IATTR_WATCH) ? 2 : 0) );
    tmp |= ( ((flags&VC_IATTR_HIDE)  ? 1 : 0) |
	     ((flags&VC_IATTR_WATCH) ? 2 : 0) );

    if (vc_X_set_xflg(fd, tmp)==-1) goto err;
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
