// $Id: unify-settime.c,v 1.2 2004/06/27 13:03:58 ensc Exp $    --*- c -*--

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

#include "unify.h"
#include <utime.h>
#include <sys/stat.h>

bool
Unify_setTime(char const *dst, struct stat const *st)
{
  struct utimbuf	utm;

    // skip symlinks
  if (S_ISLNK(st->st_mode)) return true;
  
  utm.actime  = st->st_atime;
  utm.modtime = st->st_mtime;
  return utime(dst, &utm)!=-1;
}

