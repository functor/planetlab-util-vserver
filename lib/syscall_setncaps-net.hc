// $Id: syscall_setncaps-net.hc,v 1.1 2004/04/22 20:46:43 ensc Exp $    --*- c -*--

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
vc_set_ncaps_net(nid_t nid, struct vc_net_caps const *caps)
{
  struct vcmd_net_caps_v0	k_caps;

  k_caps.ncaps = caps->ncaps;
  k_caps.cmask = caps->cmask;
  
  return vserver(VCMD_set_ncaps, NID_USER2KERNEL(nid), &k_caps);
}