#
# GNUMakefile for util-vserver Python bindings
#
# Steve Muir <smuir@cs.princeton.edu>
# Mark Huang <mlhuang@cs.princeton.edu>
# Copyright (C) 2005 The Trustees of Princeton University
#
# $Id: Makefile,v 1.10 2005/11/01 22:02:04 smuir Exp $
#

INCLUDES := -I.. -I../lib
LIBS = -L../lib -lvserver

PY_MODS := vserver.py cpulimit.py bwlimit.py plcapi.py
PY_EXT_MODS := vduimpl.so vserverimpl.so

LT_LINK = ../libtool --tag=CC --mode=link



all: py-build

# XXX - compatibility with util-vserver specfile
INSTALL_ROOT ?= $(DESTDIR)

install: py-install

clean: py-clean



UTIL_PYTHON ?= $(wildcard ../../util-python*)

include $(UTIL_PYTHON)/pybuild.mk

.PHONY: all install clean
