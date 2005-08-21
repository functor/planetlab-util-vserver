// $Id: createskeleton-short.hc,v 1.1 2004/02/18 04:42:38 ensc Exp $    --*- c -*--

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

#include <stdlib.h>
#include <string.h>

static inline int
vc_createSkeleton_short(char const *id, int flags)
{
  size_t	l = strlen(id);
  char		buf[sizeof(CONFDIR "/") + l];

  memcpy(buf, CONFDIR "/", sizeof(CONFDIR "/")-1);
  memcpy(buf+sizeof(CONFDIR "/")-1, id, l+1);
  
  return vc_createSkeleton_full(buf, id, flags);
}