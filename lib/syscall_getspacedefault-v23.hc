// $Id: syscall_getspacedefault-v23.hc 2817 2008-10-31 04:45:26Z dhozac $    --*- c -*--

// Copyright (C) 2008 Daniel Hokka Zakrisson
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

static inline ALWAYSINLINE uint_least64_t
vc_get_space_default_v23(int UNUSED tmp)
{
  struct vcmd_space_mask_v1 data = { .mask = 0 };
  int ret = vserver(VCMD_get_space_default, 0, &data);
  if (ret)
    return ret;
  return data.mask & ~(CLONE_NEWNS|CLONE_FS);
}
