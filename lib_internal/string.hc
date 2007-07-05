// $Id: string.hc 1915 2005-03-18 00:20:42Z ensc $    --*- c -*--

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

static inline UNUSED void
String_init(String *str)
{
  str->d = 0;
  str->l = 0;
}

static inline UNUSED char const *
String_c_str(String const *str, char *buf)
{
  if (str->l==0) return "";

  if (buf!=str->d)
    abort();	// TODO: copy content

  buf[str->l] = '\0';
  return buf;
}


static inline UNUSED void
String_free(String *str)
{
  free((char *)(str->d));
}
