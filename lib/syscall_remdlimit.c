// $Id: syscall_remdlimit.c,v 1.2 2005/03/02 01:27:54 ensc Exp $   --*- c++ -*--

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
#include "linuxvirtual.h"

#include "vserver-internal.h"

#ifdef VC_ENABLE_API_V13
#  include "syscall_remdlimit-v13.hc"
#endif

int
vc_rem_dlimit(char const *filename, xid_t xid, uint32_t flags)
{
  CALL_VC(CALL_VC_V13B(vc_rem_dlimit, filename, xid, flags));
}
