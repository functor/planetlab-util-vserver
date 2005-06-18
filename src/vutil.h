// $Id: vutil.h,v 1.1 2003/09/29 22:01:57 ensc Exp $

// Copyright (C) 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
// based on vutil.h by Jacques Gelinas
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#pragma interface
#ifndef VUTIL_H
#define VUTIL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string>
#include <set>
#include <algorithm>
#include <iostream>
#include <list>

using namespace std;

extern int debug;
extern bool testmode;

// Patch to help compile this utility on unpatched kernel source
#ifndef EXT2_IMMUTABLE_FL
#define EXT2_IMMUTABLE_FL 0x00000010
#endif

#ifndef EXT2_IUNLINK_FL
/* Set both bits for backward compatibility */
#define EXT2_IUNLINK_FL 0x08008000
#endif

#ifndef EXT2_BARRIER_FL
#define EXT2_BARRIER_FL 0x04000000
#endif


FILE *vutil_execdistcmd (const char *, const string &, const char *);
extern const char K_DUMPFILES[];
extern const char K_UNIFILES[];
extern const char K_PKGVERSION[];

class PACKAGE{
public:
	string name;
	string version;	// version + release
	PACKAGE(string &_name, string &_version)
		: name (_name), version(_version)
	{
	}
	PACKAGE(const char *_name, const char *_version)
		: name (_name), version(_version)
	{
	}
	PACKAGE(const string &line)
	{
		*this = line;
	}
	PACKAGE & operator = (const string &_line)
	{
		string line (_line);
		string::iterator pos = find (line.begin(),line.end(),'=');
		if (pos != line.end()){
			name = string(line.begin(),pos);
			version = string(pos + 1,line.end());
		}
		return *this;
	}
	PACKAGE (const PACKAGE &pkg)
	{
		name = pkg.name;
		version = pkg.version;
	}
	bool operator == (const PACKAGE &v) const
	{
		return name == v.name && version == v.version;
	}
	bool operator < (const PACKAGE &v) const
	{
		bool ret = false;
		if (name < v.name){
			ret = true;
		}else if (name == v.name && version < v.version){
			ret = true;
		}
		return ret;
	}
	// Load the file member of the package, but exclude configuration file
	void loadfiles(const string &ref, set<string> &files)
	{
		if (debug > 2) cout << "Loading files for package " << name << endl;
		string namever = name + '-' + version;
		FILE *fin = vutil_execdistcmd (K_UNIFILES,ref,namever.c_str());
		if (fin != NULL){
			char tmp[1000];
			while (fgets(tmp,sizeof(tmp)-1,fin)!=NULL){
				int last = strlen(tmp)-1;
				if (last >= 0 && tmp[last] == '\n') tmp[last] = '\0';
				files.insert (tmp);
			}
		}
		if (debug > 2) cout << "Done\n";
	}
	#if 0
	bool locate(const string &path)
	{
		return find (files.begin(),files.end(),path) != files.end();
	}
	#endif
};

// Check if two package have the same name (but potentially different version)
class same_name{
	const PACKAGE &pkg;
public:
	same_name(const PACKAGE &_pkg) : pkg(_pkg) {}
	bool operator()(const PACKAGE &p)
	{
		return pkg.name == p.name;
	}
};


#include "vutil.p"

#endif

