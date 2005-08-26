#!/usr/bin/python
#
# Python distutils script for util-vserver Python bindings
#
# Steve Muir <smuir@cs.princeton.edu>
# Mark Huang <mlhuang@cs.princeton.edu>
#
# Copyright (C) 2005 The Trustees of Princeton University
#
# $Id$
#

from distutils.core import setup, Extension

extension_args = {}
extension_args['extra_compile_args'] = ['-Wall']
extension_args['include_dirs'] = ['..', '../lib']
# Link against libvserver with libtool later
#extension_args['library_dirs'] = ['../lib']
#extension_args['libraries'] = ['vserver']

modules = ['util_vserver_vars', 'vserver', 'cpulimit', 'bwlimit']
extensions = [Extension('vduimpl', ['vduimpl.c'], **extension_args),
              Extension('vserverimpl', ['vserverimpl.c'], **extension_args)]

setup(py_modules = modules, ext_modules = extensions)
