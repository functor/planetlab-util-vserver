// $Id: syscall_getvhiname-olduts.hc,v 1.1 2004/02/02 18:32:53 ensc Exp $    --*- c -*--

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
vc_get_vhi_name_olduts(xid_t xid, vc_uts_type type, char *val, size_t len)
{
  if (xid!=VC_SAMECTX) {
    errno = ESRCH;
    return -1;
  }
 
  switch (type) {
    case vcVHI_NODENAME		:  return gethostname  (val, len);
    case vcVHI_DOMAINNAME	:  return getdomainname(val, len);
    default			:
      errno = ENOENT;
      return -1;
  }
}
