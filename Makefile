# Makefile for CDimg|tools
# Copyright © 2012,2013 Géraud Meyer <graud@gmx.com>
#   This file is part of CDimg|tools.
#
#   CDimg|tools is free software; you can redistribute it and/or modify it under
#   the terms of the GNU General Public License version 2 as published by the
#   Free Software Foundation.
#
#   This package is distributed in the hope that it will be useful, but WITHOUT
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#   more details.
#
#   You should have received a copy of the GNU General Public License
#   along with CDimg|tools.  If not, see <http://www.gnu.org/licenses/>.

# This a GNU Makefile.  Under BSD systems run gmake.

# Include the settings chosen by the configure script
# If it is not found, default values are used
-include config.make

prefix	?= $(HOME)/.local
bindir	?= $(prefix)/bin
datarootdir	?= $(prefix)/share
sysconfdir	?= $(prefix)/etc
docdir	?= $(datarootdir)/doc/$(PACKAGE_TARNAME)
mandir	?= $(datarootdir)/man
# DESTDIR =  # distributors set this on the command line

empty	:=
tab	:= $(empty)	$(empty)

PACKAGE_NAME	?= CDimg|tools
PACKAGE_TARNAME	?= cdimgtools
HAVE_CONFIG_H	= $(shell test -e config.h && echo 1 || echo 0)

# Get the version via git or from the VERSION file or from the project
# directory name.
VERSION	= $(shell test -x version.sh && ./version.sh $(PACKAGE_TARNAME) \
	  || echo "unknown_version")
# Allow either to be overwritten by setting DIST_VERSION on the command line.
ifdef DIST_VERSION
VERSION	= $(DIST_VERSION)
endif
PACKAGE_VERSION	= $(VERSION)

# Remove the _g<SHA1> part from the $VERSION
RPM_VERSION	= $(shell echo $(VERSION) | $(SED) -e 's/_g[0-9a-z]\+//')
# Get the release from the spec file
RPM_RELEASE	= $(shell $(SED) -n -e 's/^Release:[ $(tab)]\+\(.*\)%.*$$/\1/p' \
	  $(PACKAGE_TARNAME).spec.in)
DEB_RELEASE	= 1

PROGS	= cssdec dvdimgdecss
SCRIPTS	= raw96cdconv nrgtool
TESTS	= 
SOURCE	= README INSTALL COPYING BUGS NEWS
PERLDOC = raw96cdconv nrgtool
MANDOC	= $(PERLDOC:%=%.1) $(PROGS:%=%.1)
HTMLDOC	= $(PERLDOC:%=%.1.html) $(PROGS:%=%.1.html) README.html INSTALL.html NEWS.html
RELEASEDOC = $(MANDOC) $(HTMLDOC)
ALLDOC	= $(PERLDOC:%=%.1.txt) $(MANDOC) $(HTMLDOC)

TARNAME	= $(PACKAGE_TARNAME)-$(RPM_VERSION)

MV	?= mv
CP	?= cp
RM	?= rm
LN	?= ln
MKDIR	?= mkdir
INSTALL	?= install
SED	?= sed
TAR	?= tar
TAR_FLAGS	= --owner root --group root --mode a+rX,o-w --mtime .
RPMBUILD_FLAGS	= --nodeps # in case we are not on an rpm system
DEBUILD_FLAGS	= -d # in case we are not on a deb system
MAKE	?= make
AUTORECONF	?= autoreconf
ASCIIDOC	?= asciidoc
ASCIIDOC_FLAGS	= -apackagename="$(PACKAGE_NAME)" -aversion="$(VERSION)"
POD2MAN	?= pod2man
POD2MAN_FLAGS = --utf8 -c "$(PACKAGE_NAME) commands" -r "$(PACKAGE_NAME) $(VERSION)"
POD2HTML	?= pod2html
POD2TEXT	?= pod2text
XMLTO	?= xmlto
MD5SUM	?= md5sum
SHA512SUM	?= sha512sum

PERL	?= /usr/bin/perl

all: build
.help:
	@echo "Available targets for $(PACKAGE_NAME) Makefile:"
	@echo "	.help all configure build clean doc doc-txt doc-man doc-html"
	@echo "	ChangeLog dist rpm deb distclean maintainer-clean debclean"
	@echo "	install install-doc install-doc-man install-doc-html"
	@echo "Useful variables for $(PACKAGE_NAME) Makefile:"
	@echo "	CFLAGS CPPFLAGS LDFLAGS prefix DESTDIR RPMBUILD_FLAGS DEBUILD_FLAGS"
help: .help
.PHONY: .help help all build clean doc doc-txt doc-man doc-html \
	dist nodocdist rpm deb deborig distclean maintainer-clean debuild_clean debclean \
	install install-doc install-doc-man install-doc-html

build: $(PROGS) $(TESTS)
doc: $(ALLDOC)
doc-txt: $(PERLDOC:%=%.1.txt)
doc-man: $(MANDOC)
doc-html: $(HTMLDOC)

install: all
	$(MKDIR) -p $(DESTDIR)$(bindir)
	set -e; for prog in $(SCRIPTS); do \
		$(SED) -e '1 s#/usr/bin/perl#$(PERL)#' "$$prog" > "$$prog".ins; \
	done
	set -e; for prog in $(PROGS) $(SCRIPTS:%=%.ins); do \
		$(INSTALL) -p -m 0755 "$$prog" "$(DESTDIR)$(bindir)/$${prog%.ins}"; \
	done
	rm $(SCRIPTS:%=%.ins)
install-doc: install-doc-man install-doc-html
install-doc-man: doc-man
	$(MKDIR) -p $(DESTDIR)$(mandir)/man1
	set -e; for doc in $(MANDOC); do \
		gzip -9 <"$$doc" >"$$doc".gz; \
		$(INSTALL) -p -m 0644 "$$doc".gz "$(DESTDIR)$(mandir)/man1/"; \
		$(RM) "$$doc".gz; \
	done
install-doc-html: doc-html
	$(MKDIR) -p $(DESTDIR)$(docdir)
	set -e; for doc in $(HTMLDOC); do \
		case "$$doc" in \
		(*.html) $(INSTALL) -p -m 0644 "$$doc" "$(DESTDIR)$(docdir)/" ;; \
		esac; \
	done

clean:
	$(RM) -r $(TARNAME)
	$(RM) $(PACKAGE_TARNAME)-*.tar $(PACKAGE_TARNAME)-*.tar.gz \
	  $(PACKAGE_TARNAME)-*.tar.gz.md5 $(PACKAGE_TARNAME)-*.tar.gz.sha512 \
	  $(PACKAGE_TARNAME)-*.tar.gz.sig $(PACKAGE_TARNAME)-*.tar.gz.asc
	$(RM) $(PROGS) $(TESTS) *.xml pod2htm* *~
distclean: clean
	$(RM) -r autom4te.cache/
	$(RM) $(ALLDOC) $(PACKAGE_TARNAME).spec aclocal.m4 autoscan.log
	$(RM) config.h config.log config.make config.status
maintainer-clean: distclean
	$(RM) configure config.h.in debian/changelog
	test ! -d .git && test ! -f .git || $(RM) ChangeLog
debuild_clean:
	-debuild clean
debclean: debuild_clean maintainer-clean
	$(RM) -r .pc/ debian/patches/

$(TARNAME).tar: configure config.h.in ChangeLog $(PACKAGE_TARNAME).spec debian/changelog
	$(MKDIR) -p $(TARNAME)/debian
	echo $(VERSION) > $(TARNAME)/VERSION
	$(CP) -p configure config.h.in ChangeLog $(PACKAGE_TARNAME).spec $(TARNAME)
	$(CP) -p debian/changelog $(TARNAME)/debian
	git archive --format=tar --prefix=$(TARNAME)/ HEAD > $(TARNAME).tar
	$(TAR) $(TAR_FLAGS) -rf $(TARNAME).tar $(TARNAME)
	$(RM) -r $(TARNAME)
dist: $(TARNAME).tar.gz
nodocdist:
	$(MAKE) $(TARNAME).tar.gz NODOCDIST=yes
$(TARNAME).tar.gz:
	set -e; \
	if test -f ../$(TARNAME).tar.gz; then \
		echo NOTE: re-using already existing ../$(TARNAME).tar.gz; \
		$(CP) ../$(TARNAME).tar.gz .; \
	else \
		if test -d .git || test -f .git; then \
			$(MAKE) $(TARNAME).tar; \
		else \
			echo WARNING: re-building the distribution archive in ../$(TARNAME).tar; \
			sleep 3; \
			$(MAKE) distclean; \
			  # most generated distribution files are preserved \
			$(MAKE) $(PACKAGE_TARNAME).spec; \
			( cd .. && $(TAR) $(TAR_FLAGS) --mtime $(TARNAME) -cvf $(TARNAME).tar $(TARNAME) \
			  && $(MV) $(TARNAME).tar $(TARNAME)/ ); \
		fi; \
		if test x$(NODOCDIST) = x; then \
			$(MAKE) $(RELEASEDOC); \
			$(MKDIR) $(TARNAME); \
			$(CP) -p -P $(RELEASEDOC) $(TARNAME); \
			$(TAR) $(TAR_FLAGS) -rf $(TARNAME).tar $(TARNAME); \
			$(RM) -r $(TARNAME); \
		fi; \
		gzip -f -9 $(TARNAME).tar; \
	fi
	$(MD5SUM) $(TARNAME).tar.gz > $(TARNAME).tar.gz.md5
	$(SHA512SUM) $(TARNAME).tar.gz > $(TARNAME).tar.gz.sha512
rpm: dist
	rpmbuild $(RPMBUILD_FLAGS) -ta $(TARNAME).tar.gz
deborig:
	set -e; if test -f ../$(PACKAGE_TARNAME)_$(RPM_VERSION).orig.tar.gz; then \
		echo "WARNING: re-using ../$(PACKAGE_TARNAME)_$(RPM_VERSION).orig.tar.gz"; \
		sleep 3; \
	else \
		$(MAKE) $(TARNAME).tar.gz; \
		$(MV) $(TARNAME).tar.gz ../$(PACKAGE_TARNAME)_$(RPM_VERSION).orig.tar.gz; \
	fi
deb: deborig debian/changelog
	debuild $(DEBUILD_FLAGS)

configure: configure.ac
	$(AUTORECONF) -v
#config.h.in: configure

ChangeLog:
	( echo "# $@ - automatically generated from the VCS's history"; \
	  echo; \
	  ./gitchangelog.sh --tags --tag-pattern 'version\/[^\n]*' \
	    -- - --date-order ) \
	| $(SED) 's/^\[version/\[version/' \
	> $@
$(PACKAGE_TARNAME).spec: $(PACKAGE_TARNAME).spec.in
	$(SED) -e 's/@@VERSION@@/$(RPM_VERSION)/g' \
	    -e 's/@@RELEASE@@/$(RPM_RELEASE)/g' < $< > $@
debian/changelog: debian/changelog.in
	$(SED) -e 's/@@VERSION@@/$(RPM_VERSION)/g' \
	    -e 's/@@RELEASE@@/$(DEB_RELEASE)/g' < $< > $@
	-debchange -r ""

cssdec: cssdec.c
	$(CC) $(CPPFLAGS) -DHAVE_CONFIG_H=$(HAVE_CONFIG_H) \
		$(CFLAGS) $(LDFLAGS) -o $@ $< -ldvdcss
dvdimgdecss: dvdimgdecss.c
	$(CC) $(CPPFLAGS) -DHAVE_CONFIG_H=$(HAVE_CONFIG_H) \
		$(CFLAGS) $(LDFLAGS) -o $@ $< -ldvdcss -ldvdread


README.html: README BUGS asciidoc.conf
	$(ASCIIDOC) $(ASCIIDOC_FLAGS) -b xhtml11 -d article -a readme $<
INSTALL.html: INSTALL asciidoc.conf
	$(ASCIIDOC) $(ASCIIDOC_FLAGS) -b xhtml11 -d article -a readme $<
NEWS.html: NEWS asciidoc.conf
	$(ASCIIDOC) $(ASCIIDOC_FLAGS) -b xhtml11 -d article $<

$(PERLDOC:%=%.1): $(PERLDOC)
	$(POD2MAN) $(POD2MAN_FLAGS) $(patsubst %.1,%,$@) >$@
$(PERLDOC:%=%.1.txt): $(PERLDOC)
	$(POD2TEXT) --utf8 $(patsubst %.1.txt,%,$@) >$@
$(PERLDOC:%=%.1.html): $(PERLDOC)
	$(POD2HTML) --noindex $(patsubst %.1.html,%,$@) >$@

%.html: %.txt asciidoc.conf
	$(ASCIIDOC) $(ASCIIDOC_FLAGS) -b xhtml11 -d manpage $<
%.xml: %.txt asciidoc.conf
	$(ASCIIDOC) $(ASCIIDOC_FLAGS) -b docbook -d manpage $<
%: %.xml
	$(XMLTO) man $<
