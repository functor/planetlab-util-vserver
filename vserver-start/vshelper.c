// $Id: vshelper.c,v 1.2 2004/08/19 16:06:37 ensc Exp $    --*- c -*--

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

#include "vserver-start.h"
#include "pathconfig.h"

#include <lib_internal/util.h>
#include <lib_internal/command.h>

void
Vshelper_doSanityCheck()
{
  struct Command	cmd;
  char const *		argv[] = {
    "/bin/bash", "-c",
    ". " PATH_UTILVSERVER_VARS ";. " PATH_FUNCTIONS "; vshelper.doSanityCheck",
    0
  };

  Command_init(&cmd);
  Command_setParams(&cmd, argv);
  if (!Command_exec(&cmd, true) ||
      !Command_wait(&cmd, true))
    WRITE_MSG(2, "vserver-start: failed to do the vshelper-sanitycheck\n");

  if (cmd.rc!=0)
    exit(0);
}