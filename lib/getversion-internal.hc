// $Id: getversion-internal.hc,v 1.1.2.3 2003/12/26 00:16:48 uid68581 Exp $    --*- c++ -*--

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


#ifndef H_UTIL_VSERVER_LIB_GETVERSION_INTERNAL_H
#define H_UTIL_VSERVER_LIB_GETVERSION_INTERNAL_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "compat.h"

#include "vserver-internal.h"
#include "linuxvirtual.h"

static inline ALWAYSINLINE int
vc_get_version_internal(int cat)
{
  return vserver(VC_CMD(VERSION, 0, 0), cat, 0);
}

#endif	//  H_UTIL_VSERVER_LIB_GETVERSION_INTERNAL_H
