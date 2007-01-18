// $Id: syscall-syscall.c 1655 2004-08-19 13:57:53Z ensc $    --*- c -*--

// Copyright (C) 2004 Enrico Scholz Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
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

#include "vserver-internal.h"

#if 0
int
vc_syscall(uint32_t cmd, xid_t xid, void *data) __attribute__((__alias__("vserver")));
#else
int
vc_syscall(uint32_t cmd, xid_t xid, void *data)
{
  return vserver(cmd, xid, data);
}
#endif
