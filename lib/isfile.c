// $Id: isfile.c 1654 2004-08-19 13:56:47Z ensc $    --*- c -*--

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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "internal.h"

#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>

bool
utilvserver_isFile(char const *path, bool follow_link)
{
  struct stat		st;
  if ( ( follow_link &&  stat(path, &st)==-1) ||
       (!follow_link && lstat(path, &st)==-1) ) return false;

  return S_ISREG(st.st_mode);
}
