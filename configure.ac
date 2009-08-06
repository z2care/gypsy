AC_PREREQ(2.59)
AC_INIT(gypsy, 0.6, http://gypsy.freedesktop.org)

AC_CONFIG_SRCDIR(src/main.c)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([1.9 foreign])
GYPSY_VERSION=1

AC_PROG_CC
AC_ISC_POSIX

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_GCC_TRADITIONAL
AM_PROG_LIBTOOL

GYPSY_PC_MODULES='glib-2.0 dbus-glib-1 >= 0.60'

AC_ARG_ENABLE(bluetooth, AC_HELP_STRING([--disable-bluetooth],[Enable support for Bluetooth GPS devices]),, enable_bluetooth=yes)

if test x$enable_bluetooth = xyes; then
	AC_DEFINE(HAVE_BLUEZ,1,[Define if we have Bluez])
	GYPSY_PC_MODULES="$GYPSY_PC_MODULES bluez"
fi

PKG_CHECK_MODULES(GYPSY, $GYPSY_PC_MODULES)
AC_SUBST(GYPSY_LIBS)
AC_SUBST(GYPSY_CFLAGS)

GTK_DOC_CHECK([1.6])

AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

DBUS_SYS_DIR="${sysconfdir}/dbus-1/system.d"
AC_SUBST(DBUS_SYS_DIR)
AC_DEFINE_UNQUOTED(DBUS_SYS_DIR, "$DBUS_SYS_DIR", [Where the system dir for D-Bus is])

DBUS_SERVICES_DIR="${datadir}/dbus-1/system-services"
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for D-Bus is])

CFLAGS="$CFLAGS -g -Wall -Werror -Wno-format"

# TODO: add --with-xsltproc argument to override PATH
AC_CHECK_PROGS(XSLT, xsltproc)

AC_CONFIG_FILES([
Makefile
interfaces/Makefile
src/Makefile
gypsy/Makefile
examples/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
docs/tools/Makefile
gypsy.pc
])

AC_OUTPUT
