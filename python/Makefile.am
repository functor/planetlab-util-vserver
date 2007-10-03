vserverimpl_la_SOURCES	= vserverimpl.c
vserverimpl_la_LIBADD	= $(top_builddir)/lib/libvserver.la
vserverimpl_la_CFLAGS	= -Wno-redundant-decls -I$(top_srcdir)/lib $(PYTHON_INCLUDES)
vserverimpl_la_LDFLAGS	= -module -avoid-version

pyexec_LTLIBRARIES	= vserverimpl.la
pyexec_DATA		= vserver.py bwlimit.py cpulimit.py
sbin_SCRIPTS		= bwlimit disklimit
