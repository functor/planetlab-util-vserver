// $Id: util-declarecmd.h 1009 2004-02-26 13:07:15Z ensc $    --*- c -*--

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


#ifndef H_UTILVSERVER_LIB_INTERNAL_UTIL_DECLARECMD_H
#define H_UTILVSERVER_LIB_INTERNAL_UTIL_DECLARECMD_H

#define VSERVER_DECLARE_CMD(CMD)     \
  char		buf[strlen(CMD)+1];  \
  memcpy(buf, (CMD), strlen(CMD)+1); \
  CMD = basename(buf);

#endif	//  H_UTILVSERVER_LIB_INTERNAL_UTIL_DECLARECMD_H
