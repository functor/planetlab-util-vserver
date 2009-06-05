// $Id: syscall_adddlimit-v13.hc 1881 2005-03-02 01:29:44Z ensc $    --*- c -*--

// Copyright (C) 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
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
vc_add_dlimit_v13b(char const *filename, xid_t xid, uint32_t flags)
{
  struct vcmd_ctx_dlimit_base_v0	init = {
    .name   =  filename,
    .flags  =  flags
  };

  return vserver(VCMD_add_dlimit, CTX_USER2KERNEL(xid), &init);
}
