// $Id: capability-compat.h 2788 2008-09-28 11:24:07Z dhozac $    --*- c -*--

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

#ifdef HAVE_SYS_CAPABILITY_H
#  include <sys/capability.h>
#else
#  include <linux/capability.h>

extern int capget (struct __user_cap_header_struct *, struct __user_cap_data_struct *);
extern int capset (struct __user_cap_header_struct *, struct __user_cap_data_struct *);

#endif

#ifndef _LINUX_CAPABILITY_VERSION_1
#  define _LINUX_CAPABILITY_VERSION_1	_LINUX_CAPABILITY_VERSION
#endif

#ifndef _LINUX_CAPABILITY_VERSION_3
#  define _LINUX_CAPABILITY_VERSION_3	0x20080522
#endif
