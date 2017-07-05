INCLUDES = -I$(top_srcdir)/src/engine

bin_PROGRAMS = sabcmd 
bin_SCRIPTS = sablot-config

LIBS = @ICONV_LIB@ @EXPAT_LIBS@ @OTHER_LIBS@

sabcmd_SOURCES = sabcmd.cpp

#if AM_HAS_EXPAT
#sabcmd_LDADD=../engine/libsablot.la \
#	$(top_builddir)/$(EXPATH)/libxmlparse.la\
#	$(top_builddir)/$(EXPATH)/libxmltok.la
#else

sabcmd_LDADD = ../engine/libsablot.la

#endif
