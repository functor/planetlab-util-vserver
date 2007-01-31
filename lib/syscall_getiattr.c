// $Id: syscall_getiattr.c,v 1.3 2004/03/05 04:40:59 ensc Exp $    --*- c++ -*--

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
#include "linuxvirtual.h"

#if defined(VC_ENABLE_API_FSCOMPAT) && defined(VC_ENABLE_API_V13)
#  define VC_MULTIVERSION_SYSCALL	1
#endif
#include "vserver-internal.h"

#ifdef VC_ENABLE_API_V13
#  include "syscall_getiattr-v13.hc"
#endif

#ifdef VC_ENABLE_API_FSCOMPAT
#  include "syscall_getiattr-fscompat.hc"
#endif

int
vc_get_iattr(char const *filename, xid_t *xid,  uint32_t *flags, uint32_t *mask)
{
  if ( (mask==0) ||
       ((*mask&VC_IATTR_XID)  && xid==0) ||
       ((*mask&~VC_IATTR_XID) && flags==0) ) {
    errno = EFAULT;
    return -1;
  }
  if ( flags ) *flags &= ~*mask;

  CALL_VC(CALL_VC_V13     (vc_get_iattr, filename, xid, flags, mask),
	  CALL_VC_FSCOMPAT(vc_get_iattr, filename, xid, flags, mask));
}
