// $Id: syscall_enternamespace.c 2817 2008-10-31 04:45:26Z dhozac $    --*- c++ -*--

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

#define VC_MULTIVERSION_SYSCALL 1
#include "vserver-internal.h"

#ifdef VC_ENABLE_API_V13
#  include "syscall_enternamespace-v13.hc"
#endif

#ifdef VC_ENABLE_API_V21
#  include "syscall_enternamespace-v21.hc"
#endif

#ifdef VC_ENABLE_API_V23
#  include "syscall_enternamespace-v23.hc"
#endif

#if defined(VC_ENABLE_API_V13) || defined(VC_ENABLE_API_V21)
int
vc_enter_namespace(xid_t xid, uint_least64_t mask, uint32_t index)
{
  CALL_VC(CALL_VC_V23P  (vc_enter_namespace, xid, mask, index),
	  CALL_VC_SPACES(vc_enter_namespace, xid, mask, index),
	  CALL_VC_V13   (vc_enter_namespace, xid, mask, index));
}
#endif
