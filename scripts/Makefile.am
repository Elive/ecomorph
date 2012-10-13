MAINTAINERCLEANFILES = Makefile.in
filesdir = $(DESTDIR)$(prefix)/bin
files_DATA = ecomp.sh


install-filesDATA:
	test -z $(filesdir) || /bin/mkdir -p $(filesdir)
	install -c -m 755 'ecomp.sh' '$(filesdir)/ecomp.sh'
