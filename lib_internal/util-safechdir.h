// $Id: util-safechdir.h,v 1.1 2004/02/18 04:42:38 ensc Exp $    --*- c -*--

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


#ifndef H_UTIL_VSERVER_LIB_INTERNAL_UTIL_SAFECHDIR_H
#define H_UTIL_VSERVER_LIB_INTERNAL_UTIL_SAFECHDIR_H

struct stat;
int	safeChdir(char const *, struct stat const *exp_stat) NONNULL((1,2));

#define EsafeChdir(PATH,EXP_STAT) \
  FatalErrnoError(safeChdir(PATH,EXP_STAT)==-1, "safeChdir()")

#endif	//  H_UTIL_VSERVER_LIB_INTERNAL_UTIL_SAFECHDIR_H
