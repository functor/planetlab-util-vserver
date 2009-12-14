#
# $Id$
# $URL$
#
## This package understands the following switches:
## --without dietlibc        ...   disable usage of dietlibc
## --with    xalan           ...   require/use the xalan xslt processor

## Fedora Extras specific customization below...

%ifarch ppc64
## fails because '__sigsetjmp' and '__longjmp' are undefined
%bcond_with		dietlibc
%else
%bcond_without		dietlibc
%endif

%bcond_with		xalan
##

%global confdir		%_sysconfdir/vservers
%global confdefaultdir	%confdir/.defaults
%global pkglibdir	%_libdir/%name
%global chkconfig	/sbin/chkconfig

%global _localstatedir	%_var

%{!?python_sitearch:%global python_sitearch %(%__python -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

%global release pre2864

Summary:	Linux virtual server utilities
Name:		util-vserver
Version:	0.30.216
Release:	%release
License:	GPLv2
Group:		System Environment/Base
URL:		http://savannah.nongnu.org/projects/util-vserver/
Source0:	http://ftp.linux-vserver.org/pub/utils/util-vserver/%name-%version-%release.tar.bz2
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
BuildRequires:	e2fsprogs-devel nss-devel
BuildRequires:	doxygen tetex-latex graphviz ghostscript
BuildRequires:	libxslt rsync dump
Requires(post):		%name-core
Requires(pre):		%pkglibdir
Requires(postun):	%pkglibdir
%{?with_dietlibc:BuildRequires:	dietlibc}
%{?with_xalan:BuildRequires:	xalan-j}

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
Requires:		rpm wget binutils tar
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
Summary:		Python bindings to develop vserver-based applications
Group:			Development/Libraries
BuildRequires:		python-devel ctags
Requires:		%name-lib = %version-%release


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
util-vserver provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This package contains the files needed to interface with the
Linux-VServer API from Python.


%prep
%setup -q

sed -i -e 's!^\(# chkconfig: \)[0-9]\+ !\1- !' sysv/*


%build
%configure --with-initrddir=%_initrddir --enable-release \
	   --with-crypto-api=nss \
	   --with-python \
	   %{!?with_dietlibc:--disable}%{?with_dietlibc:--enable}-dietlibc

%__make %{?_smp_mflags} all
%__make %{?_smp_mflags} doc


%install
rm -rf $RPM_BUILD_ROOT
%__make DESTDIR="$RPM_BUILD_ROOT" install install-distribution

rm -f $RPM_BUILD_ROOT%_libdir/*.la \
      $RPM_BUILD_ROOT%python_sitearch/*.*a

MANIFEST_CONFIG='%config' \
MANIFEST_CONFIG_NOREPLACE='%config(noreplace)' \
contrib/make-manifest %name $RPM_BUILD_ROOT contrib/manifest.dat


%check
%__make check
LC_NUMERIC=de_DE.UTF-8 ./lib_internal/testsuite/crypto-speed || :


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


%post   lib -p /sbin/ldconfig
%postun lib -p /sbin/ldconfig


%post sysv
%chkconfig --add vservers-default
%chkconfig --add vprocunhide
%chkconfig --add util-vserver


%preun sysv
test "$1" != 0 || %_initrddir/vprocunhide stop &>/dev/null || :

test "$1" != 0 || %chkconfig --del vprocunhide
test "$1" != 0 || %chkconfig --del vservers-default
test "$1" != 0 || %chkconfig --del util-vserver


%postun sysv
test "$1" = 0  || %_initrddir/vprocunhide condrestart >/dev/null || :


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
%chkconfig --add rebootmgr
%chkconfig --add vservers-legacy

for i in %v_services; do
	%chkconfig --add v_$i
done


%preun legacy
test "$1" != 0 || %_initrddir/rebootmgr   stop &>/dev/null || :

test "$1" != 0 || for i in %v_services; do
	%chkconfig --del v_$i
done

test "$1" != 0 || %chkconfig --del rebootmgr
test "$1" != 0 || %chkconfig --del vservers-legacy

%postun legacy
test "$1" = 0  || %_initrddir/rebootmgr   condrestart >/dev/null || :


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
%doc lib/apidoc/latex/refman.pdf
%doc lib/apidoc/html


%files python
%defattr(-,root,root,-)
%python_sitearch/*.so


%changelog
* Sun Aug 23 2009 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.215+svn2847-0
- updated to svn 2847 snapshot
- added -python subpackage

* Sun Jul 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.30.215-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.30.215-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Sat Oct 18 2008 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.215-6
- rebuilt with recent dietlibc

* Sat Oct 18 2008 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.215-5
- fixed build with recent rpm

* Mon Sep  8 2008 Tom "spot" Callaway <tcallawa@redhat.com> - 0.30.215-4
- fix license tag

* Sat Apr 19 2008 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.215-3
- reenabled %%check

* Mon Apr 14 2008 Alex Lancaster <alexlan[AT]fedoraproject org> - 0.30.215-2
- Temporarily disable check to get it to at least build and fix broken
  dep in rawhide/F-9.

* Sat Apr 12 2008 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.215-1
- updated to 0.30.215
- use nss-devel instead of beecrypt-devel as BR
- run crypto-suite benchmark in %%check
- fixed initscript runlevels (#441311)

* Mon Feb 18 2008 Fedora Release Engineering <rel-eng@fedoraproject.org> - 0.30.214-3
- Autorebuild for GCC 4.3

* Wed Sep 19 2007 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.214-2
- fixed upgrade path from 0.30.213 to 0.30.214; rpm fails to handle when
  a directory becomes a symlink. Hence, remove the 'etch' directory in
  %%pre

* Mon Sep  3 2007 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.214-1
- updated to 0.30.214

* Sat Aug  4 2007 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.213-2
- added patch to make 'vyum' work with a patched yum-3.2

* Thu May 31 2007 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.213-1
- updated to 0.30.213
- disabled dietlibc build for PPC64
- enabled support for 'util-vserver' initscript

* Fri Apr 20 2007 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.212-4
- BR some tools tested by ./configure

* Fri Jan 19 2007 David Woodhouse <dwmw2@infradead.org> - 0.30.212-3
- Build with 64KiB page size

* Fri Jan 19 2007 David Woodhouse <dwmw2@infradead.org> - 0.30.212-2
- rebuilt with PPC support

* Sun Dec 10 2006 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.212-1
- updated to 0.30.212
- updated URLs
- requires 'rsync' for -build to support new 'rsync' build method

* Thu Oct 12 2006 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.211-2
- added graphiz + ghostscript BR

* Thu Oct 12 2006 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.211-1
- updated to 0.30.211

* Thu Oct 05 2006 Christian Iseli <Christian.Iseli@licr.org> 0.30.210-5
- rebuilt for unwind info generation, broken in gcc-4.1.1-21

* Mon Sep 18 2006 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.210-4
- rebuilt

* Sun Jul  9 2006 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.210-3
- rebuilt with dietlibc-0.30

* Mon Feb 20 2006 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.210-2
- rebuilt for FC5

* Sun Jan 22 2006 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.210-1
- version 0.30.210
- removed patches which were from upstream

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

* Thu Nov  3 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.209-4
- exclude PPC from build; see
  https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=172389
- added patch to make 'vyum' work with yum-2.4

* Tue Nov  1 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.209-3
- added lot of debug stuff to find out the reason for the 'make check'
  failure on PPC (cflags & personlity get killed there)

* Sun Oct 30 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0.30.209-2
- made sure that ensc_fmt/* it compiled with dietlibc. Else, it will
  fail with the stack-protector in FC5's gcc

* Sun Oct 30 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.209-1
- version 0.30.209
- copy centos keys

* Sat Jul 16 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.208-2
- updated URLs

* Fri Jul 15 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.208-1
- version 0.30.208
- require the -lib subpackage by -devel
- copy GPG keys from /etc/pki/rpm-gpg/

* Fri Apr 15 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.206-1
- added patches to make yum work in chroot environments
- version 0.30.206

* Thu Mar 24 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.205-1
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

* Thu Sep  9 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.194-0
- documented switches for 'rpmbuild'

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
