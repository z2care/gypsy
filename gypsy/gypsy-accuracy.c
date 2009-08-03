/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>, Ross Burton <ross@openedhand.com>
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
 * SECTION:gypsy-accuracy
 * @short_description: Object for obtaining accuracy information from gypsy-daemon
 *
 * #GypsyAccuracy is used whenever the client program wishes to know about GPS
 * accuracy changes. It can report the current accuracy, and has a signal to
 * notify listeners to changes. The accuracy consists of positional accuracy,
 * horizontal accuracy (on the latitude/longitude plane) and vertical 
 * (altitudinal) accuracy.
 *
 * A #GypsyAccuracy object is created using gypsy_accuracy_new() using the 
 * D-Bus path of the GPS device. This path is returned from the 
 * gypsy_control_create() function. The client can then find out about the
 * course with gypsy_accuracy_get_accuracy().
 *
 * As the accuracy information changes #GypsyAccuracy will emit the 
 * accuracy-changed signal. This signal contains the accuracy information if 
 * gypsy-daemon knows it. It has a fields paramater which is a bitmask of
 * #GypsyAccuracyFields which indicates which of the position, horizontal or
 * vertical contains valid information. 
 *
 * <informalexample>
 * <programlisting>
 * GypsyAccuracy *accuracy;
 * GError *error = NULL;
 *
 * . . .
 *
 * / * path comes from the gypsy_control_create() function * /
 * accuracy = gypsy_accuracy_new (path);
 * g_signal_connect (accuracy, "accuracy-changed", G_CALLBACK (accuracy_changed), NULL);
 * 
 * . . .
 *
 * static void 
 * accuracy_changed (GypsyAccuracy *accuracy, 
 * GypsyAccuracyFields fields,
 * double position,
 * double horizontal,
 * double vertical,
 * gpointer userdata)
 * {
 * &nbsp;&nbsp;g_print ("horizontal: %f\n", (fields & GYPSY_ACCURACY_FIELDS_HORIZONTAL) ? horizontal : -1.0);
 * }
 * </programlisting>
 * </informalexample>
 */

#include <glib-object.h>

#include <gypsy/gypsy-marshal.h>
#include <gypsy/gypsy-accuracy.h>

#include "gypsy-client-bindings.h"

typedef struct _GypsyAccuracyPrivate {
	DBusGProxy *proxy;
	char *object_path;
} GypsyAccuracyPrivate;

enum {
	ACCURACY_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_PATH
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GYPSY_TYPE_ACCURACY, GypsyAccuracyPrivate))

G_DEFINE_TYPE (GypsyAccuracy, gypsy_accuracy, G_TYPE_OBJECT);

static void accuracy_changed (DBusGProxy    *proxy,
			      int            fields,
			      double         pdop,
			      double         hdop,
			      double         vdop,
			      GypsyAccuracy *accuracy);

static guint32 signals[LAST_SIGNAL] = {0, };
static void
finalize (GObject *object)
{
	GypsyAccuracyPrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->object_path) {
		g_free (priv->object_path);
	}

	G_OBJECT_CLASS (gypsy_accuracy_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GypsyAccuracyPrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->proxy) {
		dbus_g_proxy_disconnect_signal (priv->proxy, "AccuracyChanged",
						G_CALLBACK (accuracy_changed),
						object);
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (gypsy_accuracy_parent_class)->dispose (object);
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	GypsyAccuracyPrivate *priv;

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
	GypsyAccuracyPrivate *priv;

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
accuracy_changed (DBusGProxy    *proxy,
		  int            fields,
		  double         pdop,
		  double         hdop,
		  double         vdop,
		  GypsyAccuracy *accuracy)
{
	g_signal_emit (accuracy, signals[ACCURACY_CHANGED], 0,
		       fields, pdop, hdop, vdop);
}

static void
get_accuracy_cb (DBusGProxy *proxy,
		 int         fields, 
		 double      pdop,
		 double      hdop,
		 double      vdop,
		 GError     *error, 
		 gpointer    userdata)
{
	GypsyAccuracy *accuracy = userdata;

	if (error) {
		g_warning ("Cannot get accuracy: %s", error->message);
		g_error_free (error);
		return;
	}

	accuracy_changed (proxy, fields,
			  pdop, hdop, vdop, accuracy);
}

static GObject *
constructor (GType                  type,
	     guint                  n_construct_properties,
	     GObjectConstructParam *construct_properties)
{
	GypsyAccuracy *accuracy;
	GypsyAccuracyPrivate *priv;
	DBusGConnection *connection;
	GError *error;

	accuracy = GYPSY_ACCURACY (G_OBJECT_CLASS (gypsy_accuracy_parent_class)->constructor 
				   (type, n_construct_properties,
				    construct_properties));

	priv = GET_PRIVATE (accuracy);

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		g_printerr ("Failed to open connection to bus: %s\n",
			    error->message);
		g_error_free (error);
		
		priv->proxy = NULL;
		return G_OBJECT (accuracy);
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection, 
						 GYPSY_ACCURACY_DBUS_SERVICE,
						 priv->object_path,
						 GYPSY_ACCURACY_DBUS_INTERFACE);

	dbus_g_proxy_add_signal (priv->proxy, "AccuracyChanged",
				 G_TYPE_INT, G_TYPE_DOUBLE,
				 G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "AccuracyChanged",
				     G_CALLBACK (accuracy_changed),
				     accuracy, NULL);

	org_freedesktop_Gypsy_Accuracy_get_accuracy_async
		(priv->proxy, get_accuracy_cb, accuracy);
	
	return G_OBJECT (accuracy);
}

static void
gypsy_accuracy_class_init (GypsyAccuracyClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;
	o_class->set_property = set_property;
	o_class->get_property = get_property;

	g_type_class_add_private (klass, sizeof (GypsyAccuracyPrivate));

	/**
	 * GypsyAccuracy:object-path:
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
	 * GypsyAccuracy::accuracy-changed:
	 * @fields: A bitmask of #GypsyAccuracyFields indicating which of the following fields are valid
	 * @pdop: The new position DOP
	 * @hdop: The new horizonal DOP
	 * @vdop: The new vertical DOP
	 *
	 * The ::accuracy-changed signal is emitted when the GPS device
	 * indicates that one or more of the accuracy fields has changed.
	 * The fields which have changed will be indicated in the @fields
	 * bitmask.
	 */
	signals[ACCURACY_CHANGED] = g_signal_new ("accuracy-changed",
						  G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (GypsyAccuracyClass, accuracy_changed),
						  NULL, NULL,
						  gypsy_marshal_VOID__INT_DOUBLE_DOUBLE_DOUBLE,
						  G_TYPE_NONE, 4,
						  G_TYPE_INT, G_TYPE_DOUBLE,
						  G_TYPE_DOUBLE, G_TYPE_DOUBLE);
}

static void
gypsy_accuracy_init (GypsyAccuracy *accuracy)
{
}

/**
 * gypsy_accuracy_new:
 * @object_path: Object path to the GPS device
 *
 * Creates a new #GypsyAccuracy object that listens for accuracy changes
 * from the GPS found at @object_path.
 *
 * Return value: A #GypsyAccuracy object
 */
GypsyAccuracy *
gypsy_accuracy_new (const char *object_path)
{
	return g_object_new (GYPSY_TYPE_ACCURACY, 
			     "object-path", object_path,
			     NULL);
}

/**
 * gypsy_accuracy_get_accuracy:
 * @accuracy: A #GypsyAccuracy
 * @pdop: Pointer to store the position DOP
 * @hdop: Pointer to store the horizonal DOP
 * @vdop: Pointer to store the vertical DOP
 * @error: Pointer to store a #GError
 *
 * Obtains the current accuracy, if known, from the GPS device.  @pdop, @hdop
 * and @vdop can be #NULL if the result is not required.
 *
 * Return value: Bitmask of #GypsyAccuracyFields indicating what fields were set
 */
GypsyAccuracyFields
gypsy_accuracy_get_accuracy (GypsyAccuracy *accuracy,
			     double        *pdop,
			     double        *hdop,
			     double        *vdop,
			     GError       **error)
{
	GypsyAccuracyPrivate *priv;
	double p, h, v;
	int fields;
	
	g_return_val_if_fail (GYPSY_IS_ACCURACY (accuracy), GYPSY_ACCURACY_FIELDS_NONE);

	priv = GET_PRIVATE (accuracy);
	if (!org_freedesktop_Gypsy_Accuracy_get_accuracy (priv->proxy,
							  &fields,
							  &p, &h, &v,
							  error)) {
		return GYPSY_ACCURACY_FIELDS_NONE;
	}

	if (pdop != NULL && (fields & GYPSY_ACCURACY_FIELDS_POSITION)) {
		*pdop = p;
	}

	if (hdop != NULL && (fields & GYPSY_ACCURACY_FIELDS_HORIZONTAL)) {
		*hdop = h;
	}

	if (vdop != NULL && (fields & GYPSY_ACCURACY_FIELDS_VERTICAL)) {
		*vdop = v;
	}

	return fields;
}
