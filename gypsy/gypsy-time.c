/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>
 *
 * Copyright (C) 2007 Iain Holmes
 * Copyright (C) 2007 Openedhand, Ltd
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
 * SECTION:gypsy-time
 * @short_description: Object for obtaining GPS time from gypsy-daemon
 *
 * #GypsyTime is used whenever the client program wishes to know about
 * GPS time changes. It can report the current GPS time, and has a signal
 * to notify listeners of changes.
 *
 * A #GypsyTime object is created with gypsy_time_new() using the 
 * D-Bus path of the GPS device. This path is returned from the 
 * gypsy_control_create() function. The client can then find out about the
 * GPS time with gypsy_time_get_time().
 *
 * As the GPS time information changes #GypsyTime will emit the 
 * time-changed signal. This signal contains the GPS time of the most recent
 * GPS update that it received.
 *
 * <informalexample>
 * <programlisting>
 * GypsyTime *gps_time;
 * GError *error = NULL;
 *
 * . . .
 *
 * /<!-- -->* path comes from the gypsy_control_create() function *<!-- -->/
 * gps_time = gypsy_time_new (path);
 * g_signal_connect (gps_time, "time-changed", G_CALLBACK (time_changed), NULL);
 * 
 * . . .
 *
 * static void position_changed (GypsyTime *gps_time, 
 * int timestamp,
 * gpointer userdata)
 * {
 * &nbsp;&nbsp;g_print ("timestamp: %d\n", timestamp);
 * }
 * </programlisting>
 * </informalexample>
 */

#include <glib-object.h>

#include <gypsy/gypsy-marshal.h>
#include <gypsy/gypsy-time.h>

#include "gypsy-client-bindings.h"

typedef struct _GypsyTimePrivate {
	DBusGProxy *proxy;
	char *object_path;
} GypsyTimePrivate;

enum {
	TIME_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_PATH
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GYPSY_TYPE_TIME, GypsyTimePrivate))

G_DEFINE_TYPE (GypsyTime, gypsy_time, G_TYPE_OBJECT);

static void time_changed (DBusGProxy *proxy,
			  int         timestamp,
			  GypsyTime  *gps_time);

static guint32 signals[LAST_SIGNAL] = {0, };

static void
finalize (GObject *object)
{
	GypsyTimePrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->object_path) {
		g_free (priv->object_path);
	}

	G_OBJECT_CLASS (gypsy_time_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GypsyTimePrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->proxy) {
		dbus_g_proxy_disconnect_signal (priv->proxy, "TimeChanged",
						G_CALLBACK (time_changed),
						object);
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (gypsy_time_parent_class)->dispose (object);
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	GypsyTimePrivate *priv;

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
	GypsyTimePrivate *priv;

	priv = GET_PRIVATE (object);
	switch (prop_id) {
	case PROP_PATH:
		break;

	default:
		break;
	}
}

static void
time_changed (DBusGProxy *proxy,
	      int         timestamp,
	      GypsyTime  *gps_time)
{
	g_signal_emit (gps_time, signals[TIME_CHANGED], 0, timestamp);
}

static GObject *
constructor (GType                  type,
	     guint                  n_construct_properties,
	     GObjectConstructParam *construct_properties)
{
	GypsyTime *gps_time;
	GypsyTimePrivate *priv;
	DBusGConnection *connection;
	GError *error;

	gps_time = GYPSY_TIME (G_OBJECT_CLASS (gypsy_time_parent_class)->constructor 
			       (type, n_construct_properties,
				construct_properties));

	priv = GET_PRIVATE (gps_time);

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		g_printerr ("Failed to open connection to bus: %s\n",
			    error->message);
		g_error_free (error);
		
		priv->proxy = NULL;
		return G_OBJECT (gps_time);
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection, 
						 GYPSY_TIME_DBUS_SERVICE,
						 priv->object_path,
						 GYPSY_TIME_DBUS_INTERFACE);

	dbus_g_proxy_add_signal (priv->proxy, "TimeChanged",
				 G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "TimeChanged",
				     G_CALLBACK (time_changed),
				     gps_time, NULL);

	return G_OBJECT (time);
}

static void
gypsy_time_class_init (GypsyTimeClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;
	o_class->set_property = set_property;
	o_class->get_property = get_property;

	g_type_class_add_private (klass, sizeof (GypsyTimePrivate));

	/**
	 * GypsyTime:object-path:
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
	 * GypsyTime::time-changed:
	 * @timestamp: The timestamp when this change occurred
	 *
	 * The ::time-changed signal is emitted when the GPS device sends
	 * a new timestamp in its GPS data.
	 */
	signals[TIME_CHANGED] = g_signal_new ("time-changed",
					      G_TYPE_FROM_CLASS (klass),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      G_STRUCT_OFFSET (GypsyTimeClass, time_changed),
					      NULL, NULL,
					      g_cclosure_marshal_VOID__INT,
					      G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
gypsy_time_init (GypsyTime *time)
{
}

/**
 * gypsy_time_new:
 * @object_path: Object path to the GPS device
 *
 * Creates a new #GypsyTime object that listens for time changes
 * from the GPS found at @object_path.
 *
 * Return value: A #GypsyTime object
 */
GypsyTime *
gypsy_time_new (const char *object_path)
{
	return g_object_new (GYPSY_TYPE_TIME, 
			     "object-path", object_path,
			     NULL);
}

/**
 * gypsy_time_get_time:
 * @gps_time: A #GypsyTime
 * @timestamp: Pointer to store the timestamp
 * @error: Pointer to store a #GError
 *
 * Obtains the current time, if known, from the GPS device.
 *
 * Return value: TRUE on success, FALSE on error.
 */
gboolean
gypsy_time_get_time (GypsyTime *gps_time,
		     int       *timestamp,
		     GError   **error)
{
	GypsyTimePrivate *priv;
	
	g_return_val_if_fail (GYPSY_IS_TIME (gps_time), FALSE);

	priv = GET_PRIVATE (gps_time);
	if (!org_freedesktop_Gypsy_Time_get_time (priv->proxy,
						  timestamp,
						  error)) {
		return FALSE;
	}

	return TRUE;
}
