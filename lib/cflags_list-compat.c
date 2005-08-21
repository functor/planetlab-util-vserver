// $Id: cflags_list-compat.c,v 1.1 2004/04/14 23:19:51 ensc Exp $    --*- c -*--

// Copyright (C) 2004 Enrico Scholz <>
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

#include "vserver.h"
#include <string.h>

uint_least32_t
vc_list2cflag_compat(char const *str, size_t len,
		    struct vc_err_listparser *err)
{
  uint32_t		res = 0;

  if (len==0) len = strlen(str);
  
  for (;len>0;) {
    char const		*ptr = strchr(str, ',');
    size_t		cnt  = ptr ? (size_t)(ptr-str) : len;
    unsigned int	tmp;

    if (cnt>=len) { cnt=len; len=0; }
    else len-=(cnt+1);
    
    tmp = vc_text2cflag_compat(str,cnt);

    if (tmp!=0) res |= tmp;
    else {
      if (err) {
	err->ptr = str;
	err->len = cnt;
      }
      return res;
    }

    if (ptr==0) break;
    str = ptr+1;
  }

  if (err) {
    err->ptr = 0;
    err->len = 0;
  }
  return res;
}
