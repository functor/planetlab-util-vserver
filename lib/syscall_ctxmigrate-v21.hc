// $Id: syscall_ctxmigrate-v21.hc 2814 2008-10-31 04:05:53Z dhozac $    --*- c -*--

// Copyright (C) 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
// Copyright (C) 2006 Daniel Hokka Zakrisson
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

static inline ALWAYSINLINE int
vc_ctx_migrate_spaces(xid_t xid, uint_least64_t flags)
{
  struct vcmd_ctx_migrate data = { .flagword = flags };

  return vserver(VCMD_ctx_migrate_v1, CTX_USER2KERNEL(xid), &data);
}
