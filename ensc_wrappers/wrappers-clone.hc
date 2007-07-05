// $Id: wrappers-clone.hc 814 2004-02-06 14:47:18Z ensc $    --*- c -*--

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

inline static WRAPPER_DECL pid_t
Eclone(int (*fn)(void *), void *child_stack, int flags, void *arg)
{
  pid_t		res;
#ifndef __dietlibc__
  res = clone(fn, child_stack, flags, arg);
#else
  res = clone((void*(*)(void*))(fn), child_stack, flags, arg);
#endif
  FatalErrnoError(res==-1, "clone()");
  return res;
}
