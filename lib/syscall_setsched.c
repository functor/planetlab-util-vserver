// $Id: syscall_setsched.c 2578 2007-08-08 20:05:26Z dhozac $    --*- c -*--

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

#define VC_MULTIVERSION_SYSCALL	1
#include "vserver-internal.h"

#ifdef VC_ENABLE_API_V13
#  include "syscall_setsched-v13.hc"
#endif

#ifdef VC_ENABLE_API_V13OBS
#  include "syscall_setsched-v13obs.hc"
#endif

#ifdef VC_ENABLE_API_V21
#  include "syscall_setsched-v21.hc"
#endif

#ifdef VC_ENABLE_API_V22
#  include "syscall_setsched-v22.hc"
#endif

int
vc_set_sched(xid_t xid, struct vc_set_sched const *data)
{
  CALL_VC(CALL_VC_V22   (vc_set_sched,xid,data),
	  CALL_VC_V21   (vc_set_sched,xid,data),
	  CALL_VC_V13B  (vc_set_sched,xid,data),
	  CALL_VC_V13OBS(vc_set_sched,xid,data));
}
