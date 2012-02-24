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
 * SECTION:gypsy-satellite
 * @short_description: Object for obtaining satellite information
 *
 * #GypsySatellite is used whenever the client program wishes to know about
 * changes in the satellite details. The satellite details contain the satellite
 * ID number (the PRN), the elevation, the azimuth, the signal-to-noise ratio
 * (SNR) and whether or not the satellite was used to calculate the fix.
 * 
 * A #GypsySatellite object is created using gypsy_satellite_new() using the 
 * D-Bus path of the GPS device. This path is returned from the 
 * gypsy_control_create() function. The client can then find out about the
 * visible satellites with gypsy_satellite_get_satellites() which returns a
 * GPtrArray containing the #GypsySatelliteDetails for each visible satellite.
 * Once the client is finished with this GPtrArray 
 * gypsy_satellite_free_satellite_array() should be used to free the data.
 *
 * As the satellite information changes #GypsySatellite will emit the 
 * satellite-changed signal. This signal contains the satellite details in
 * a GPtrArray. In this case the satellite array does not need to be freed
 * with gypsy_satellite_free_satellite_array().
 *
 * Although gypsy-daemon only emits signals whenever the associated data has 
 * changed, satellite data is constantly changing, so so the satellite-changed
 * signal will be emitted at a rate of once every second.
 *
 * <informalexample>
 * <programlisting>
 * GypsySatellite *satellite;
 * GError *error = NULL;
 *
 * . . .
 *
 * / * path comes from the gypsy_control_create() function * /
 * satellite = gypsy_satellite_new (path);
 * g_signal_connect (satellite, "satellite-changed", G_CALLBACK (satellite_changed), NULL);
 * 
 * . . .
 *
 * static void satellite_changed (GypsySatellite *satellite, 
 * GPtrArray *satellites,
 * gpointer userdata)
 * {
 * &nbsp;&nbsp;int i;
 *
 * &nbsp;&nbsp;for (i = 0; i < satellites->len; i++) {
 * &nbsp;&nbsp;&nbsp;&nbsp;GypsySatelliteDetails *details = satellites->pdata[i];
 * &nbsp;&nbsp;&nbsp;&nbsp;g_print ("Satellite %d: %s", details->satellite_id, details->in_use ? "In use" : "Not in use");
 * &nbsp;&nbsp;}
 * }
 * </programlisting>
 * </informalexample>
 */

#include <glib-object.h>

#include <gypsy/gypsy-satellite.h>
#include <gypsy/gypsy-marshal.h>

#include "gypsy-client-bindings.h"

typedef struct _GypsySatellitePrivate {
	DBusGProxy *proxy;
	char *object_path;
} GypsySatellitePrivate;

enum {
	SATELLITES_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_PATH
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GYPSY_TYPE_SATELLITE, GypsySatellitePrivate))

#define GYPSY_SATELLITE_SATELLITES_CHANGED_TYPE (dbus_g_type_get_struct ("GValueArray", G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID))

G_DEFINE_TYPE (GypsySatellite, gypsy_satellite, G_TYPE_OBJECT);

static void satellites_changed (DBusGProxy     *proxy,
				GPtrArray      *sats,
				GypsySatellite *satellite);

static guint32 signals[LAST_SIGNAL] = {0, };
static void
finalize (GObject *object)
{
	GypsySatellitePrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->object_path) {
		g_free (priv->object_path);
	}

	G_OBJECT_CLASS (gypsy_satellite_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GypsySatellitePrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->proxy) {
		dbus_g_proxy_disconnect_signal (priv->proxy, "SatellitesChanged",
						G_CALLBACK (satellites_changed),
						object);
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (gypsy_satellite_parent_class)->dispose (object);
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	GypsySatellitePrivate *priv;

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
	GypsySatellitePrivate *priv;

	priv = GET_PRIVATE (object);
	switch (prop_id) {
	case PROP_PATH:
		g_value_set_string (value, priv->object_path);
		break;

	default:
		break;
	}
}

static GPtrArray *
make_satellite_array (GPtrArray *sats)
{
	GPtrArray *satellites;
	int i;
	
	satellites = g_ptr_array_sized_new (sats->len);
	
	for (i = 0; i < sats->len; i++) {
		GypsySatelliteDetails *details;
		GValueArray *vals = sats->pdata[i];

		details = g_slice_new (GypsySatelliteDetails);

		details->satellite_id = g_value_get_uint (g_value_array_get_nth (vals, 0));
		details->in_use = g_value_get_boolean (g_value_array_get_nth (vals, 1));
		details->elevation = g_value_get_uint (g_value_array_get_nth (vals, 2));
		details->azimuth = g_value_get_uint (g_value_array_get_nth (vals, 3));
		details->snr = g_value_get_uint (g_value_array_get_nth (vals, 4));

		g_ptr_array_add (satellites, details);
	}

	return satellites;
}

static void
satellites_changed (DBusGProxy     *proxy,
		    GPtrArray      *sats,
		    GypsySatellite *satellite)
{
	GPtrArray *satellites;

	satellites = make_satellite_array (sats);

	g_signal_emit (satellite, signals[SATELLITES_CHANGED], 0, satellites);
	gypsy_satellite_free_satellite_array (satellites);
}

static GObject *
constructor (GType                  type,
	     guint                  n_construct_properties,
	     GObjectConstructParam *construct_properties)
{
	GypsySatellite *satellite;
	GypsySatellitePrivate *priv;
	DBusGConnection *connection;
	GError *error;

	satellite = GYPSY_SATELLITE (G_OBJECT_CLASS (gypsy_satellite_parent_class)->constructor 
				     (type, n_construct_properties,
				      construct_properties));

	priv = GET_PRIVATE (satellite);

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		g_printerr ("Failed to open connection to bus: %s\n",
			    error->message);
		g_error_free (error);
		
		priv->proxy = NULL;
		return G_OBJECT (satellite);
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection, 
						 GYPSY_SATELLITE_DBUS_SERVICE,
						 priv->object_path,
						 GYPSY_SATELLITE_DBUS_INTERFACE);
	dbus_g_proxy_add_signal (priv->proxy, "SatellitesChanged",
				 dbus_g_type_get_collection ("GPtrArray", GYPSY_SATELLITE_SATELLITES_CHANGED_TYPE),
				 G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "SatellitesChanged",
				     G_CALLBACK (satellites_changed),
				     satellite, NULL);

	return G_OBJECT (satellite);
}

static void
gypsy_satellite_class_init (GypsySatelliteClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;
	o_class->set_property = set_property;
	o_class->get_property = get_property;

	g_type_class_add_private (klass, sizeof (GypsySatellitePrivate));

	/**
	 * GypsyCourse:object-path:
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
	 * GypsySatellite::satellites-changed:
	 * @satellites: A #GPtrArray containing #GypsySatelliteDetails
	 *
	 * The ::satellites-changed signal is emitted every time the GPS
	 * reports a change in the satellite data.
	 */
	signals[SATELLITES_CHANGED] = g_signal_new ("satellites-changed",
						    G_TYPE_FROM_CLASS (klass),
						    G_SIGNAL_RUN_LAST, 0,
						    NULL, NULL,
						    g_cclosure_marshal_VOID__POINTER,
						    G_TYPE_NONE, 1, 
						    G_TYPE_POINTER);
}

static void
gypsy_satellite_init (GypsySatellite *satellite)
{
}

/**
 * gypsy_satellite_new:
 * @object_path: Object path to the GPS device
 *
 * Creates a new #GypsySatellite object that listens for satellite changes
 * from the GPS found at @object_path.
 *
 * Return value: A #GypsySatellite object
 */
GypsySatellite *
gypsy_satellite_new (const char *object_path)
{
	return g_object_new (GYPSY_TYPE_SATELLITE, 
			     "object-path", object_path,
			     NULL);
}

/**
 * gypsy_satellite_get_satellites:
 * @satellite: A #GypsySatellite
 * @error: A #GError for error return
 *
 * Retrieves the #GypsySatelliteDetails about the satellites that the 
 * GPS is able to see.
 *
 * Return value: A #GPtrArray of #GypsySatelliteDetails or #NULL on error. 
 * Should be freed using gypsy_satellite_free_satellite_array().
 */
GPtrArray *
gypsy_satellite_get_satellites (GypsySatellite *satellite,
				GError        **error)
{
	GypsySatellitePrivate *priv;
	GPtrArray *satellites, *sats;

	g_return_val_if_fail (GYPSY_IS_SATELLITE (satellite), NULL);

	priv = GET_PRIVATE (satellite);

	if (!org_freedesktop_Gypsy_Satellite_get_satellites (priv->proxy,
							     &sats, error)) {
		return NULL;
	}

	satellites = make_satellite_array (sats);

	return satellites;
}

/**
 * gypsy_satellite_free_satellite_array:
 * @satellites: #GPtrArray containing #GypsySatelliteDetails
 *
 * Frees all resources used in the array.
 */
void
gypsy_satellite_free_satellite_array (GPtrArray *satellites)
{	
	int i;
	
	for (i = 0; i < satellites->len; i++) {
		g_slice_free (GypsySatelliteDetails, satellites->pdata[i]);
	}

	g_ptr_array_free (satellites, TRUE);
}
