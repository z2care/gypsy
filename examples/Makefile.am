noinst_PROGRAMS = 				\
	list-known-gps-devices			\
	simple-gps-dbus				\
	simple-gps-gypsy			\
	simple-gps-satellites

list_known_gps_devices_SOURCES = list-known-gps-devices.c
list_known_gps_devices_LDADD = $(GYPSY_LIBS) $(top_builddir)/gypsy/libgypsy.la
list_known_gps_devices_CFLAGS = $(GYPSY_CFLAGS) -I$(top_srcdir)

simple_gps_dbus_SOURCES = simple-gps-dbus.c
simple_gps_dbus_LDADD = $(GYPSY_LIBS)
simple_gps_dbus_CFLAGS = $(GYPSY_CFLAGS)

simple_gps_gypsy_SOURCES = simple-gps-gypsy.c
simple_gps_gypsy_LDADD = $(GYPSY_LIBS) $(top_builddir)/gypsy/libgypsy.la
simple_gps_gypsy_CFLAGS = $(GYPSY_CFLAGS) -I$(top_srcdir)

simple_gps_satellites_SOURCES = simple-gps-satellites.c
simple_gps_satellites_LDADD = $(GYPSY_LIBS) $(top_builddir)/gypsy/libgypsy.la
simple_gps_satellites_CFLAGS = $(GYPSY_CFLAGS) -I$(top_srcdir)

EXTRA_DIST = \
	simple-gps-python.py \
	gypsy-example-initscript
