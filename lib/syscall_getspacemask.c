// $Id: syscall_getspacemask.c 2818 2008-10-31 15:40:01Z dhozac $    --*- c -*--

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

#include "vserver.h"
#include "virtual.h"

#if defined(VC_ENABLE_API_V21) && defined(VC_ENABLE_API_V23)
#  define VC_MULTIVERSION_SYSCALL 1
#endif
#include "vserver-internal.h"

#if defined(VC_ENABLE_API_V21)
#  include "syscall_getspacemask-v21.hc"
#endif

#if defined(VC_ENABLE_API_V23)
#  include "syscall_getspacemask-v23.hc"
#endif

#if defined(VC_ENABLE_API_V21) || defined(VC_ENABLE_API_V23)
uint_least64_t
vc_get_space_mask()
{
  CALL_VC(CALL_VC_V23P  (vc_get_space_mask, 0),
	  CALL_VC_SPACES(vc_get_space_mask, 0));
}
#endif
