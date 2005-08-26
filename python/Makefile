#
# GNUMakefile for util-vserver Python bindings
#
# It's too hard to integrate distutils into the autoconf/libtool
# framework, so run this Makefile separately from and after the normal
# util-vserver build.
#
# Steve Muir <smuir@cs.princeton.edu>
# Mark Huang <mlhuang@cs.princeton.edu>
# Copyright (C) 2005 The Trustees of Princeton University
#
# $Id$
#

ALL := vserverimpl.so vduimpl.so util_vserver_vars.py

pythonlibdir := @libdir@/python@PYTHON_VERSION@/site-packages

all: $(ALL)

%.o: %.c
        # builds object and incompletely linked library
	python setup.py build_ext
        # copy to current directory
	cp -a build/temp.*/*.o .

vserverimpl.so vduimpl.so: %.so: %.o
        # relink the object against libvserver with libtool
	../libtool --tag=CC --mode=link $(CC) -shared -o $@ $< ../lib/libvserver.la

util_vserver_vars.py: ../scripts/util-vserver-vars
	install -m 644 $< $@

install: $(ALL)
        # install relinked libraries and byte-compiled scripts
	python setup.py install --root="$(DESTDIR)"
        # reinstall libraries with libtool so that the final path
        # to libvserver is resolved
	for so in $(filter %.so, $(ALL)) ; do \
	  ../libtool --tag=CC --mode=install install "$$so" `find "$(DESTDIR)" -name "$$so"` ; \
	done

clean:
	rm -rf $(ALL) *.o build

.PHONY: all install clean
