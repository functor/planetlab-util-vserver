// $Id: syscall_kill-v11.hc,v 1.1.2.5 2004/01/26 18:20:18 ensc Exp $    --*- c++ -*--

// Copyright (C) 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
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
vc_ctx_kill_v11(xid_t ctx, pid_t pid, int sig)
{
  struct vcmd_ctx_kill_v0	param = { 0,0 };
  param.pid = pid;
  param.sig = sig;

  return vserver(VC_CMD(PROCTRL, 1, 0), ctx, &param);
}
