// $Id: ext2fs.h 720 2004-01-29 11:00:41Z ensc $    --*- c -*--

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


#ifndef H_UTIL_VSERVER_SRC_EXT2FS_H
#define H_UTIL_VSERVER_SRC_EXT2FS_H

#ifdef ENSC_HAVE_EXT2FS_EXT2_FS_H
#  include <ext2fs/ext2_fs.h>
#elif defined(ENSC_HAVE_LINUX_EXT2_FS_H)
#  include <linux/ext2_fs.h>
#else
#  error Do not know how to include <ext2_fs.h>
#endif

#endif	//  H_UTIL_VSERVER_SRC_EXT2FS_H
