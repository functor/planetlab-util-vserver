// $Id: errinfo.h,v 1.1 2004/07/02 23:34:52 ensc Exp $    --*- c -*--

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


#ifndef H_UTIL_VSERVER_LIB_INTERNAL_ERRINFO_H
#define H_UTIL_VSERVER_LIB_INTERNAL_ERRINFO_H

struct ErrorInformation
{
    char const *	app;	// the application-name
    char const *	pos;	// the detailed position of the error
    int			id;	// usually errno
};

void	ErrInfo_writeErrno(struct ErrorInformation const *);

#endif	//  H_UTIL_VSERVER_LIB_INTERNAL_ERRINFO_H
