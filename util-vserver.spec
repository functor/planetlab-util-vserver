%define __chattr	/usr/bin/chattr

Summary:	Linux virtual server utilities
Name:		util-vserver
Version:	0.30
Release:	0
Epoch:		0
Copyright:	GPL
Group:		System Environment/Base
URL:		http://savannah.nongnu.org/projects/util-vserver/
Source0:	http://savannah.nongnu.org/download/util-vserver/stable.pkg/%version/%name-%version.tar.bz2
Provides:	%name-devel = %epoch:%version-%release
BuildRoot:	%_tmppath/%name-%version-%release-root
Provides:	vserver = %epoch:%version-%release
Conflicts:	vserver < %epoch:%version-%release
Conflicts:	vserver > %epoch:%version-%release
BuildRequires:	e2fsprogs-devel
Requires(post):	%__chattr

%package linuxconf
Summary:	Linuxconf administration modules for vservers
Group:		Applications/System
Requires:	%name = %epoch:%version-%release
Provides:	vserver-admin = %epoch:%version-%release
Conflicts:	vserver-admin < %epoch:%version-%release
Conflicts:	vserver-admin > %epoch:%version-%release

%description
This package provides the components and a framework to setup virtual
servers.  A virtual server runs inside a linux server. It is nevertheless
highly independent. As such, you can run various services with normal
configuration. The various vservers can't interact with each other and
can't interact with services in the main server.

This requires a special kernel supporting the new new_s_context and
set_ipv4root system call.


%description linuxconf
This package provides the components to setup virtual servers with
linuxconf.


%prep
%setup -q


%build
%configure --enable-linuxconf
%__make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
%__make DESTDIR=$RPM_BUILD_ROOT install

mkdir -p $RPM_BUILD_ROOT/vservers
test "%_initrddir" = %_sysconfdir/init.d || {
	mkdir -p ${RPM_BUILD_ROOT}%_initrddir
	mv ${RPM_BUILD_ROOT}%_sysconfdir/init.d/* ${RPM_BUILD_ROOT}%_initrddir/
}


%clean
rm -rf $RPM_BUILD_ROOT


%define v_services	httpd named portmap sendmail smb sshd xinetd
%post
/sbin/chkconfig --add vservers
/sbin/chkconfig --add rebootmgr

for i in %v_services; do
	/sbin/chkconfig --add v_$i
done

%__chattr +t /vservers || :


%preun
test "$1" != 0 || for i in %v_services; do
	/sbin/chkconfig --del v_$i
done

test "$1" != 0 || %{_initrddir}/rebootmgr stop &>/dev/null || :
test "$1" != 0 || /sbin/chkconfig --del rebootmgr
test "$1" != 0 || /sbin/chkconfig --del vservers


%postun
test "$1" = 0  || %{_initrddir}/rebootmgr condrestart >/dev/null || :


%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog NEWS README THANKS
%_sbindir/*
%_libdir/%name
%_includedir/vserver.h
%_libdir/libvserver.a
%_mandir/man8/*
%config %_initrddir/*
%config(noreplace) /etc/vservers.conf
%attr(0,root,root) %dir /vservers

%exclude %_sbindir/newvserver
%exclude %_mandir/man8/newvserver*


%files linuxconf
%defattr(-,root,root)
%config(noreplace) /etc/vservers/newvserver.defaults
%_sbindir/newvserver
%_mandir/man8/newvserver*


%changelog
* Thu Mar 18 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.29.3-0
- removed '%%doc doc/FAQ.txt' since file does not exist anymore

* Fri Sep 26 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.23.4-1
- initial build.
