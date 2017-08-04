bin_PROGRAMS = mpg321

mpg321_SOURCES = \
	mpg321.c \
	mad.c \
	playlist.c \
	network.c \
	mpg321.h \
	remote.c \
	ao.c \
	options.c \
	auth.c \
	scrobbler.c \
	fft.c \
	fbuffer.c \
	volume.c

SUBDIRS = 
EXTRA_DIST = README.remote HACKING BUGS mpg321.sgml mpg321.1 $(srcdir)/debian/*

man_MANS = mpg321.1

install-exec-hook:
if DO_SYMLINK
	-cd $(DESTDIR)/$(bindir) && $(LN_S) mpg321 mpg123 
endif

uninstall-hook:
if DO_SYMLINK
	if [ -L $(DESTDIR)/$(bindir)/mpg123 ]; then \
		echo "The $(DESTDIR)/$(bindir)/mpg123 symlink still exists. You may want to remove it."\
	fi	
endif

dist-hook:
	-for i in `find $(distdir) -name "CVS" -print`; do rm -r $$i; done
