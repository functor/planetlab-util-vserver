# $Id: util-vserver.spec.in 2482 2007-01-29 23:37:07Z dhozac $

## This package understands the following switches:
## --without dietlibc        ...   disable usage of dietlibc
## --with xalan              ...   require/use the xalan xslt processor
## --without doc             ...   disable doc generation
## --with legacy             ...   enable the legacy APIs

%global confdir		%_sysconfdir/vservers
%global confdefaultdir	%confdir/.defaults
%global pkglibdir	%_libdir/%name
%global chkconfig	/sbin/chkconfig

%global _localstatedir	%_var

%global ver		%( echo 0.30.213 | sed 's/-.*//' )
%global subver		%( s=`echo 0.30.213 | grep -- - | sed 's/.*-/./'`; echo ${s:-.1} )
%global fullver		0.30.213


%define name util-vserver
%define version 0.30.213
%{!?release_func:%global release_func() 1%{?pldistro:.%{pldistro}}%{?date:.%{date}}}

%global python_sitearch	%( python -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)" )

%define _without_dietlibc 1
%define _without_xalan 1

# don't build debuginfo RPM
%define debug_package %{nil}

Vendor: PlanetLab
Packager: PlanetLab Central <support@planet-lab.org>
Distribution: PlanetLab 3.0
URL: http://cvs.planet-lab.org/cvs/util-vserver

Summary:	Linux virtual server utilities
Name:		util-vserver
Version:	%ver
Release:	%release_func 0%subver
License:	GPL
Group:		System Environment/Base
#URL:		http://savannah.nongnu.org/projects/util-vserver/
Source0:	http://ftp.linux-vserver.org/pub/utils/util-vserver/%name-%version.tar.bz2
#Source1:	http://ftp.linux-vserver.org/pub/utils/util-vserver/%name-%version.tar.bz2.asc
BuildRoot:	%_tmppath/%name-%version-%release-root
Requires:	init(%name)
Requires:	%name-core = %version-%release
Requires:	%name-lib  = %version-%release
Requires:	diffutils mktemp sed
Provides:	vserver = %version-%release
Obsoletes:	vserver < %version
BuildRequires:	mount vconfig gawk iproute iptables
BuildRequires:	gcc-c++ wget which diffutils
BuildRequires:	e2fsprogs-devel beecrypt-devel
%{!?_without_doc:BuildRequires:	doxygen tetex-latex}
Requires(post):		%name-core
Requires(pre):		%pkglibdir
Requires(postun):	%pkglibdir
%{!?_without_dietlibc:BuildRequires:	dietlibc >= 0:0.25}
%{?_with_xalan:BuildRequires:	xalan-j}

%package lib
Summary:		Dynamic libraries for util-vserver
Group:			System Environment/Libraries

%package core
Summary:		The core-utilities for util-vserver
Group:			Applications/System
Requires:		util-linux

%package build
Summary:		Tools which can be used to build vservers
Group:			Applications/System
Requires:		rpm wget tar
Requires:		%name = %version-%release
Requires(pre):		%confdir
Requires(postun):	%confdir
Requires(post):		%name-core

%package sysv
Summary:		SysV-initscripts for vserver
Group:			System Environment/Base
Provides:		init(%name) = sysv
Requires:		make diffutils
Requires:		initscripts
Requires:		%name = %version-%release
Requires(post):		%chkconfig
Requires(preun):	%chkconfig
Requires(pre):		%_initrddir %pkglibdir
requires(postun):	%_initrddir %pkglibdir

%package legacy
Summary:		Legacy utilities for util-vserver
Group:			Applications/System
Requires:		%name = %version-%release
Requires(post):		%chkconfig
Requires(preun):	%chkconfig
Requires(pre):		%_initrddir %pkglibdir
requires(postun):	%_initrddir %pkglibdir

%package devel
Summary:		Header-files and libraries needed to develop vserver based applications
Group:			Development/Libraries
Requires:		pkgconfig
Requires:		%name-lib = %version-%release

%package python
Summary:		Python modules for manipulating vservers
Group:			Applications/System
Requires:		python
Obsoletes:		util-vserver-py23 resman


%description
util-vserver provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This requires a special kernel supporting the new new_s_context and
set_ipv4root system call.

%description lib
util-vserver provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This package contains the shared libraries needed by all other
'util-vserver' subpackages.

%description core
util-vserver provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This package contains utilities which are required to communicate with
the Linux-Vserver enabled kernel.


%description build
util-vserver provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This package contains utilities which assist in building Vservers.

%description sysv
util-vserver provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This package contains the SysV initscripts which start and stop
VServers and related tools.


%description legacy
util-vserver provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This package contains the tools which are needed to work with VServers
having an old-style configuration.


%description devel
util-vserver provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This package contains header files and libraries which are needed to
develop VServer related applications.


%description python
Python modules for manipulating vservers.  Provides a superset of the
functionality of the vserver script (at least will do in the future),
but more readily accessible from Python code.


%prep
%setup -q -n %name-%fullver

aclocal -I m4
autoconf
automake --add-missing

%build
%configure --with-initrddir=%_initrddir --enable-release \
           %{?_without_dietlibc:--disable-dietlibc} \
           %{!?_without_legacy:--enable-apis=NOLEGACY} \
           --with-initscripts=sysv

%__make %{?_smp_mflags} all
%{!?_without_doc:%__make %{?_smp_mflags} doc}

%install
rm -rf $RPM_BUILD_ROOT
%__make DESTDIR="$RPM_BUILD_ROOT" install install-distribution

rm -f $RPM_BUILD_ROOT/%_libdir/*.la $RPM_BUILD_ROOT%{python_sitearch}/*.{la,a}

MANIFEST_CONFIG='%config' \
MANIFEST_CONFIG_NOREPLACE='%config(noreplace)' \
contrib/make-manifest %name $RPM_BUILD_ROOT contrib/manifest.dat

# Generate file list for python package
find "%{buildroot}" -name '*.py' | { while read FILE; do
	f="${FILE#%{buildroot}}"
	echo "${f}"
	# need to touch these files, as they are not produced on FC4 or below
	touch ${FILE}c
	touch ${FILE}o
	echo %%ghost "${f}c"
	echo %%ghost "${f}o"
done } > %name-python.list


%check || :
#%__make check


%clean
rm -rf $RPM_BUILD_ROOT


%post
test -d /vservers      || mkdir -m0000 /vservers
test -d /vservers/.pkg || mkdir -m0755 /vservers/.pkg

f="%confdefaultdir/vdirbase";  test -L "$f" -o -e "$f" || ln -s /vservers                        "$f"
f="%confdefaultdir/run.rev";   test -L "$f" -o -e "$f" || ln -s %_localstatedir/run/vservers.rev "$f"
f="%confdefaultdir/cachebase"; test -L "$f" -o -e "$f" || ln -s %_localstatedir/cache/vservers   "$f"

%_sbindir/setattr --barrier /vservers /vservers/.pkg || :


%preun
test "$1" != 0 || rm -rf %_localstatedir/cache/vservers/* 2>/dev/null || :

# add /bin/vsh to list of secure shells
if [ ! -f /etc/shells ] || ! grep -q '^/bin/vsh$' /etc/shells ; then
    echo /bin/vsh >> /etc/shells
fi


%postun
# 0 = erase, 1 = upgrade
if [ "$1" = 0 ] ; then
    perl -i -n -e 'next if /^\/bin\/vsh$/; print' /etc/shells
fi


%post   lib -p /sbin/ldconfig
%postun lib -p /sbin/ldconfig


%post sysv
#%chkconfig --add vservers-default
#%chkconfig --add vprocunhide
# PlanetLab Node Manager takes care of starting and stopping VServers
%chkconfig --del vservers-default
%chkconfig --del util-vserver
# PlanetLab does not require /proc security
%chkconfig --del vprocunhide
# vip6-autod handles IPv6 auto-assignments
%chkconfig --add vip6-autod


%preun sysv
#test "$1" != 0 || %_initrddir/vprocunhide stop &>/dev/null || :

#test "$1" != 0 || %chkconfig --del vprocunhide
#test "$1" != 0 || %chkconfig --del vservers-default
#test "$1" != 0 || %chkconfig --del util-vserver
test "$1" != 0 || %chkconfig --del vip6-autod


%postun sysv
#test "$1" = 0  || %_initrddir/vprocunhide condrestart >/dev/null || :


%triggerin build -- fedora-release, centos-release
function copy()
{
    base=$1
    shift

    for i; do
	test -r "$i" || continue

	target=%confdir/.distributions/.common/pubkeys/$base-$(basename "$i")
	cp -a "$i" "$target"
    done
}
copy fedora /usr/share/doc/fedora-release-*/RPM-GPG-*
copy fedora /etc/pki/rpm-gpg/RPM-GPG-*
copy centos /usr/share/doc/centos-*/RPM-GPG-KEY-*


%post build
test -d /vservers/.hash || mkdir -m0700 /vservers/.hash

f="%confdefaultdir/apps/vunify/hash"; test -e "$f"/method -o -e "$f"/00 || \
	ln -s /vservers/.hash "$f"/00

%_sbindir/setattr --barrier /vservers/.hash || :


%preun build
test "$1" != 0 || rm -f %confdir/.distributions/.common/pubkeys/fedora-*


## Temporary workaround to remove old v_* files; it will conflict
## somehow with the -legacy package but can be fixed by reinstalling
## this package.
## TODO: remove me in the final .spec file
%define v_services	httpd named portmap sendmail smb sshd xinetd gated
%triggerun sysv -- util-vserver-sysv < 0.30.198
for i in %v_services; do
	%chkconfig --del v_$i || :
done


%post legacy
# PlanetLab Node Manager takes care of starting and stopping VServers
#%chkconfig --add rebootmgr
#%chkconfig --add vservers-legacy

# PlanetLab does not require these legacy services
#for i in %v_services; do
#	%chkconfig --add v_$i
#done


%preun legacy
#test "$1" != 0 || %_initrddir/rebootmgr   stop &>/dev/null || :

#test "$1" != 0 || for i in %v_services; do
#	%chkconfig --del v_$i
#done

#test "$1" != 0 || %chkconfig --del rebootmgr
#test "$1" != 0 || %chkconfig --del vservers-legacy

%postun legacy
#test "$1" = 0  || %_initrddir/rebootmgr   condrestart >/dev/null || :


%files -f %name-base.list
%defattr(-,root,root,-)
%doc AUTHORS COPYING ChangeLog NEWS README THANKS
%doc doc/*.html doc/*.css
/sbin/vshelper
%dir %confdir
%dir %confdefaultdir
%dir %confdefaultdir/apps
%dir %confdefaultdir/files
%dir %pkglibdir/defaults
%ghost %confdefaultdir/cachebase
%ghost %confdefaultdir/vdirbase
%ghost %confdefaultdir/run.rev

%dir %_localstatedir/cache/vservers
%dir %_localstatedir/run/vservers
%dir %_localstatedir/run/vservers.rev
%dir %_localstatedir/run/vshelper


%files lib -f %name-lib.list
%files sysv -f %name-sysv.list


%files core -f %name-core.list
%defattr(-,root,root,-)
%dir %pkglibdir


%files build -f %name-build.list
%defattr(-,root,root,-)
%doc contrib/yum*.patch
%dir %confdir/.distributions
%dir %confdir/.distributions/*
%dir %confdir/.distributions/*/apt
%dir %confdir/.distributions/.common
%dir %confdir/.distributions/.common/pubkeys
%dir %confdefaultdir/apps/vunify
%dir %confdefaultdir/apps/vunify/hash


%files legacy -f %name-legacy.list
%defattr(-,root,root,-)
%dir %pkglibdir/legacy


%files devel -f %name-devel.list
%defattr(-,root,root,-)
%{!?_without_doc:%doc lib/apidoc/latex/refman.pdf}
%{!?_without_doc:%doc lib/apidoc/html}


%files python -f %name-python.list
%{_sbindir}/bwlimit
%{_sbindir}/disklimit
%{python_sitearch}/vserverimpl.so


%changelog
* Wed Oct 03 2007 Daniel Hokka Zakrisson <daniel@hozac.com>
- build python code as part of the regular build process

* Mon Jul 30 2007 Daniel Hokka Zakrisson <daniel@hozac.com>
- add vip6-autod

* Fri Dec 29 2006 Daniel Hokka Zakrisson <daniel@hozac.com> - 0.30.213-0
- add --with legacy and --without doc switches
- add util-vserver initscript

* Fri Feb 17 2006 Steve Muir <smuir@cs.princeton.edu>
- add support for setting guaranteed CPU share flag in rspec

* Sun Jan 22 2006 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.210-0
- do not require 'xalan' anymore by default
- removed 'Requires: apt'; apt-rpm is not maintained upstream anymore
- removed 'chattr' leftovers
- create the '/etc/vservers/.defaults/cachebase' symlink
- added /var/cache/vservers and the needed support
- set barrier attribute on /vservers/.pkg and /vservers/.hash
- added 'centos-release' to the list of packages in the copy-the-keys
  trigger script
- create '/vservers/.hash' and add initial configuration for it

* Fri Jan 13 2006 Steve Muir <smuir@cs.princeton.edu>
- fix bug in python/vserverimpl.c where attempting to adjust CPU share
  for a context that didn't exist would cause an error (it should be a
  safe no-op)

* Fri Dec  2 2005 Steve Muir <smuir@cs.princeton.edu>
- fix bugs in python/vserverimpl.c where exceptions were not raised when
  they should be and thus occured later at unexpected times
- add support for stopping a vserver

* Wed Nov  9 2005 Steve Muir <smuir@cs.princeton.edu>
- add support for removing resource limits e.g., when a slice is deleted

* Mon Nov  7 2005 Steve Muir <smuir@cs.princeton.edu>
- fix file descriptor leak in vduimpl
- clean up handling of network parameters
- don't rely upon /etc/vservers/foo.conf to initialise vserver object

* Wed Nov  2 2005 Steve Muir <smuir@cs.princeton.edu>
- fix Python modules to handling scheduling parameters correctly

* Sun Oct 30 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.209-0
- version 0.30.209
- copy centos keys

* Fri Oct 28 2005 Steve Muir <smuir@cs.princeton.edu>
- raise exception about being over disk limit after setting usage values

* Fri Oct  7 2005 Steve Muir <smuir@cs.princeton.edu>
- create common function to be used for entering a vserver and applying
  resource limits

* Thu Aug 21 2005 Mark Huang <mlhuang@cs.princeton.edu>
- restore build of python modules

* Sat Aug 20 2005 Mark Huang <mlhuang@cs.princeton.edu>
- upgrade to util-vserver-0.30.208
- forward-port vbuild and legacy support until we can find a suitable
  replacement
- make vsh use new vc_create_context() call

* Thu Jul 28 2005 Steve Muir <smuir@cs.princeton.edu>
- add support for static vserver IDs to vuseradd and vuserdel

* Thu Jul 21 2005 Steve Muir <smuir@cs.princeton.edu>
- add bwlimit and cpulimit modules

* Sat Jul 16 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.208-2
- updated URLs

* Fri Jul 15 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.208-1
- version 0.30.208
- require the -lib subpackage by -devel
- copy GPG keys from /etc/pki/rpm-gpg/

* Mon Jun 20 2005 Steve Muir <smuir@cs.princeton.edu>
- import Marc's vdu implementation

* Wed Jun 15 2005 Steve Muir <smuir@cs.princeton.edu>
- 'vserver-init start' functionality subsumed by Node Manager

* Thu Jun 02 2005 Marc E. Fiuczynski <mef@cs.princeton.edu>
- Fixed vlimit command

* Wed May 25 2005 Steve Muir <smuir@cs.princeton.edu>
- add Python modules for manipulating vservers

* Fri Apr 15 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.206-1
- added patches to make yum work in chroot environments
- version 0.30.206

* Thu Apr  7 2005 Steve Muir <smuir@cs.princeton.edu>
- vuserdel changes: don't shutdown vserver, just kill all processes;
  unmount all mountpoints in vserver before deleting

* Thu Mar 24 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.205-0
- added some %%descriptions
- copy GPG keys from the system into the confdir
- buildrequire dietlibc-0.25
- BuildRequire beecrypt-devel
- cleanups
- use %%global instead of %%define
- removed 'run.rev' as a vserver-local variable and made it a system-wide setting

* Wed Jan 26 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.198-0.3
- updated BuildRequires:
- use 'setattr --barrier' instead of 'chattr +t' in the %%post scriptlet
- moved the v_* initscripts to legacy
- do not ship the /vservers directory itself; as it is immutable, the
  extraction will fail else

* Fri Nov 19 2004 Mark Huang <mlhuang@cs.princeton.edu>
- vcached no longer runs as a daemon
- do not restart vservers when package is upgraded

* Wed Nov 17 2004 Mark Huang <mlhuang@cs.princeton.edu> 0.30-6.planetlab
+ planetlab-3_0-rc4
- PL2445
- Both vcached and vuseradd now print a warning message when vbuild
  succeeds but the resulting new vserver image is smaller in size than
  the vserver-reference image.
- vuseradd: clean up some more junk on failure

* Tue Nov 16 2004 Mark Huang <mlhuang@cs.princeton.edu> 0.30-5.planetlab
+ planetlab-3_0-rc3
- PL3026: This is the upgraded version of vdu that maintains an
  internal hash table of files with a nlink count > 1.  Only if vdu
  sees all hard links to a particular inode does it add its size and
  block count to the total.

* Fri Nov 12 2004 Mark Huang <mlhuang@cs.princeton.edu> 0.30-4.planetlab
- PL2445 Use -b option to du to avoid rounding errors.

* Sat Nov  6 2004 Mark Huang <mlhuang@cs.princeton.edu> 0.30-3.planetlab
+ planetlab-3_0-rc2
- don't create the symbolic link /home/slice/.ssh, this is not how
  pl_sshd works

* Mon Oct 11 2004 Marc E. Fiuczynski <mef@cs.princeton.edu>
- added vsh

* Thu Sep  9 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.194-0
- documented switches for 'rpmbuild'

* Wed Aug 11 2004 Mark Huang <mlhuang@cs.princeton.edu> 0.29-1.planetlab
- initial PlanetLab 3.0 build.

* Wed May 26 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.29.215-0
- (re)added the MANIFEST_* variables which were lost some time ago;
  this will preserve %%config files...

* Mon Mar 15 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.29.202-0
- use file-list for sysv scripts also

* Sat Mar  6 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.29.198-0
- added vprocunhide-service support
- added doxygen support
- updated Requires:

* Wed Oct  1 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.23.5-0
- Initial build.
