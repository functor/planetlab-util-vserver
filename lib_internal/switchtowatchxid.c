// $Id: switchtowatchxid.c,v 1.4 2005/03/25 02:37:41 ensc Exp $    --*- c -*--

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

#include <vserver.h>
#include <errno.h>
#include <unistd.h>

  // try to switch into context 1
bool
switchToWatchXid(char const **errptr)
{
#if 0
#  warning Compiling in debug-code
  return;
#endif
  if (vc_get_task_xid(0)==1) return true;

  if (vc_isSupported(vcFEATURE_MIGRATE)) {
    if (vc_ctx_migrate(1)==-1) {
      if (errptr) *errptr = "vc_migrate_context()";
      return false;
    }
  }
  else {
#if VC_ENABLE_API_COMPAT
    if (vc_new_s_context(1,0,0)==VC_NOCTX) {
      if (errptr) *errptr = "vc_new_s_context()";
      return false;
    }
#else
    if (errptr) *errptr = "can not change context: migrate kernel feature missing and 'compat' API disabled";
    errno = ENOSYS;
    return false;
#endif
  }

  return true;
}

