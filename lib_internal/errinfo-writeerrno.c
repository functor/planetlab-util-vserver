// $Id: errinfo-writeerrno.c 1616 2004-07-02 23:34:52Z ensc $    --*- c -*--

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

#include "errinfo.h"
#include "util.h"

void
ErrInfo_writeErrno(struct ErrorInformation const *info)
{
  if (info->app) {
    WRITE_STR(2, info->app);
    WRITE_MSG(2, ": ");
  }

  if (info->pos) {
    WRITE_STR(2, info->pos);
    if (info->id!=0) WRITE_MSG(2, ": ");
  }

  if (info->id!=0) {
    WRITE_STR(2, strerror(info->id));
  }

  WRITE_MSG(2, "\n");
}
