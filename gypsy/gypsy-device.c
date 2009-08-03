/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>
 * Copyright (C) 2007 Iain Holmes,
 * Copyright (C) 2007 Openedhand Ltd
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
 * SECTION:gypsy-device
 * @short_description: Object for obtaining device information
 *
 * #GypsyDevice is used whenever the client program wishes to know about
 * changes in the device's status. It has signals for connection status and
 * fix status. It can also be used to tell gypsy-daemon to start or stop parsing
 * sentences from the GPS device.
 *
 * A #GypsyDevice object is created using gypsy_device_new() using the D-Bus path of
 * the GPS device. This path is returned from the gypsy_control_create() 
 * function. The client can start the GPS data stream with gypsy_device_start(),
 * stop it with gypsy_device_stop(), or find out about the status with
 * gypsy_device_get_fix_status() and gypsy_device_get_connection_status().
 *
 * As the fix status and connection status change, #GypsyDevice will emit the
 * #GypsyDevice::fix-status-changed and #GypsyDevice::connection-changed signals
 * respectively.
 *
 * <informalexample>
 * <programlisting>
 * GypsyDevice *device;
 * GError *error = NULL;
 *
 * . . .
 *
 * /<!-- -->* path comes from the #gypsy_control_create() function *<!-- -->/
 * device = #gypsy_device_new (path);
 * g_signal_connect (device, "connection-changed", G_CALLBACK (connection_changed), NULL);
 * #gypsy_device_start (device, &error);
 * if (error != NULL) {
 * &nbsp;&nbsp;g_warning ("Error starting GPS: %s", error->message);
 * &nbsp;&nbsp;g_error_free (error);
 * }
 * 
 * . . .
 *
 * static void connection_changed (GypsyDevice *device, gboolean connected, gpointer data)
 * {
 * &nbsp;&nbsp;g_print ("Connection status: %s\n", connected ? "Connected" : "Disconnected");
 * }
 * </programlisting>
 * </informalexample>
 */

#include <glib-object.h>

#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-marshal.h>

#include "gypsy-client-bindings.h"

typedef struct _GypsyDevicePrivate {
	char *object_path;
	DBusGProxy *proxy;
} GypsyDevicePrivate;

enum {
	POSITION_CHANGED,
	COURSE_CHANGED,
	CONNECTION_CHANGED,
	FIX_STATUS_CHANGED,
	SATELLITES_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_PATH
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GYPSY_TYPE_DEVICE, GypsyDevicePrivate))

static void connection_changed_cb (DBusGProxy  *proxy,
				   gboolean     connected,
				   GypsyDevice *device);
static void fix_status_changed_cb (DBusGProxy  *proxy,
				   int          fix_status,
				   GypsyDevice *device);

G_DEFINE_TYPE (GypsyDevice, gypsy_device, G_TYPE_OBJECT);

static guint32 signals[LAST_SIGNAL] = {0, };
static void
finalize (GObject *object)
{
	GypsyDevicePrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->object_path) {
		g_free (priv->object_path);
	}

	G_OBJECT_CLASS (gypsy_device_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GypsyDevicePrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->proxy) {
		dbus_g_proxy_disconnect_signal (priv->proxy, "ConnectionStatusChanged",
						G_CALLBACK (connection_changed_cb),
						object);
		dbus_g_proxy_disconnect_signal (priv->proxy, "FixStatusChanged",
						G_CALLBACK (fix_status_changed_cb),
						object);

		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (gypsy_device_parent_class)->dispose (object);
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	GypsyDevicePrivate *priv;

	priv = GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_PATH:
		priv->object_path = g_value_dup_string (value);
		break;

	default:
		break;
	}
}

static void
get_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
	GypsyDevicePrivate *priv;

	priv = GET_PRIVATE (object);
	switch (prop_id) {
	case PROP_PATH:
		g_value_set_string (value, priv->object_path);
		break;

	default:
		break;
	}
}

static void
connection_changed_cb (DBusGProxy  *proxy,
		       gboolean     connected,
		       GypsyDevice *device)
{
	g_signal_emit (device, signals[CONNECTION_CHANGED], 0, connected);
}

static void
fix_status_changed_cb (DBusGProxy  *proxy,
		       int          fix_status,
		       GypsyDevice *device)
{
	g_signal_emit (device, signals[FIX_STATUS_CHANGED], 0, fix_status);
}

static GObject *
constructor (GType                  type,
	     guint                  n_construct_properties,
	     GObjectConstructParam *construct_properties)
{
	GypsyDevice *device;
	GypsyDevicePrivate *priv;
	DBusGConnection *connection;
	GError *error;

	device = GYPSY_DEVICE (G_OBJECT_CLASS (gypsy_device_parent_class)->constructor 
			       (type, n_construct_properties,
				construct_properties));

	priv = GET_PRIVATE (device);

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		g_printerr ("Failed to open connection to bus: %s\n",
			    error->message);
		g_error_free (error);
		
		priv->proxy = NULL;
		return G_OBJECT (device);
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection, 
						 GYPSY_DEVICE_DBUS_SERVICE,
						 priv->object_path,
						 GYPSY_DEVICE_DBUS_INTERFACE);

	dbus_g_proxy_add_signal (priv->proxy, "ConnectionStatusChanged",
				 G_TYPE_BOOLEAN, G_TYPE_INVALID);
	dbus_g_proxy_add_signal (priv->proxy, "FixStatusChanged",
				 G_TYPE_INT, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal (priv->proxy, "ConnectionStatusChanged",
				     G_CALLBACK (connection_changed_cb),
				     device, NULL);
	dbus_g_proxy_connect_signal (priv->proxy, "FixStatusChanged",
				     G_CALLBACK (fix_status_changed_cb),
				     device, NULL);
				     
	return G_OBJECT (device);
}

static void
gypsy_device_class_init (GypsyDeviceClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;
	o_class->set_property = set_property;
	o_class->get_property = get_property;

	g_type_class_add_private (klass, sizeof (GypsyDevicePrivate));

	/**
	 * GypsyDevice:object-path:
	 *
	 * The path of the Gypsy GPS object
	 */
	g_object_class_install_property 
		(o_class, PROP_PATH,
		 g_param_spec_string ("object-path",
				      "Object path",
				      "The DBus path to the object", 
				      "",
				      G_PARAM_WRITABLE |
				      G_PARAM_CONSTRUCT_ONLY |
				      G_PARAM_STATIC_NICK |
				      G_PARAM_STATIC_BLURB |
				      G_PARAM_STATIC_NAME));

	/**
	 * GypsyDevice::connection-changed:
	 * @connected: Whether or not the device is connected
	 *
	 * The ::connection-changed signal is emitted whenever the device
	 * connection changes.
	 */
	signals[CONNECTION_CHANGED] = g_signal_new ("connection-changed",
						    G_TYPE_FROM_CLASS (klass),
						    G_SIGNAL_RUN_FIRST |
						    G_SIGNAL_NO_RECURSE,
						    G_STRUCT_OFFSET (GypsyDeviceClass, connection_changed),
						    NULL, NULL,
						    g_cclosure_marshal_VOID__BOOLEAN,
						    G_TYPE_NONE,
						    1, G_TYPE_BOOLEAN);

	/**
	 * GypsyDevice::fix-status-changed:
	 * @fix_status: The new fix status
	 *
	 * The ::fix-status-changed signal is emitted whenever the GPS device
	 * reports that its fix status has changed. @fix_status is a
	 * #GypsyDeviceFixStatus
	 */
	signals[FIX_STATUS_CHANGED] = g_signal_new ("fix-status-changed",
						    G_TYPE_FROM_CLASS (klass),
						    G_SIGNAL_RUN_FIRST |
						    G_SIGNAL_NO_RECURSE,
						    G_STRUCT_OFFSET (GypsyDeviceClass,
								     fix_status_changed),
						    NULL, NULL,
						    g_cclosure_marshal_VOID__INT,
						    G_TYPE_NONE, 
						    1, G_TYPE_INT);
	
}

static void
gypsy_device_init (GypsyDevice *device)
{
}

/**
 * gypsy_device_new:
 * @object_path: Object path to the device
 *
 * Creates a new #GypsyDevice that points to @object_path
 *
 * Return value: A pointer to a #GypsyDevice
 */
GypsyDevice *
gypsy_device_new (const char *object_path)
{
	return g_object_new (GYPSY_TYPE_DEVICE,
			     "object-path", object_path,
			     NULL);
}

/**
 * gypsy_device_start:
 * @device: A #GypsyDevice
 * @error: A pointer to a #GError to return the error in
 *
 * Starts the connection to the physical device pointed to by @device, and
 * listens for incoming messages.
 *
 * Return value: #TRUE on success, #FALSE otherwise.
 */
gboolean
gypsy_device_start (GypsyDevice *device,
		    GError     **error)
{
	GypsyDevicePrivate *priv;

	g_return_val_if_fail (GYPSY_IS_DEVICE (device), FALSE);

	priv = GET_PRIVATE (device);

	if (!org_freedesktop_Gypsy_Device_start (priv->proxy, error)) {
		return FALSE;
	}

	return TRUE;
}

/**
 * gypsy_device_stop:
 * @device: A #GypsyDevice
 * @error: A pointer to a #GError to return the error in
 *
 * Stops the physical device pointed to by @device.
 *
 * Return value: #TRUE on success, #FALSE otherwise.
 */
gboolean
gypsy_device_stop (GypsyDevice *device,
		   GError     **error)
{
	GypsyDevicePrivate *priv;

	g_return_val_if_fail (GYPSY_IS_DEVICE (device), FALSE);

	priv = GET_PRIVATE (device);

	if (!org_freedesktop_Gypsy_Device_stop (priv->proxy, error)) {
		return FALSE;
	}

	return TRUE;
}

/**
 * gypsy_device_get_fix_status:
 * @device: A #GypsyDevice
 * @error: A pointer to a #GError to return a error in.
 *
 * Obtains the current fix status of @device.
 *
 * Return value: A #GypsyDeviceFixStatus
 */
GypsyDeviceFixStatus
gypsy_device_get_fix_status (GypsyDevice *device,
			     GError      **error)
{
	GypsyDevicePrivate *priv;
	int status;

	g_return_val_if_fail (GYPSY_IS_DEVICE (device), GYPSY_DEVICE_FIX_STATUS_INVALID);
	
	priv = GET_PRIVATE (device);
	if (!org_freedesktop_Gypsy_Device_get_fix_status (priv->proxy, &status,
							  error)) {
		return GYPSY_DEVICE_FIX_STATUS_INVALID;
	}

	return (GypsyDeviceFixStatus) status;
}

/**
 * gypsy_device_get_connection_status:
 * @device: A #GypsyDevice
 * @error: A pointer to a #GError to return an error in.
 *
 * Obtains the connection status of @device.
 *
 * Return value: #TRUE if the device is connected, #FALSE otherwise.
 */
gboolean
gypsy_device_get_connection_status (GypsyDevice *device,
				    GError     **error)
{
	GypsyDevicePrivate *priv;
	gboolean status;

	g_return_val_if_fail (GYPSY_IS_DEVICE (device), FALSE);
	
	priv = GET_PRIVATE (device);
	if (!org_freedesktop_Gypsy_Device_get_connection_status (priv->proxy,
								 &status,
								 error)) {
		return FALSE;
	}

	return status;
}
