// $Id: command.c,v 1.2 2004/08/19 14:10:06 ensc Exp $    --*- c -*--

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

#include <lib_internal/util.h>
#include <lib_internal/command.h>

int wrapper_exit_code = 255;

int
main(int argc, char *argv[])
{
  struct Command		cmd;
  ssize_t			i;
  
  if (argc<3) {
    WRITE_MSG(2, "Not enough parameters\n");
    return EXIT_FAILURE;
  }

  Command_init(&cmd);
  for (i=2; i<argc; ++i)
    Command_appendParameter(&cmd, argv[i]);

  if (Command_exec(&cmd, argv[1][0]!=0)) {
    Command_wait(&cmd, true);
  }

  Command_free(&cmd);
}
