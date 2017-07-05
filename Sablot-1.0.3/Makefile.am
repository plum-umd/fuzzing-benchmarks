SUBDIRS = src doc utils
EXTRA_DIST = makefile.msvc makefile_include.msvc makefile_rules.msvc sabcmd.msvc sablot_config.msvc INSTALL_WIN RELEASE README_JS

dist-hook:
	if test ! -d $(distdir)/autocfg; then mkdir $(distdir)/autocfg; fi
	@cp autocfg/config.h.in $(distdir)/autocfg

