SUBDIRS = interfaces src gypsy examples docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gypsy.pc

dbussysdir = $(DBUS_SYS_DIR)
dbussys_DATA = Gypsy.conf

servicedir = $(DBUS_SERVICES_DIR)
service_DATA = org.freedesktop.Gypsy.service

DISTCLEANFILES = 	\
	gypsy.pc	\
	org.freedesktop.Gypsy.service

EXTRA_DIST = \
	gypsy.pc.in				\
	LICENSE					\
	COPYING					\
	COPYING.lib				\
	Gypsy.conf

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
