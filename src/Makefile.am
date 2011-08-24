libexec_PROGRAMS = gypsy-daemon

gypsy_daemon_CFLAGS =		\
	-I$(top_srcdir)		\
	-I$(srcdir)		\
	-I$(top_builddir)	\
	-DLOCALSTATEDIR=\"$(localstatedir)\"	\
	$(GYPSY_CFLAGS)		\
	$(GPSBT_CFLAGS)

gypsy_daemon_LDADD =		\
	$(GYPSY_LIBS)		\
	$(GPSBT_LIBS)		\
	-lm

NOINST_H_FILES =		\
	gypsy-client.h		\
	gypsy-debug.h		\
	gypsy-discovery.h	\
	gypsy-garmin-parser.h	\
	gypsy-marshal-internal.h	\
	gypsy-nmea-parser.h	\
	gypsy-parser.h		\
	gypsy-server.h		\
	nmea.h			\
	garmin.h		\
	nmea-parser.h

gypsy_daemon_SOURCES =		\
	gypsy-client.c		\
	gypsy-discovery.c	\
	gypsy-garmin-parser.c	\
	gypsy-marshal-internal.c	\
	gypsy-nmea-parser.c	\
	gypsy-parser.c		\
	gypsy-server.c		\
	main.c			\
	nmea-parser.c		\
	$(NOINST_H_FILES)

BUILT_SOURCES =			\
	gypsy-marshal-internal.c	\
	gypsy-marshal-internal.h	\
	gypsy-client-glue.h	\
	gypsy-discovery-glue.h	\
	gypsy-server-glue.h

EXTRA_DIST = gypsy-marshal.list \
	$(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES)

gypsy-marshal-internal.h: gypsy-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --header --prefix=gypsy_marshal > $@
gypsy-marshal-internal.c: gypsy-marshal.list gypsy-marshal-internal.h $(GLIB_GENMARSHAL)
	$(AM_V_GEN)echo "#include \"gypsy-marshal-internal.h\"" > $@ \
	&& $(GLIB_GENMARSHAL) --prefix=gypsy_marshal $(srcdir)/gypsy-marshal.list --body >> $@

gypsy-server-glue.h: ../interfaces/gypsy-server.xml
	$(AM_V_GEN)$(DBUS_BINDING_TOOL) --prefix=gypsy_server --mode=glib-server $< > $@

gypsy-client-glue.h: ../interfaces/gypsy-client.xml
	$(AM_V_GEN)$(DBUS_BINDING_TOOL) --prefix=gypsy_client --mode=glib-server $< > $@

gypsy-discovery-glue.h: ../interfaces/gypsy-discovery.xml
	$(AM_V_GEN)$(DBUS_BINDING_TOOL) --prefix=gypsy_discovery --mode=glib-server $< > $@
