// $Id: stack-start.h,v 1.2 2004/02/20 17:02:20 ensc Exp $    --*- c++ -*--

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


#ifndef H_UTIL_VSERVER_SRC_STACK_START_H
#define H_UTIL_VSERVER_SRC_STACK_START_H

#ifdef HAVE_GROWING_STACK
#  define STACK_START(PTR)		(PTR)
#else
#  define STACK_START(PTR)		((PTR)+sizeof(PTR)/sizeof(PTR[0])-1)
#endif

#endif	//  H_UTIL_VSERVER_SRC_STACK_START_H
