%define name util-vserver
%define version 0.30
%define release 6.planetlab%{?date:.%{date}}

Vendor: PlanetLab
Packager: PlanetLab Central <support@planet-lab.org>
Distribution: PlanetLab 3.0
URL: http://cvs.planet-lab.org/cvs/util-vserver

%define __chattr	/usr/bin/chattr

Summary:	Linux virtual server utilities
Name:		%{name}
Version:	%{version}
Release:	%{release}
Epoch:		0
Copyright:	GPL
Group:		System Environment/Base
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
aclocal -I m4
autoconf
automake --add-missing
# bootstrap to avoid BuildRequires of kernel-source
for linux in $RPM_BUILD_DIR/linux-* /lib/modules/`uname -r`/build ; do
   [[ -d $linux/include ]] && %configure --with-kerneldir=$linux --enable-linuxconf && break
done


%build
make

%install
rm -rf $RPM_BUILD_ROOT
%__make DESTDIR=$RPM_BUILD_ROOT install

mkdir -p $RPM_BUILD_ROOT/vservers
test "%_initrddir" = %_sysconfdir/init.d || {
	mkdir -p ${RPM_BUILD_ROOT}%_initrddir
	mv ${RPM_BUILD_ROOT}%_sysconfdir/init.d/* ${RPM_BUILD_ROOT}%_initrddir/
}

mkdir -p ${RPM_BUILD_ROOT}/bin
ln -f ${RPM_BUILD_ROOT}%_sbindir/vsh ${RPM_BUILD_ROOT}/bin/vsh

install -D -m 644 sysv/vcached.logrotate ${RPM_BUILD_ROOT}/etc/logrotate.d/vcached

%clean
rm -rf $RPM_BUILD_ROOT

%define services vcached vservers

%pre
# 1 = install, 2 = upgrade/reinstall
if [ $1 -eq 2 ] ; then
    for i in %{services} ; do
	[ "`/sbin/runlevel`" = "unknown" ] || service $i stop || :
    done
fi

%post
# 1 = install, 2 = upgrade/reinstall
if [ $1 -eq 1 ] ; then
    for i in %{services} ; do
	chkconfig --add $i
	chkconfig $i on
    done
fi
for i in %{services} ; do
    [ "`/sbin/runlevel`" = "unknown" ] || service $i start
done
if [ ! -f /etc/shells ] || ! grep -q '^/bin/vsh$' /etc/shells ; then
    echo /bin/vsh >> /etc/shells
fi

%__chattr +t /vservers || :


%postun
# 0 = erase, 1 = upgrade
if [ "$1" = 0 ] ; then
    perl -i -n -e 'next if /^\/bin\/vsh$/; print' /etc/shells
fi

%preun
# 0 = erase, 1 = upgrade
if [ $1 -eq 0 ] ; then
    for i in %{services} ; do
	[ "`/sbin/runlevel`" = "unknown" ] || service $i stop || :
	chkconfig $i off
	chkconfig --del $i
    done
fi

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
%config(noreplace) /etc/vcached.conf
/etc/logrotate.d/vcached
%dir /etc/vservers
%attr(0,root,root) %dir /vservers
%attr(4755,root,root) /usr/sbin/vsh
%attr(4755,root,root) /bin/vsh

%exclude %_sbindir/newvserver
%exclude %_mandir/man8/newvserver*

%files linuxconf
%defattr(-,root,root)
%config(noreplace) /etc/vservers/newvserver.defaults
%_sbindir/newvserver
%_mandir/man8/newvserver*

%changelog
* Mon Oct 11 2004 Marc E. Fiuczynski <mef@cs.princeton.edu> 0.1-1.planetlab
- added vsh

* Wed Aug 11 2004 Mark Huang <mlhuang@cs.princeton.edu> 0.29-1.planetlab
- initial PlanetLab 3.0 build.

* Thu Mar 18 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.29.3-0
- removed '%%doc doc/FAQ.txt' since file does not exist anymore

* Fri Sep 26 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de> - 0:0.23.4-1
- initial build.

