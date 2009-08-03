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
 * SECTION:gypsy-position
 * @short_description: Object for obtaining positions from gypsy-daemon
 *
 * #GypsyPosition is used whenever the client program wishes to know about
 * GPS position changes. It can report the current position, and has a signal
 * to notify listeners of changes.
 *
 * A #GypsyPosition object is created using gypsy_position_new() using the 
 * D-Bus path of the GPS device. This path is returned from the 
 * gypsy_control_create() function. The client can then find out about the
 * position with gypsy_position_get_position().
 *
 * As the position information changes #GypsyPosition will emit the 
 * position-changed signal. This signal contains the fix position if 
 * gypsy-daemon knows it. It has a fields paramater which is a bitmask of
 * #GypsyPositionFields which indicates which of the latitude, longitude or
 * altitude contains valid information. The timestamp will always be valid
 * if it is greater than 0.
 *
 * <informalexample>
 * <programlisting>
 * GypsyPosition *position;
 * GError *error = NULL;
 *
 * . . .
 *
 * / * path comes from the gypsy_control_create() function * /
 * position = gypsy_position_new (path);
 * g_signal_connect (position, "position-changed", G_CALLBACK (position_changed), NULL);
 * 
 * . . .
 *
 * static void position_changed (GypsyPosition *position, 
 * GypsyPositionFields fields,
 * int timestamp,
 * double latitude,
 * double longitude,
 * double altitude,
 * gpointer userdata)
 * {
 * &nbsp;&nbsp;g_print ("latitude, longitude (%f, %f)\n", (fields & GYPSY_POSITION_FIELDS_LATITUDE) ? latitude : -1.0, (fields & GYPSY_POSITION_FIELDS_LONGITUDE) ? longitude : -1.0);
 * }
 * </programlisting>
 * </informalexample>
 */

#include <glib-object.h>

#include <gypsy/gypsy-marshal.h>
#include <gypsy/gypsy-position.h>

#include "gypsy-client-bindings.h"

typedef struct _GypsyPositionPrivate {
	DBusGProxy *proxy;
	char *object_path;
} GypsyPositionPrivate;

enum {
	POSITION_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_PATH
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GYPSY_TYPE_POSITION, GypsyPositionPrivate))

G_DEFINE_TYPE (GypsyPosition, gypsy_position, G_TYPE_OBJECT);

static void position_changed (DBusGProxy    *proxy,
			      int            fields,
			      int            timestamp,
			      double         latitude,
			      double         longitude,
			      double         altitude,
			      GypsyPosition *position);

static guint32 signals[LAST_SIGNAL] = {0, };
static void
finalize (GObject *object)
{
	GypsyPositionPrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->object_path) {
		g_free (priv->object_path);
	}

	G_OBJECT_CLASS (gypsy_position_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GypsyPositionPrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->proxy) {
		dbus_g_proxy_disconnect_signal (priv->proxy, "PositionChanged",
						G_CALLBACK (position_changed),
						object);
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (gypsy_position_parent_class)->dispose (object);
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	GypsyPositionPrivate *priv;

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
	GypsyPositionPrivate *priv;

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
position_changed (DBusGProxy    *proxy,
		  int            fields,
		  int            timestamp,
		  double         latitude,
		  double         longitude,
		  double         altitude,
		  GypsyPosition *position)
{
	g_signal_emit (position, signals[POSITION_CHANGED], 0,
		       fields, timestamp, latitude, longitude, altitude);
}

static void
get_position_cb (DBusGProxy *proxy,
		 int         fields, 
		 int         timestamp,
		 double      latitude,
		 double      longitude,
		 double      altitude,
		 GError     *error, 
		 gpointer    userdata)
{
	GypsyPosition *position = userdata;

	if (error) {
		g_warning ("Cannot get position: %s", error->message);
		g_error_free (error);
		return;
	}

	position_changed (proxy, fields, timestamp, 
			  latitude, longitude, altitude, position);
}

static GObject *
constructor (GType                  type,
	     guint                  n_construct_properties,
	     GObjectConstructParam *construct_properties)
{
	GypsyPosition *position;
	GypsyPositionPrivate *priv;
	DBusGConnection *connection;
	GError *error;

	position = GYPSY_POSITION (G_OBJECT_CLASS (gypsy_position_parent_class)->constructor 
				   (type, n_construct_properties,
				    construct_properties));

	priv = GET_PRIVATE (position);

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		g_printerr ("Failed to open connection to bus: %s\n",
			    error->message);
		g_error_free (error);
		
		priv->proxy = NULL;
		return G_OBJECT (position);
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection, 
						 GYPSY_POSITION_DBUS_SERVICE,
						 priv->object_path,
						 GYPSY_POSITION_DBUS_INTERFACE);

	dbus_g_proxy_add_signal (priv->proxy, "PositionChanged",
				 G_TYPE_INT, G_TYPE_INT, G_TYPE_DOUBLE,
				 G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "PositionChanged",
				     G_CALLBACK (position_changed),
				     position, NULL);

	org_freedesktop_Gypsy_Position_get_position_async
		(priv->proxy, get_position_cb, position);
	
	return G_OBJECT (position);
}

static void
gypsy_position_class_init (GypsyPositionClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;
	o_class->set_property = set_property;
	o_class->get_property = get_property;

	g_type_class_add_private (klass, sizeof (GypsyPositionPrivate));

	/**
	 * GypsyPosition:object-path:
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
	 * GypsyPosition::position-changed:
	 * @fields: A bitmask of #GypsyPositionFields indicating which of the following fields are valid
	 * @timestamp: The timestamp when this change occurred
	 * @latitude: The new latitude
	 * @longitude: The new longitude
	 * @altitude: The new altitude
	 *
	 * The ::position-changed signal is emitted when the GPS device
	 * indicates that one or more of the position fields has changed.
	 * The fields which have changed will be indicated in the @fields
	 * bitmask.
	 */
	signals[POSITION_CHANGED] = g_signal_new ("position-changed",
						  G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (GypsyPositionClass, position_changed),
						  NULL, NULL,
						  gypsy_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE,
						  G_TYPE_NONE, 5,
						  G_TYPE_INT, G_TYPE_INT,
						  G_TYPE_DOUBLE, G_TYPE_DOUBLE,
						  G_TYPE_DOUBLE);
}

static void
gypsy_position_init (GypsyPosition *position)
{
}

/**
 * gypsy_position_new:
 * @object_path: Object path to the GPS device
 *
 * Creates a new #GypsyPosition object that listens for position changes
 * from the GPS found at @object_path.
 *
 * Return value: A #GypsyPosition object
 */
GypsyPosition *
gypsy_position_new (const char *object_path)
{
	return g_object_new (GYPSY_TYPE_POSITION, 
			     "object-path", object_path,
			     NULL);
}

/**
 * gypsy_position_get_position:
 * @position: A #GypsyPosition
 * @timestamp: Pointer to store the timestamp
 * @latitude: Pointer to store the latitude
 * @longitude: Pointer to store the longitude
 * @altitude: Pointer to store the altitude
 * @error: Pointer to store a #GError
 *
 * Obtains the current position, if known, from the GPS device.
 * @timestamp, @latitude, @longitude and @altitude can be #NULL if the result
 * is not required.
 *
 * Return value: Bitmask of #GypsyPositionFields indicating what field was set
 */
GypsyPositionFields
gypsy_position_get_position (GypsyPosition *position,
			     int           *timestamp,
			     double        *latitude,
			     double        *longitude,
			     double        *altitude,
			     GError       **error)
{
	GypsyPositionPrivate *priv;
	double la, lo, al;
	int ts, fields;
	
	g_return_val_if_fail (GYPSY_IS_POSITION (position), GYPSY_POSITION_FIELDS_NONE);

	priv = GET_PRIVATE (position);
	if (!org_freedesktop_Gypsy_Position_get_position (priv->proxy,
							  &fields, &ts,
							  &la, &lo, &al,
							  error)) {
		return GYPSY_POSITION_FIELDS_NONE;
	}

	if (timestamp != NULL) {
		*timestamp = ts;
	}

	if (latitude != NULL && (fields & GYPSY_POSITION_FIELDS_LATITUDE)) {
		*latitude = la;
	}

	if (longitude != NULL && (fields & GYPSY_POSITION_FIELDS_LONGITUDE)) {
		*longitude = lo;
	}

	if (altitude != NULL && (fields & GYPSY_POSITION_FIELDS_ALTITUDE)) {
		*altitude = al;
	}

	return fields;
}
