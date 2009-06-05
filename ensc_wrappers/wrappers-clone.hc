// $Id: wrappers-clone.hc 2747 2008-07-15 22:14:13Z dhozac $    --*- c -*--

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


#ifndef H_ENSC_IN_WRAPPERS_H
#  error wrappers_handler.hc can not be used in this way
#endif

#include <lib_internal/sys_clone.h>

inline static WRAPPER_DECL pid_t
Eclone(uint64_t flags, void *child_stack)
{
  pid_t		res;
  res = sys_clone(flags, child_stack);
  FatalErrnoError(res==-1, "clone()");
  return res;
}
