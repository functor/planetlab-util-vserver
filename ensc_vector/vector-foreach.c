// $Id: vector-foreach.c,v 1.1 2005/07/03 09:12:31 ensc Exp $    --*- c -*--

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

#include "vector.h"

void
Vector_foreach(struct Vector *vec, void (*func)(void *, void *), void *data)
{
  char *	ptr  = Vector_begin(vec);
  char * const	end  = Vector_end(vec);
  size_t const	step = vec->elem_size;

  for (; ptr<end; ptr += step)
    func(ptr, data);
}
