SUBDIRS = interfaces src gypsy examples docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gypsy.pc

dbussysdir = $(DBUS_SYS_DIR)
dbussys_DATA = Gypsy.conf

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.freedesktop.Gypsy.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

DISTCLEANFILES = 	\
	gypsy.pc	\
	org.freedesktop.Gypsy.service

EXTRA_DIST = \
	gypsy.pc.in				\
	org.freedesktop.Gypsy.service.in	\
	LICENSE					\
	COPYING					\
	COPYING.lib				\
	Gypsy.conf

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
