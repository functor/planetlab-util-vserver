// $Id: syscall_setnamespace-v13.hc 2817 2008-10-31 04:45:26Z dhozac $    --*- c -*--

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

static inline ALWAYSINLINE int
vc_set_namespace_v13(xid_t xid, uint_least64_t mask, uint32_t index)
{
  if ((mask & (CLONE_NEWNS|CLONE_FS)) == 0)
    return 0;
  if (index != 0) {
    errno = EINVAL;
    return -1;
  }
  return vserver(VCMD_set_space_v0, CTX_USER2KERNEL(xid), 0);
}
