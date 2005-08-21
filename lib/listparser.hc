// $Id: listparser.hc,v 1.5 2005/04/24 20:23:11 ensc Exp $    --*- c -*--

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

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define TONUMBER_uint64(S,E,B)		strtoll(S,E,B)
#define TONUMBER_uint32(S,E,B)		strtol (S,E,B)

#define ISNUMBER(TYPE,SHORT)						\
  static inline ALWAYSINLINE bool					\
  isNumber_##SHORT(char const **str,size_t *len,TYPE *res,char end_chr) \
  {									\
    char	*err_ptr;						\
    if (**str=='^') {							\
      *res = ((TYPE)(1)) << TONUMBER_##SHORT(++*str, &err_ptr, 0);	\
      if (len) --*len;							\
    }									\
    else								\
      *res = TONUMBER_##SHORT(*str, &err_ptr, 0);			\
    return err_ptr>*str && *err_ptr==end_chr;				\
  }


#define LISTPARSER(TYPE,SHORT)						\
  ISNUMBER(TYPE,SHORT)							\
  int									\
  utilvserver_listparser_ ## SHORT(char const *str, size_t len,		\
				   char const **err_ptr,		\
				   size_t *err_len,			\
				   TYPE * const flag,			\
				   TYPE * const mask,			\
				   TYPE (*func)(char const *,		\
						size_t, bool *))	\
  {									\
    if (len==0) len = strlen(str);					\
    for (;len>0;) {							\
      char const	*ptr = strchr(str, ',');			\
      size_t		cnt;						\
      TYPE		tmp = 0;					\
      bool		is_neg     = false;				\
      bool		failed     = false;				\
      									\
      while (mask!=0 && len>0 && (*str=='!' || *str=='~')) {		\
	is_neg = !is_neg;						\
	++str;								\
	--len;								\
      }									\
      									\
      cnt = ptr ? (size_t)(ptr-str) : len;				\
      if (cnt>=len) { cnt=len; len=0; }					\
      else len-=(cnt+1);						\
									\
      if (cnt==0) 							\
	failed = true;							\
      else if (mask!=0 &&						\
	       (strncasecmp(str,"all",cnt)==0 ||			\
		strncasecmp(str,"any",cnt)==0))				\
	tmp = ~(TYPE)(0);						\
      else if (mask!=0 && strncasecmp(str,"none",cnt)==0) {}		\
      else if (!isNumber_##SHORT(&str, &cnt, &tmp, str[cnt]))		\
	tmp = (*func)(str,cnt, &failed);				\
									\
      if (!failed) {							\
	if (!is_neg) *flag |=  tmp;					\
	else         *flag &= ~tmp;					\
	if (mask!=0) *mask |=  tmp;					\
      }									\
      else {								\
	if (err_ptr) *err_ptr = str;					\
	if (err_len) *err_len = cnt;					\
	return -1;							\
      }									\
									\
      if (ptr==0) break;						\
      str = ptr+1;							\
    }									\
									\
    if (err_ptr) *err_ptr = 0;						\
    if (err_len) *err_len = 0;						\
    return 0;								\
  }
