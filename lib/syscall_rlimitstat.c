// $Id: syscall_rlimitstat.c 2380 2006-11-15 20:14:00Z dhozac $    --*- c++ -*--

// Copyright (C) 2006 Daniel Hokka Zakrisson <daniel@hozac.com>
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
#include "vserver-internal.h"
#include "virtual.h"

#if defined(VC_ENABLE_API_V21)
#  include "syscall_rlimitstat-v21.hc"
#endif

#if defined(VC_ENABLE_API_V21)

int
vc_rlimit_stat(xid_t ctx, int resource, struct vc_rlimit_stat *stat)
{
  CALL_VC(CALL_VC_V21(vc_rlimit_stat, ctx, resource, stat));
}

#endif
