// $Id: string.h,v 1.3 2005/03/18 00:20:30 ensc Exp $    --*- c -*--

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


#ifndef H_UTIL_VSERVER_LIB_INTERNAL_STRING_H
#define H_UTIL_VSERVER_LIB_INTERNAL_STRING_H

#include <stdlib.h>

typedef struct
{
    char const *	d;
    size_t		l;
} String;

static void	String_init(String *);
static void	String_free(String *);

#define ENSC_STRING_FIXED(VAL) { .d = VAL, .l = (sizeof(VAL)-1) }
#define ENSC_STRING_ASSIGN_FIXED(DST,VAL) \
  { DST.d = (VAL); DST.l = (sizeof(VAL)-1); }

#include "string.hc"

#endif	//  H_UTIL_VSERVER_LIB_INTERNAL_STRING_H
