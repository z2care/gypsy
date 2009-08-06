/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>
 * Copyright (C) 2007
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:gypsy-control
 * @short_description: Control object for gypsy-daemon
 *
 * #GypsyControl is the object that controls the gypsy-daemon process. 
 * It is a singleton object, meaning that there will only be one instance of it
 * per application. Once the object has been created (or referenced if it 
 * had already been created for the application) with 
 * gypsy_control_get_default(), it can be used to tell gypsy-daemon what GPS
 * device to connect to with gypsy_control_create(). This call returns the
 * D-Bus object path to the GPS object in gypsy-daemon. This object path is then used to 
 * create other objects, such as #GypsyPosition, or #GypsyCourse.
 *
 * As gypsy-daemon is able to connect to multiple GPS devices, the one
 * #GypsyControl can be used to create them, returning a different path for each
 * GPS device. Gypsy-daemon can connect to both serial port devices which have
 * an entry in <filename>/dev</filename> and Bluetooth devices natively without
 * the need to use rfcomm to set up a <filename>/dev</filename> entry. To do
 * this you pass either the device path or the Bluetooth address of the device
 * to gypsy_control_create().
 *
 * Once the program has finished with the GPS, the #GypsyControl object should
 * by unreferenced with g_object_unref().
 *
 * <informalexample>
 * <programlisting>
 * . . .
 *
 * #GypsyControl *control;
 * char *path_bt, *path_dev;
 * GError *error = NULL;
 *
 * . . .
 *
 * control = gypsy_control_get_default ();
 * / * Use a Bluetooth device * /
 * path_bt = gypsy_control_create (control, "aa:bb:cc:dd:ee", &error);
 * if (path_bt == NULL) {
 * &nbsp;&nbsp;g_warning ("There was an error creating aa:bb:cc:dd:ee - %s", error->message);
 * &nbsp;&nbsp;g_error_free (error);
 * &nbsp;&nbsp;error = NULL;
 * }
 *
 * / * Use a serial port device * /
 * path_dev = gypsy_control_create (control, "/dev/gps", &error);
 * if (path_dev == NULL) {
 * &nbsp;&nbsp;g_warning ("There was an error creating /dev/gps - %s", error->message);
 * &nbsp;&nbsp;g_error_free (error);
 * &nbsp;&nbsp;error = NULL;
 * }
 *
 * . . .
 *
 * / * Use the paths here to create listener objects * /
 *
 * . . . 
 *
 * g_free (path_bt);
 * g_free (path_dev);
 *
 * . . .
 *
 * / * The program has finished with Gypsy now, unref the object. * /
 * g_object_unref (G_OBJECT (control));
 * </programlisting>
 * </informalexample>
 */

#include <glib-object.h>

#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-marshal.h>

#include "gypsy-server-bindings.h"

typedef struct _GypsyControlPrivate {
	DBusGProxy *proxy;
	char *device_name;
} GypsyControlPrivate;

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GYPSY_TYPE_CONTROL, GypsyControlPrivate))

G_DEFINE_TYPE (GypsyControl, gypsy_control, G_TYPE_OBJECT);

GQuark 
gypsy_control_error_quark (void)
{
	static GQuark quark = 0;
	if (G_UNLIKELY (quark == 0)) {
		quark = g_quark_from_static_string ("gypsy-control-error-quark");
	}

	return quark;
}

static void
dispose (GObject *object)
{
	GypsyControlPrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->device_name) {
		GError *error = NULL;

		/* Shoutdown the server object when this control object
		   is unreffed */
		if (!org_freedesktop_Gypsy_Server_shutdown (priv->proxy,
							    priv->device_name,
							    &error)) {
			g_error_free (error);
		}

		g_free (priv->device_name);
		priv->device_name = NULL;
	}

	if (priv->proxy) {
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (gypsy_control_parent_class)->dispose (object);
}

static void
gypsy_control_class_init (GypsyControlClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->dispose = dispose;

	g_type_class_add_private (klass, sizeof (GypsyControlPrivate));
}

static void
gypsy_control_init (GypsyControl *control)
{
	GypsyControlPrivate *priv;
	DBusGConnection *connection;
	GError *error;

	priv = GET_PRIVATE (control);

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (!connection) {
		g_warning ("Unable to get connection to system bus\n%s",
			   error->message);
		g_error_free (error);
		return;
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection,
						 GYPSY_CONTROL_DBUS_SERVICE,
						 GYPSY_CONTROL_DBUS_PATH,
						 GYPSY_CONTROL_DBUS_INTERFACE);
	dbus_g_object_register_marshaller (gypsy_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE,
					   G_TYPE_NONE,
					   G_TYPE_INT,
					   G_TYPE_INT,
					   G_TYPE_DOUBLE,
					   G_TYPE_DOUBLE,
					   G_TYPE_DOUBLE,
					   G_TYPE_INVALID);
	dbus_g_object_register_marshaller (gypsy_marshal_VOID__INT_DOUBLE_DOUBLE_DOUBLE,
					   G_TYPE_NONE,
					   G_TYPE_INT,
					   G_TYPE_DOUBLE,
					   G_TYPE_DOUBLE,
					   G_TYPE_DOUBLE,
					   G_TYPE_INVALID);
}

/**
 * gypsy_control_get_default:
 * 
 * Retrieves the default #GypsyControl object.
 *
 * Return value: A singleton #GypsyControl.  Once the program has finished using
 * the #GypsyControl, it should be unreferenced with g_object_unref().
 */
GypsyControl *
gypsy_control_get_default (void)
{
	static GypsyControl *default_control = NULL;

	if (G_UNLIKELY (default_control == NULL)) {
		default_control = g_object_new (GYPSY_TYPE_CONTROL, NULL);
		g_object_add_weak_pointer (G_OBJECT (default_control),
					   (gpointer) &default_control);
		return default_control;
	}

	return g_object_ref (default_control);
}

/**
 * gypsy_control_create:
 * @control: The #GypsyControl device
 * @device_name: The path to the device file, or Bluetooth address
 * @error: A #GError to return errors in, or %NULL
 *
 * Creates a object on the server that refers to the GPS device at @device_name.
 * When this object is finalized, the remote object on the server will be
 * shutdown after which any calls to the object at the returned path
 * are not guaranteed to work.
 *
 * Return value: The path to the created object.
 */
char *
gypsy_control_create (GypsyControl *control,
		      const char   *device_name,
		      GError      **error)
{
	GypsyControlPrivate *priv;
	char *path;

	g_return_val_if_fail (GYPSY_IS_CONTROL (control), NULL);
	g_return_val_if_fail (device_name != NULL, NULL);

	priv = GET_PRIVATE (control);

	if (!org_freedesktop_Gypsy_Server_create (priv->proxy, device_name,
						  &path, error)) {
		return NULL;
	}

	priv->device_name = g_strdup (device_name);
	return path;
}
