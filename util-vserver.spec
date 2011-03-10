# $Id: util-vserver.spec.in 2807 2008-10-30 01:59:52Z dhozac $

%if "%{?_without_python:1}" != "1"
%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%endif

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

%global fullver		0.30.216-pre2939
%global modulever 0.30.216
%global ver		%( echo %fullver | sed 's/-.*//' )
%global subver		%( s=`echo %fullver | grep -- - | sed 's/.*-/./'`; echo ${s:-.1} )

# for module-tools
%global module_version_varname modulever
%global taglevel 15

%{!?release_func:%global release_func() %1%{?dist}}

Summary:	Linux virtual server utilities
Name:		util-vserver
Version:	%ver
Release:	%taglevel
License:	GPL
Group:		System Environment/Base
URL:		http://savannah.nongnu.org/projects/util-vserver/
Source0:	http://ftp.linux-vserver.org/pub/utils/util-vserver/%name-%fullver.tar.bz2
Source1:	fstab
Source2:	vprocunhide-files
Patch1: nomtab.patch
Patch2: upstart.patch
Patch3: sl.patch
BuildRoot:	%_tmppath/%name-%version-%release-root
Requires:	init(%name)
Requires:	%name-core = %version-%release
Requires:	%name-lib  = %version-%release
Requires:	diffutils mktemp sed
Provides:	vserver = %version-%release
Obsoletes:	vserver < %version
BuildRequires:	mount vconfig gawk /sbin/ip iptables
BuildRequires:	gcc-c++ wget which diffutils
BuildRequires:	e2fsprogs-devel e2fsprogs
%{!?_without_beecrypt:BuildRequires: beecrypt-devel}
%{?_without_beecrypt:BuildRequires: nss-devel}
BuildRequires:	e2fsprogs
%{!?_without_doc:BuildRequires:	doxygen tetex-latex}
%{!?_without_python:BuildRequires: python python-devel ctags}
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
%setup -q -n %name-%fullver
%patch1 -p1
%patch2 -p1
%patch3 -p0
autoreconf -fi

%build
%configure --with-initrddir=%_initrddir --enable-release \
           %{?_without_dietlibc:--disable-dietlibc} \
           %{?_with_legacy:--enable-apis=NOLEGACY} \
           --with-initscripts=sysv \
           %{?_without_python:--without-python}

%__make %{?_smp_mflags} all
%{!?_without_doc:%__make %{?_smp_mflags} doc}


%install
rm -rf $RPM_BUILD_ROOT
%__make DESTDIR="$RPM_BUILD_ROOT" install install-distribution

rm -f $RPM_BUILD_ROOT/%_libdir/*.la

MANIFEST_CONFIG='%config' \
MANIFEST_CONFIG_NOREPLACE='%config(noreplace)' \
contrib/make-manifest %name $RPM_BUILD_ROOT contrib/manifest.dat

install -c -m 644 %{SOURCE1} %{buildroot}/%pkglibdir/defaults/fstab
install -c -m 644 %{SOURCE2} %{buildroot}/%pkglibdir/defaults/vprocunhide-files

%check
%__make check


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
%chkconfig vprocunhide on
%chkconfig util-vserver on

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


%pre build
x="%_libdir/util-vserver/distributions/etch"
test -d "$x" && mv "$x" "$x.rpmsave" || :


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
%doc AUTHORS COPYING NEWS README THANKS
#%doc AUTHORS COPYING ChangeLog NEWS README THANKS
#%doc doc/*.html doc/*.css
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


%files python
%defattr(-,root,root,-)
%{!?_without_python:%{python_sitearch}/*}


%changelog
* Thu Mar 10 2011 S.Çağlar Onur <caglar@verivue.com> - util-vserver-0.30.216-15
- * Sync with upstream
- * Add SL6 as a supported distro

* Fri Feb 18 2011 Sapan Bhatia <sapanb@cs.princeton.edu> - util-vserver-0.30.216-14
- Retagging to make sure the tagging operation worked.

* Fri Feb 18 2011 Andy Bavier <acb@cs.princeton.edu> - util-vserver-0.30.216-13
- Add /proc/diskstats to vprocunhide-files

* Mon Jan 31 2011 Andy Bavier <acb@cs.princeton.edu> - util-vserver-0.30.216-12
- add files needed by CoMon to vprocunhide-files list

* Thu Jan 20 2011 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-11
- add a custom vprocunhide-files file which contains /proc/partitions

* Wed Dec 08 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-10
- Enable vprocunhide service

* Wed Dec 01 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-9
- Sync with upstream revision 2926

* Tue Nov 16 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-8
- Sync with upstream revision 2924

* Thu Aug 12 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-7
- Sync with upstream revision 2912

* Mon Aug 09 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-6
- Sync with upstream revision 2908

* Thu Jul 29 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-5
- Fix f12 build

* Wed Jul 28 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-4
- Sync with upstream revision 2902

* Tue Jun 08 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-3
- Do not restart util-vserver service on upgrades

* Tue Jun 01 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-2
- remove tmpfs mounted /tmp from fstab template

* Tue May 11 2010 S.Çağlar Onur <caglar@cs.princeton.edu> - util-vserver-0.30.216-1

* Mon Jun 25 2007 Daniel Hokka Zakrisson <daniel@hozac.com> - 0.30.214-0
- updated URLs
- get rid of e2fsprogs requirement

* Fri Dec 29 2006 Daniel Hokka Zakrisson <daniel@hozac.com> - 0.30.213-0
- add --with legacy and --without doc switches
- add util-vserver initscript

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

* Sun Oct 30 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.209-0
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
