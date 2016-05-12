## Run automake on this file to produce Makefile.in

SUBDIRS = src doc

dist_icon_DATA = xfig.png

appdefault_DATA = app-defaults/Fig

dist_applications_DATA = xfig.desktop

dist_compkeydb_DATA = CompKeyDB

EXTRA_DIST = version.m4 CHANGES FIGAPPS app-defaults \
	LATEX.AND.XFIG LATEX.AND.XFIG.zh_CN LATEX.AND.XFIG.zh_TW

xfig_librariesdir = Libraries

if XFIG_LIBRARIES
EXTRA_DIST += $(xfig_librariesdir)
endif

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.h.in config.h.in~ configure

if XFIG_LIBRARIES
# Libraries might not be distributed - so test, whether the
# source is there, before trying to copy it to the destination.
install-data-hook:
	test -d "$(srcdir)/$(xfig_librariesdir)" \
	&& $(MKDIR_P) "$(DESTDIR)$(pkgdatadir)" \
	&& cp -Rp "$(srcdir)/$(xfig_librariesdir)" "$(DESTDIR)$(pkgdatadir)" \
	&& chmod -R u+rwX "$(DESTDIR)$(pkgdatadir)"

uninstall-local:
	test -d "$(DESTDIR)$(pkgdatadir)/$(xfig_librariesdir)" \
	&& rm -rf "$(DESTDIR)$(pkgdatadir)/$(xfig_librariesdir)"
endif
