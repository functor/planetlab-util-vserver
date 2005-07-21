INCLUDES := -I.. -I../lib
LIBS = -L../lib -lvserver

PY_MODS := vserver.py cpulimit.py bwlimit.py
PY_EXT_MODS := vduimpl.so vserverimpl.so

all: py-build

install: py-install
	ln -s /usr/lib/util-vserver/util-vserver-vars \
	$(INSTALL_ROOT)/usr/lib/python2.3/site-packages/util_vserver_vars.py

-include pybuild.mk

.PHONY: all install
