// $Id: interface-free.hc,v 1.1 2004/07/03 00:07:42 ensc Exp $    --*- c -*--

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
Iface_free(struct Interface *iface)
{
  free(const_cast(char *)(iface->name));
  free(const_cast(char *)(iface->scope));
  free(const_cast(char *)(iface->dev));
  free(const_cast(char *)(iface->mac));
}
