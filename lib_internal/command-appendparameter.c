// $Id: command-appendparameter.c 1665 2004-08-19 14:09:34Z ensc $    --*- c -*--

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
#include "util.h"

void
Command_appendParameter(struct Command *cmd, char const *param)
{
  switch (cmd->params_style_) {
    case parNONE	:
      Vector_init(&cmd->params.v, 10);
      cmd->params_style_ = parVEC;
	/*@fallthrough@*/
    case parVEC		: {
      char const **p = Vector_pushback(&cmd->params.v);
      *p = param;
      break;
    }

    default		:
      WRITE_MSG(2, "internal error: conflicting functions Command_appendParameter() and Command_setParams() used together; aborting...\n");
      abort();
  }
}
