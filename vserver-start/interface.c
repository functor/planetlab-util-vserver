// $Id: interface.c,v 1.3 2004/10/19 21:11:10 ensc Exp $    --*- c -*--

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

#include "interface.h"

#include "vserver-start.h"
#include "configuration.h"
#include "undo.h"

#include <lib_internal/util.h>

static void
Iface_removeWrapper(void const *iface)
{
  (void)Iface_remove(iface);
}

void
activateInterfaces(InterfaceList const *interfaces)
{
  struct Interface const *	iface;

  for (iface=Vector_begin_const(interfaces);
       iface!=Vector_end_const(interfaces);
       ++iface) {
    if (!Iface_add(iface)) {
      WRITE_MSG(2, "Failed to add interface ");
      Iface_print(iface, 2);
      WRITE_MSG(2, "\n");
      
      exit(1);
    }
    Undo_addTask(Iface_removeWrapper, iface);
  }
}
