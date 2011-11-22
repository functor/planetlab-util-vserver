#
WEBFETCH		:= wget
SHA1SUM			:= sha1sum

ALL			+= util-vserver
util-vserver-URL1      := http://build.planet-lab.org/third-party/util-vserver-0.30.216-pre2939.tar.bz2
util-vserver-URL2      := http://mirror.onelab.eu/third-party/util-vserver-0.30.216-pre2939.tar.bz2
util-vserver-SHA1SUM   := 2fc905190fed57a4a31b6191c6d085cd699e65b6
util-vserver		:= $(notdir $(util-vserver-URL1))

all: $(ALL)
.PHONY: all

##############################
define download_target
$(1): $($(1))
.PHONY: $(1)
$($(1)): 
	@if [ ! -e "$($(1))" ] ; then \
	{ echo Using primary; echo "$(WEBFETCH) $($(1)-URL1)" ; $(WEBFETCH) $($(1)-URL1) ; } || \
	{ echo Using secondary; echo "$(WEBFETCH) $($(1)-URL2)" ; $(WEBFETCH) $($(1)-URL2) ; } ; fi
	@if [ ! -e "$($(1))" ] ; then echo "Could not download source file: $($(1)) does not exist" ; exit 1 ; fi
	@if test "$$$$($(SHA1SUM) $($(1)) | awk '{print $$$$1}')" != "$($(1)-SHA1SUM)" ; then \
	    echo "sha1sum of the downloaded $($(1)) does not match the one from 'Makefile'" ; \
	    echo "Local copy: $$$$($(SHA1SUM) $($(1)))" ; \
	    echo "In Makefile: $($(1)-SHA1SUM)" ; \
	    false ; \
	else \
	    ls -l $($(1)) ; \
	fi
endef

$(eval $(call download_target,util-vserver))

sources: $(ALL) 
.PHONY: sources

####################
# default - overridden by the build
SPECFILE = util-vserver.spec

PWD=$(shell pwd)
PREPARCH ?= noarch
RPMDIRDEFS = --define "_sourcedir $(PWD)" --define "_builddir $(PWD)" --define "_srcrpmdir $(PWD)" --define "_rpmdir $(PWD)"
trees: sources
	rpmbuild $(RPMDIRDEFS) $(RPMDEFS) --nodeps -bp --target $(PREPARCH) $(SPECFILE)

srpm: sources
	rpmbuild $(RPMDIRDEFS) $(RPMDEFS) --nodeps -bs $(SPECFILE)

TARGET ?= $(shell uname -m)
rpm: sources
	rpmbuild $(RPMDIRDEFS) $(RPMDEFS) --nodeps --target $(TARGET) -bb $(SPECFILE)

clean:
	rm -f *.rpm *.tgz *.bz2 *.gz

++%: varname=$(subst +,,$@)
++%:
	@echo "$(varname)=$($(varname))"
+%: varname=$(subst +,,$@)
+%:
	@echo "$($(varname))"
