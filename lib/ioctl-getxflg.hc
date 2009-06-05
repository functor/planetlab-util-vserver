// $Id: ioctl-getxflg.hc 685 2004-01-22 13:36:30Z ensc $    --*- c -*--

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

#include "vserver.h"
#include "ext2fs.h"

#include <sys/ioctl.h>

#define FIOC_GETXFLG    _IOR('x', 5, long)

static inline ALWAYSINLINE int
vc_X_get_xflg(int fd, long *flags)
{
  int		rc;
  *flags = 0;
  rc = ioctl(fd, FIOC_GETXFLG, flags);

  if (rc<-1) {
    errno = -rc;
    rc    = -1;
  }
  
  return rc;
}
