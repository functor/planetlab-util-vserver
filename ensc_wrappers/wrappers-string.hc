// $Id: wrappers-string.hc,v 1.1 2005/02/25 23:31:19 ensc Exp $    --*- c -*--

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

#ifndef H_ENSC_IN_WRAPPERS_H
#  error wrappers_handler.hc can not be used in this way
#endif

/*@unused@*/
inline static WRAPPER_DECL char *
Estrdup(char const *s)
{
  register char               *res = strdup(s);
  FatalErrnoError(res==0, "strdup()");
  return res;
}