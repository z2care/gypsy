%.xml: %-full.xml
	$(XSLT) -o $@ $(top_srcdir)/docs/tools/spec-strip-docs.xsl $<

noinst_DATA = gypsy-server.xml gypsy-client.xml

BUILT_SOURCES = $(noinst_DATA)
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = gypsy-server-full.xml gypsy-client-full.xml
