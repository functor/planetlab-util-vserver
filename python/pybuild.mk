# Copyright 2005 Princeton University
#
# PY_MODS variable should be list of Python source modules
# PY_EXT_MODS should be list of Python extension modules (.so) that need
# to be built
#

_PY_TMPDIR := .pybuild

# use strip to remove extra whitespace
_PY_SETUP = $(strip python setup.py \
	$(foreach i,DEFS INCLUDES LIBS PY_MODS PY_EXT_MODS,\
	$(if $(subst undefined,,$(origin $i)),$i="$($i)")))

_PY_BUILD_ARGS := -t $(_PY_TMPDIR) -b $(_PY_TMPDIR)/build

py-build: .pybuild/.prep-done $(PY_MODS) $(PY_EXT_MODS)

.pybuild/.prep-done:
	mkdir build .pybuild
	ln -s ../build .pybuild
	touch $@

$(PY_EXT_MODS): PY_EXT_MODS = $@

$(PY_EXT_MODS): %.so: %.c
	$(_PY_SETUP) build_ext -f $(_PY_BUILD_ARGS)

py-install:
	$(_PY_SETUP) install --root=$(INSTALL_ROOT)

.PHONY: py-build py-install py-clean
