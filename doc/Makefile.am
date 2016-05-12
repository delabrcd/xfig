## Run automake on this file to produce Makefile.in

dist_doc_DATA = FORMAT1.3 FORMAT1.4 FORMAT2.0 FORMAT2.1 FORMAT3.0 \
	FORMAT3.1 FORMAT3.2 xfig-howto.pdf xfig_ref_en.pdf

man1_MANS = xfig.man

install-data-hook:
	cp -Rp "$(srcdir)/html" "$(DESTDIR)$(htmldir)" \
	&& chmod -R u+rwX "$(DESTDIR)$(htmldir)"

uninstall-local:
	rm -rf "$(DESTDIR)$(htmldir)/html"

EXTRA_DIST  = xfig.man.orig html MAKEPS

MAINTAINERCLEANFILES = Makefile.in
