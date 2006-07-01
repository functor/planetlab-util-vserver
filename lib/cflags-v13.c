// $Id: cflags-v13.c,v 1.3 2004/10/21 19:00:20 ensc Exp $    --*- c -*--

// Copyright (C) 2004 Enrico Scholz <ensc@delenn.intern.sigma-chemnitz.de>
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
#include "internal.h"
#include <lib_internal/util-dimof.h>

#include <string.h>
#include <assert.h>

#define DECL(STR, VAL) { STR, sizeof(STR)-1, VAL }

static struct Mapping_uint64 const VALUES[] = {
  DECL("lock",		VC_VXF_INFO_LOCK),
  DECL("nproc",		VC_VXF_INFO_NPROC),
  DECL("private",	VC_VXF_INFO_PRIVATE),
  DECL("fakeinit",	VC_VXF_INFO_INIT),

  DECL("hideinfo",	VC_VXF_INFO_HIDEINFO),
  DECL("ulimit",	VC_VXF_INFO_ULIMIT),
  DECL("namespace",	VC_VXF_INFO_NAMESPACE),

  DECL("sched_hard",    VC_VXF_SCHED_HARD),
  DECL("sched_prio",    VC_VXF_SCHED_PRIO),
  DECL("sched_pause",   VC_VXF_SCHED_PAUSE),
  DECL("sched_share",   VC_VXF_SCHED_SHARE),

  DECL("virt_mem",      VC_VXF_VIRT_MEM),
  DECL("virt_uptime",   VC_VXF_VIRT_UPTIME),
  DECL("virt_cpu",      VC_VXF_VIRT_CPU),
  DECL("virt_load",     VC_VXF_VIRT_LOAD),

  DECL("hide_mount",	VC_VXF_HIDE_MOUNT),
  DECL("hide_netif",	VC_VXF_HIDE_NETIF),

  DECL("state_setup",   VC_VXF_STATE_SETUP),
  DECL("state_init",    VC_VXF_STATE_INIT),

  DECL("fork_rss",	VC_VXF_FORK_RSS),
  DECL("prolific",	VC_VXF_PROLIFIC),
  DECL("igneg_nice",    VC_VXF_IGNEG_NICE),
  
};

uint_least64_t
vc_text2cflag(char const *str, size_t len)
{
  ssize_t	idx = utilvserver_value2text_uint64(str, len,
						    VALUES, DIM_OF(VALUES));
  if (idx==-1) return 0;
  else         return VALUES[idx].val;
}

char const *
vc_locflag2text(uint_least64_t *val)
{
  ssize_t	idx = utilvserver_text2value_uint64(val,
						    VALUES, DIM_OF(VALUES));

  if (idx==-1) return 0;
  else         return VALUES[idx].id;
}
