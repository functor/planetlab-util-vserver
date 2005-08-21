// $Id: compat-c99.h,v 1.1 2003/12/26 00:49:32 uid68581 Exp $    --*- c -*--

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


#ifndef H_UTIL_VSERVER_COMPAT_C99_H
#define H_UTIL_VSERVER_COMPAT_C99_H

#if defined(__GNUC__) && __GNUC__ < 3 || (__GNUC__==3 && __GNUC_MINOR__<3)
#  warning Enabling hacks to make it compilable with non-C99 compilers
#  define BS { do {} while (0)
#  define BE } do {} while (0)
#else
#  define BS do {} while (0)
#  define BE do {} while (0)
#endif

#endif	//  H_UTIL_VSERVER_COMPAT_C99_H
