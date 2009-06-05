// $Id: syscall_ctxcreate.c 2777 2008-08-27 10:38:07Z dhozac $    --*- c -*--

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
#  include "syscall_ctxcreate-v13.hc"
#endif

#if defined(VC_ENABLE_API_V21)
#  include "syscall_ctxcreate-v21.hc"
#endif

#if defined(VC_ENABLE_API_V13) || defined(VC_ENABLE_API_V21)
xid_t
vc_ctx_create(xid_t xid, struct vc_ctx_flags *flags)
{
  CALL_VC(CALL_VC_V21 (vc_ctx_create, xid, flags),
	  CALL_VC_V13A(vc_ctx_create, xid, flags));
}
#endif
