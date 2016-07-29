SUBDIRS = src config

EXTRA_DIST = \
	TODO \
	autogen.sh 

BZIP2_ENV =

release: distdir
	-chmod -R a+r $(distdir)
	GZIP=$(GZIP_ENV) $(AMTAR)$(TAR) chozf $(distdir).tar.gz $(distdir)
	rm -f $(distdir)/gkrellmbups.spec
	sed '/^Source:/s/\.tar\.gz$$/\.tar\.bz2/' gkrellmbups.spec \
	  > $(distdir)/gkrellmbups.spec
	BZIP2=$(BZIP2_ENV) $(AMTAR)$(TAR) --bzip2 -chof $(distdir).tar.bz2 $(distdir)
	-rm -rf $(distdir)
