// $Id: syscall_setsched-v13obs.hc 2271 2006-01-22 18:18:28Z ensc $    --*- c -*--

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

#include "vserver.h"
#include <lib_internal/util.h>

#define VCGET(MASK,VAL)		((data->set_mask & (MASK)) ? (VAL) : SCHED_KEEP);

static inline ALWAYSINLINE int
vc_set_sched_v13obs(xid_t xid, struct vc_set_sched const *data)
{
#warning vc_set_sched_v13() uses an obsolete interface; remove it in the final version
  struct vcmd_set_sched_v2	k_data;

  
  k_data.cpu_mask    = 0;
  k_data.fill_rate   = VCGET(VC_VXSM_FILL_RATE,  data->fill_rate);
  k_data.interval    = VCGET(VC_VXSM_INTERVAL,   data->interval);
  k_data.tokens      = VCGET(VC_VXSM_TOKENS,     data->tokens);
  k_data.tokens_min  = VCGET(VC_VXSM_TOKENS_MIN, data->tokens_min);
  k_data.tokens_max  = VCGET(VC_VXSM_TOKENS_MAX, data->tokens_max);

  return vserver(VCMD_set_sched_v2, CTX_USER2KERNEL(xid), &k_data);
}
