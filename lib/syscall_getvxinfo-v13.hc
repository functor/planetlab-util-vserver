// $Id: syscall_getvxinfo-v13.hc 970 2004-02-20 17:04:12Z ensc $    --*- c -*--

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

static inline ALWAYSINLINE int
vc_get_vx_info_v13(xid_t xid, struct vc_vx_info *info)
{
  if (xid==0 || xid==1) {
    info->xid     = xid;
    info->initpid = -1;

    return 0;
  }
  else {
    struct vcmd_vx_info_v0	res;
    int				rc;

    rc = vserver(VCMD_vx_info, CTX_USER2KERNEL(xid), &res);
    if (rc==-1) return -1;

    info->xid     = CTX_KERNEL2USER(res.xid);
    info->initpid = res.initpid;

    return rc;
  }
}
