// $Id: syscall_rlimitstat-v21.hc 2380 2006-11-15 20:14:00Z dhozac $    --*- c++ -*--

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
vc_rlimit_stat_v21(xid_t ctx, int resource, struct vc_rlimit_stat *stat)
{
  struct vcmd_rlimit_stat_v0	param = { .id = resource };
  int ret;

  ret = vserver(VCMD_rlimit_stat, CTX_USER2KERNEL(ctx), &param);
  if (ret)
    return ret;

  stat->hits	= param.hits;
  stat->value	= param.value;
  stat->minimum	= param.minimum;
  stat->maximum	= param.maximum;

  return 0;
}
