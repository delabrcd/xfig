## Run automake on this file to produce Makefile.in
# This file is part of FIG - Facility for Interactive Generation of figures
#
# Copyright (c) 2016 Thomas Loimer <thomas.loimer@tuwien.ac.at>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved. This file is offered as-is,
# without any warranty.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src doc tests

dist_icon_DATA = xfig.png

appdefault_DATA = app-defaults/Fig

dist_applications_DATA = xfig.desktop

dist_compkeydb_DATA = CompKeyDB

EXTRA_DIST = CHANGES FIGAPPS app-defaults xpm_splash_logo \
	LATEX.AND.XFIG LATEX.AND.XFIG.zh_CN LATEX.AND.XFIG.zh_TW

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.h.in config.h.in~ configure

version.m4: configure.ac Makefile.am src/*.c src/*.h
	if test `git describe` && test "`git describe --abbrev=1`" \
		!= "`sed -n '/XFIG_V/ {s/.*\[\(.*\)\])/\1/;p}' $@`"; then \
	sed -i "s/VERSION\], \[.*\]/VERSION], [`git describe --abbrev=1`]/" $@ \
	&& sed -i "s/DATE\], \[.*\]/DATE], [`LC_TIME=C date +'%b %Y' -r $@`]/" $@; \
	fi
# To compare git date with version.m4 date; Easier with wildcards, like above.
# && test `git show -s --pretty=%ct` gt `date +%s -r $@`

# a fragment, to test compilation with different configure-flags
compile-check: distdir
	chmod u+w $(distdir)
	mkdir $(distdir)/_build $(distdir)/_build/sub
	cd $(distdir)/_build/sub \
	&& ../../configure --srcdir=../.. --without-xfig-libraries --disable-jpeg \
	&& $(MAKE) -j $(AM_MAKEFLAGS) \
	&& $(MAKE) $(AM_MAKEFLAGS) check \
	&& $(MAKE) $(AM_MAKEFLAGS) clean
	cd $(distdir)/_build/sub \
	../../configure --srcdir=../.. --without-xfig-libraries --disable-xpm --enable-xpm-icon \
	&& $(MAKE) -j $(AM_MAKEFLAGS) \
	&& $(MAKE) $(AM_MAKEFLAGS) check \
	&& $(MAKE) $(AM_MAKEFLAGS) clean

xfig_librariesdir = Libraries

if XFIG_LIBRARIES
EXTRA_DIST += $(xfig_librariesdir)

# do not distribute Maps/USA/assemble and Maps/Canada/assemble
# See info automake, 14.3 The dist Hook, for an example with chmod u+w.
dist-hook:
	test -d "$(distdir)/$(xfig_librariesdir)/Maps" \
	&& cd "$(distdir)/$(xfig_librariesdir)/Maps" && \
		for p in USA Canada; do \
			test -f "$$p/assemble" \
			&& { test -w "$$p" || chmod u+w "$$p"; } \
			&& rm -f "$$p/assemble" || :; \
		done || :

# Libraries might not be distributed - so test, whether the
# source is there, before trying to copy it to the destination.
install-data-hook:
	test -d "$(srcdir)/$(xfig_librariesdir)" && { \
		$(MKDIR_P) "$(DESTDIR)$(pkgdatadir)" \
		&& cp -Rp "$(srcdir)/$(xfig_librariesdir)" "$(DESTDIR)$(pkgdatadir)" \
		&& chmod -R u+rwX "$(DESTDIR)$(pkgdatadir)" || exit 1; } \
		|| :

uninstall-local:
	test -d "$(DESTDIR)$(pkgdatadir)/$(xfig_librariesdir)" \
	&& rm -rf "$(DESTDIR)$(pkgdatadir)/$(xfig_librariesdir)" || :
endif
