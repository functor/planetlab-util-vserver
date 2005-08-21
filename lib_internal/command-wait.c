// $Id: command-wait.c,v 1.2 2004/07/02 23:44:33 ensc Exp $    --*- c -*--

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

#include "command.h"
#include <errno.h>

bool
Command_wait(struct Command *cmd, bool do_block)
{
  int		rc;
  
  if (cmd->rc!=-1) return true;

  switch (wait4(cmd->pid, &rc, (!do_block ? WNOHANG : 0), &cmd->rusage)) {
    case  0	:  break;
    case -1	:  cmd->err = errno; break;
    default	:  cmd->rc  = rc;    return true;
  }

  return false;
}
