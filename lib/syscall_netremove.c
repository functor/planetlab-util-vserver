// $Id: syscall_netremove.c 2578 2007-08-08 20:05:26Z dhozac $    --*- c -*--

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

#include <string.h>

#include "vserver.h"
#include "virtual.h"

#if defined(VC_ENABLE_API_NET) && defined(VC_ENABLE_API_NETV2)
#  define VC_MULTIVERSION_SYSCALL 1
#endif
#include "vserver-internal.h"

#if defined(VC_ENABLE_API_NET)
#  include "syscall_netremove-net.hc"
#endif

#if defined(VC_ENABLE_API_NETV2)
#  include "syscall_netremove-netv2.hc"
#endif

#if defined(VC_ENABLE_API_NET) || defined(VC_ENABLE_API_NETV2)
int
vc_net_remove(nid_t nid, struct vc_net_addr const *info)
{
  if (info==0) {
    errno = EFAULT;
    return -1;
  }
  
  CALL_VC(CALL_VC_NETV2(vc_net_remove, nid, info),
	  CALL_VC_NET  (vc_net_remove, nid, info));
}
#endif
