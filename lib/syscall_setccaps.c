// $Id: syscall_setccaps.c 2367 2006-11-03 19:59:15Z dhozac $    --*- c -*--

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
#include "virtual.h"

#if defined(VC_ENABLE_API_V13) && defined(VC_ENABLE_API_V21)
#  define VC_MULTIVERSION_SYSCALL 1
#endif
#include "vserver-internal.h"

#if defined(VC_ENABLE_API_V13)
#  include "syscall_setccaps-v13.hc"
#endif

#if defined(VC_ENABLE_API_V21)
#  include "syscall_setccaps-v21.hc"
#endif

#if defined(VC_ENABLE_API_V13) || defined(VC_ENABLE_API_V21)
int
vc_set_ccaps(xid_t xid, struct vc_ctx_caps const *caps)
{
  if (caps==0) {
    errno = EFAULT;
    return -1;
  }
  
  CALL_VC(CALL_VC_V21(vc_set_ccaps, xid, caps),
	  CALL_VC_V13A(vc_set_ccaps, xid, caps));
}
#endif
