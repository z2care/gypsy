libexec_PROGRAMS = gypsy-daemon

gypsy_daemon_CFLAGS =		\
	-I$(top_srcdir)		\
	-I$(srcdir)		\
	-I$(top_builddir)	\
	-DLOCALSTATEDIR=\"$(localstatedir)\"	\
	$(GYPSY_CFLAGS)

gypsy_daemon_LDADD =		\
	$(GYPSY_LIBS)		\
	-lm

NOINST_H_FILES =		\
	gypsy-client.h		\
	gypsy-marshal-internal.h	\
	gypsy-server.h		\
	nmea.h			\
	garmin.h		\
	nmea-parser.h		\
	nmea-gen.h

gypsy_daemon_SOURCES =		\
	gypsy-client.c		\
	gypsy-marshal-internal.c	\
	gypsy-server.c		\
	main.c			\
	nmea-parser.c		\
	nmea-gen.c		\
	$(NOINST_H_FILES)

BUILT_SOURCES =			\
	gypsy-marshal-internal.c	\
	gypsy-marshal-internal.h	\
	gypsy-client-glue.h	\
	gypsy-server-glue.h

EXTRA_DIST = gypsy-marshal.list \
	$(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES) \
	stamp-gypsy-client-glue.h	\
	stamp-gypsy-server-glue.h

gypsy-marshal-internal.h: gypsy-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gypsy_marshal > $@
gypsy-marshal-internal.c: gypsy-marshal.list gypsy-marshal-internal.h $(GLIB_GENMARSHAL)
	echo "#include \"gypsy-marshal-internal.h\"" > $@ \
	&& $(GLIB_GENMARSHAL) --prefix=gypsy_marshal $(srcdir)/gypsy-marshal.list --body >> $@

%-glue.h: stamp-%-glue.h
	@true
stamp-gypsy-server-glue.h: ../interfaces/gypsy-server.xml
	$(DBUS_BINDING_TOOL) --prefix=gypsy_server --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F)	\
	&& echo timestamp > $(@F)

stamp-gypsy-client-glue.h: ../interfaces/gypsy-client.xml
	$(DBUS_BINDING_TOOL) --prefix=gypsy_client --mode=glib-server $< > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(@F:stamp-%=%) || cp xgen-$(@F) $(@F:stamp-%=%)) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)
