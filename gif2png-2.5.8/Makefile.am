## Process this file with automake to produce Makefile.in
bin_PROGRAMS = gif2png
bin_SCRIPTS = web2png
gif2png_SOURCES = 437_l1.c gif2png.c gifread.c memory.c version.c gif2png.h
man_MANS = gif2png.1 web2png.1
# automake is buggy, it ought to include bin_SCRIPTS in the distribution
EXTRA_DIST = Makefile Makefile.bcc Makefile.djgpp gif2png.xml web2png.xml gif2png.1 $(man_MANS) $(bin_SCRIPTS) ChangeLog gif2png-logo.png
CLEANFILES = gif2png.1 web2png.1

gif2png.1: gif2png.xml
	xmlto man gif2png.xml

gif2png.html: gif2png.xml
	xmlto html-nochunks gif2png.xml

web2png.1: web2png.xml
	xmlto man web2png.xml

PYLINTOPTS = --rcfile=/dev/null --reports=n --include-ids=y \
	--disable=C0111,C0103,C0301,C0321,W0402,W0141,W0401,W0614,R0912,R0914,R0915
pylint:
	@pylint --output-format=parseable $(PYLINTOPTS) web2png

SPLINTOPTS = -warnposix +charint -exitarg -compdestroy -exportlocal
splint:
	splint +quiet $(SPLINTOPTS) $(gif2png_SOURCES)

zip: $(DISTFILES)
	zip @PACKAGE@-@VERSION@.zip $(DISTFILES)

release: gif2png-$(VERSION).tar.gz gif2png.html
	shipper -u -m -t; rm -f *.1 *.html *.rpm MANIFEST SHIPPER.*
