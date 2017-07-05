EXTRA_DIST=jsdom-ref.xml sablot.xml sxp.xml

HTML_DIR=@HTML_DIR@
DEST_DIR = apidocs

APIDOCDIR = $(top_srcdir)/utils/apidoc
APIDOC = @PERL_PROG@ $(APIDOCDIR)/parse_apidoc.pl
BUILD_APIDOCS = @BUILD_APIDOCS@
SOURCES = jsdom-ref.xml sablot.xml sxp.xml

CHECKFILES = $(srcdir)/jsdom-ref/complete.html $(srcdir)/sablot/complete.html \
             $(srcdir)/sxp/complete.html

APIDIRS = jsdom-ref sablot sxp

all: $(CHECKFILES)

clean:
	@for foo in $(SOURCES); do \
	  dir=`basename $$foo .xml`; \
	  rm -rf $$dir; \
	done;

%.html: $(SOURCES)
	-@if [ "x$(BUILD_APIDOCS)" = "xyes" ]; \
	then adir=`dirname $@`; \
	if [ ! -d $$adir ]; then $(mkinstalldirs) $$adir; fi; \
	if [ ! -d $$adir/$(DEST_DIR) ]; \
	 then $(mkinstalldirs) $$adir/$(DEST_DIR); fi; \
	src=`basename $$adir.xml`; echo $$src; \
	$(APIDOC) $$src $$adir/$(DEST_DIR); \
	@INSTALL@ -m 0644 $(APIDOCDIR)/*.css $$adir/$(DEST_DIR) ; \
        @INSTALL@ -m 0644 $(APIDOCDIR)/*.html $$adir; \
	fi;

install: 
	-@if [ "x$(BUILD_APIDOCS)" = "xyes" ]; \
	   then for apidir in $(APIDIRS); do \
	    if [ ! -d $(DESTDIR)$(HTML_DIR)/$$apidir/$(DEST_DIR) ]; \
            then  $(mkinstalldirs) $(DESTDIR)$(HTML_DIR)/$$apidir/$(DEST_DIR); fi; \
            @INSTALL@ -m 0644 $(APIDOCDIR)/*.css $(DESTDIR)$(HTML_DIR)/$$apidir/$(DEST_DIR) ; \
            @INSTALL@ -m 0644 $(APIDOCDIR)/*.html $(DESTDIR)$(HTML_DIR)/$$apidir; \
            @INSTALL@ -m 0644 $$apidir/$(DEST_DIR)/*.html $(DESTDIR)$(HTML_DIR)/$$apidir/$(DEST_DIR); \
	   done; \
	 fi;

distclean: clean

.PHONY: all clean


