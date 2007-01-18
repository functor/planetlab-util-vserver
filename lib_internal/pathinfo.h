// $Id: pathinfo.h 1619 2004-07-02 23:44:53Z ensc $    --*- c -*--

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


#ifndef H_UTIL_VSERVER_LIB_INTERNAL_PATHINFO_H
#define H_UTIL_VSERVER_LIB_INTERNAL_PATHINFO_H

#include "string.h"

#define ENSC_PI_DECLARE(VAR,VAL)	PathInfo VAR={.d = VAL,.l = sizeof(VAL)-1}
#define ENSC_PI_APPSZ(P1,P2)		((P1).l + sizeof("/") + (P2).l)

typedef String		PathInfo;

void		PathInfo_append(PathInfo * restrict,
				PathInfo const * restrict,
				char *buf) NONNULL((1,2,3));

#endif	//  H_UTIL_VSERVER_LIB_INTERNAL_PATHINFO_H
