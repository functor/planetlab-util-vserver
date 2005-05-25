from distutils.core import setup, Extension

# XXX - need a way to share crap with the Makefile
setup(name = "util-vserver",
      version = "0.1",
      description = "Python modules for vserver operations",
      author = "Steve Muir",
      author_email = "smuir@cs.princeton.edu",
      py_modules = ["vserver"],
      ext_modules = [Extension("vserverimpl",
                               ["vserverimpl.c"],
                               include_dirs = ["..", "../lib"],
                               library_dirs = ["../lib"],
                               libraries = ["vserver"])])
