// $Id: util-isnumberunsigned.c 2255 2006-01-22 11:23:47Z ensc $    --*- c -*--

// Copyright (C) 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
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

#include "util.h"
#include "util-isnumber.hc"
#include <limits.h>

static inline bool
checkConstraints(unsigned long val, unsigned int fac)
{
  if ((long)(val)   <    0) return false;
  if (ULONG_MAX/fac <= val) return false;
  return true;
}

ENSC_DECL_UTIL_ISNUMBER(isNumberUnsigned, unsigned long, strtoul)
