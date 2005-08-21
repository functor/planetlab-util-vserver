// $Id: ioctl-getext2flags.hc,v 1.4 2004/01/23 20:47:32 ensc Exp $    --*- c -*--

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

#ifndef EXT2_IMMUTABLE_FILE_FL
#  define EXT2_IMMUTABLE_FILE_FL	0x00000010
#endif

#ifndef EXT2_IMMUTABLE_LINK_FL
#  define EXT2_IMMUTABLE_LINK_FL	0x00008000
#endif

static inline ALWAYSINLINE int
vc_X_get_ext2flags(int fd, long *flags)
{
  int		rc;

  *flags = 0;
  rc     = ioctl(fd, EXT2_IOC_GETFLAGS, flags);
  *flags = EXT2FLAGS_KERNEL2USER(*flags);

  ENSC_FIX_IOCTL(rc);

  return rc;
}
