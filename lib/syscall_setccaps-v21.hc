// $Id: syscall_setccaps-v21.hc 2372 2006-11-05 17:48:24Z dhozac $    --*- c -*--

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

static inline ALWAYSINLINE int
vc_set_ccaps_v21(xid_t xid, struct vc_ctx_caps const *caps)
{
  struct vcmd_ctx_caps_v1	k_ccaps;
  struct vcmd_bcaps		k_bcaps;
  int ret;

  k_bcaps.bcaps = caps->bcaps;
  k_bcaps.bmask = caps->bmask;
  k_ccaps.ccaps = caps->ccaps;
  k_ccaps.cmask = caps->cmask;
  
  ret = vserver(VCMD_set_ccaps, CTX_USER2KERNEL(xid), &k_ccaps);
  if (ret)
    return ret;
  return vserver(VCMD_set_bcaps, CTX_USER2KERNEL(xid), &k_bcaps);
}
