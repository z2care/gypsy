%.xml: %-full.xml
	$(AM_V_GEN)$(XSLT) -o $@ $(top_srcdir)/docs/tools/spec-strip-docs.xsl $<

noinst_DATA = gypsy-server.xml	\
	      gypsy-client.xml	\
	      gypsy-discovery.xml

BUILT_SOURCES = $(noinst_DATA)
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = gypsy-server-full.xml	\
	     gypsy-client-full.xml	\
	     gypsy-discovery-full.xml
