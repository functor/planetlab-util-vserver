# $Id: util-vserver.spec.in,v 1.49 2005/07/15 19:06:58 ensc Exp $

## This package understands the following switches:
## --without dietlibc        ...   disable usage of dietlibc
## --without xalan           ...   do not require/use the xalan xslt processor

%global confdir		%_sysconfdir/vservers
%global confdefaultdir	%confdir/.defaults
%global pkglibdir	%_libdir/%name
%global __chattr	/usr/bin/chattr
%global chkconfig	/sbin/chkconfig

%global _localstatedir	%_var


%{!?release_func:%global release_func() %1%{?dist}}

Summary:	Linux virtual server utilities
Name:		util-vserver
Version:	0.30.208
Release:	%release_func 0
License:	GPL
Group:		System Environment/Base
URL:		http://savannah.nongnu.org/projects/util-vserver/
Source0:	http://savannah.nongnu.org/download/util-vserver/stable.pkg/%version/%name-%version.tar.bz2
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
BuildRequires:	doxygen tetex-latex
Requires(post):		%__chattr
Requires(pre):		%pkglibdir
Requires(postun):	%pkglibdir
%{!?_without_dietlibc:BuildRequires:	dietlibc >= 0:0.25}
%{!?_without_xalan:BuildRequires:	xalan-j}

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
Requires:		rpm wget binutils tar e2fsprogs
Requires:		%name = %version-%release
Requires(pre):		%confdir
Requires(postun):	%confdir

%ifarch %ix86
Requires:		apt
%endif

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


%prep
%setup -q


%build
%configure --with-initrddir=%_initrddir --enable-release \
           %{?_without_dietlibc:--disable-dietlibc}

%__make %{?_smp_mflags} all
%__make %{?_smp_mflags} doc


%install
rm -rf $RPM_BUILD_ROOT
%__make DESTDIR="$RPM_BUILD_ROOT" install install-distribution

rm -f $RPM_BUILD_ROOT/%_libdir/*.la

MANIFEST_CONFIG='%config' \
MANIFEST_CONFIG_NOREPLACE='%config(noreplace)' \
contrib/make-manifest %name $RPM_BUILD_ROOT contrib/manifest.dat


%check || :
%__make check


%clean
rm -rf $RPM_BUILD_ROOT


%post
test -d /vservers      || mkdir -m0000 /vservers
test -d /vservers/.pkg || mkdir -m0755 /vservers/.pkg

f="%confdefaultdir/vdirbase"; test -L "$f" -o -e "$f" || ln -s /vservers                        "$f"
f="%confdefaultdir/run.rev";  test -L "$f" -o -e "$f" || ln -s %_localstatedir/run/vservers.rev "$f"

%_sbindir/setattr --barrier /vservers || :


%post   lib -p /sbin/ldconfig
%postun lib -p /sbin/ldconfig


%post sysv
%chkconfig --add vservers-default
%chkconfig --add vprocunhide



%preun sysv
test "$1" != 0 || %_initrddir/vprocunhide stop &>/dev/null || :

test "$1" != 0 || %chkconfig --del vprocunhide
test "$1" != 0 || %chkconfig --del vservers-default


%postun sysv
test "$1" = 0  || %_initrddir/vprocunhide condrestart >/dev/null || :


%triggerin build -- fedora-release
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
%ghost %confdefaultdir/vdirbase
%ghost %confdefaultdir/run.rev

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


%changelog
* Fri Jul 15 2005 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.30.208-1
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
