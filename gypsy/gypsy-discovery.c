/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@sleepfive.com>
 * Copyright (C) 2011
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

#include "gypsy-discovery.h"
#include "gypsy-discovery-bindings.h"
#include "gypsy-marshal.h"

enum {
	PROP_0,
};

enum {
	DEVICE_ADDED,
	DEVICE_REMOVED,
	LAST_SIGNAL,
};

struct _GypsyDiscoveryPrivate {
	DBusGProxy *proxy;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GYPSY_TYPE_DISCOVERY, GypsyDiscoveryPrivate))
G_DEFINE_TYPE (GypsyDiscovery, gypsy_discovery, G_TYPE_OBJECT);
static guint32 signals[LAST_SIGNAL] = {0,};

static void
gypsy_discovery_finalize (GObject *object)
{
#if 0
	GypsyDiscovery *self = (GypsyDiscovery *) object;
#endif

	G_OBJECT_CLASS (gypsy_discovery_parent_class)->finalize (object);
}

static void
gypsy_discovery_dispose (GObject *object)
{
	GypsyDiscovery *self = (GypsyDiscovery *) object;
	GypsyDiscoveryPrivate *priv = self->priv;

	if (priv->proxy) {
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (gypsy_discovery_parent_class)->dispose (object);
}

#if 0
static void
gypsy_discovery_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	GypsyDiscovery *self = (GypsyDiscovery *) object;

	switch (prop_id) {

	default:
		break;
	}
}

static void
gypsy_discovery_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	GypsyDiscovery *self = (GypsyDiscovery *) object;

	switch (prop_id) {

	default:
		break;
	}
}
#endif

static void
gypsy_discovery_class_init (GypsyDiscoveryClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->dispose = gypsy_discovery_dispose;
	o_class->finalize = gypsy_discovery_finalize;

#if 0
	o_class->set_property = gypsy_discovery_set_property;
	o_class->get_property = gypsy_discovery_get_property;
#endif

	g_type_class_add_private (klass, sizeof (GypsyDiscoveryPrivate));

	signals[DEVICE_ADDED] = g_signal_new ("device-added",
					      G_TYPE_FROM_CLASS (klass),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      0, NULL, NULL,
					      g_cclosure_marshal_VOID__STRING,
					      G_TYPE_NONE, 1, G_TYPE_STRING);
	signals[DEVICE_REMOVED] = g_signal_new ("device-removed",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_FIRST |
						G_SIGNAL_NO_RECURSE,
						0, NULL, NULL,
						g_cclosure_marshal_VOID__STRING,
						G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
device_added_cb (DBusGProxy     *proxy,
                 const char     *added,
		 const char     *type,
                 GypsyDiscovery *discovery)
{
	g_signal_emit (discovery, signals[DEVICE_ADDED], 0, added, type);
}

static void
device_removed_cb (DBusGProxy     *proxy,
                   const char     *removed,
		   const char     *type,
                   GypsyDiscovery *discovery)
{
	g_signal_emit (discovery, signals[DEVICE_REMOVED], 0, removed, type);
}

static void
gypsy_discovery_init (GypsyDiscovery *self)
{
	GypsyDiscoveryPrivate *priv = GET_PRIVATE (self);
	DBusGConnection *connection;
	GError *error = NULL;

	self->priv = priv;

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		g_warning ("Error getting bus: %s", error->message);
		return;
	}

	/* We register all the other marshallers in gypsy_control_init,
	   but GypsyDiscovery can be created before GypsyControl */
	dbus_g_object_register_marshaller (gypsy_marshal_VOID__STRING_STRING,
					   G_TYPE_NONE, G_TYPE_STRING,
					   G_TYPE_STRING, G_TYPE_INVALID);
	priv->proxy = dbus_g_proxy_new_for_name (connection,
						 GYPSY_DISCOVERY_DBUS_SERVICE,
						 GYPSY_DISCOVERY_DBUS_PATH,
						 GYPSY_DISCOVERY_DBUS_INTERFACE);
	dbus_g_proxy_add_signal (priv->proxy, "DeviceAdded",
				 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_add_signal (priv->proxy, "DeviceRemoved",
				 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "DeviceAdded",
				     G_CALLBACK (device_added_cb), self, NULL);
	dbus_g_proxy_connect_signal (priv->proxy, "DeviceRemoved",
				     G_CALLBACK (device_removed_cb),
				     self, NULL);
}

GypsyDiscovery *
gypsy_discovery_new (void)
{
	return (GypsyDiscovery *) g_object_new (GYPSY_TYPE_DISCOVERY, NULL);
}

/**
 * gypsy_discovery_list_devices:
 * @discovery: A #GypsyDiscovery object
 * @error: A pointer to a #GError for error return
 *
 * Obtains the GPS devices that Gypsy knows about.
 *
 * Return value: A #GPtrArray containing #GypsyDiscoveryDeviceInfo.
 * The array is owned by the caller and should be freed with
 * g_ptr_array_free when it is finished with.
 */
GPtrArray *
gypsy_discovery_list_devices (GypsyDiscovery *discovery,
                              GError        **error)
{
	GypsyDiscoveryPrivate *priv;
	gboolean result;
	char **devices;
	char **types;
	GPtrArray *known_devices;
	int i;

	g_return_val_if_fail (GYPSY_IS_DISCOVERY (discovery), NULL);
	priv = discovery->priv;

	result = org_freedesktop_Gypsy_Discovery_list_devices (priv->proxy,
							       &devices,
							       &types,
							       error);
	if (!result) {
		return NULL;
	}

	known_devices = g_ptr_array_new_with_free_func
		((GDestroyNotify) gypsy_discovery_device_info_free);
	for (i = 0; devices[i]; i++) {
		GypsyDiscoveryDeviceInfo *di;

		di = g_slice_new (GypsyDiscoveryDeviceInfo);

		/* We don't copy the data here,
		   so we don't need to free it */
		di->device_path = devices[i];
		di->type = types[i];
		g_ptr_array_add (known_devices, di);
	}

	/* We only need to free the array here, not the data */
	g_free (devices);
	g_free (types);

	return known_devices;
}

gboolean
gypsy_discovery_start_scanning (GypsyDiscovery *discovery,
                                GError        **error)
{
	GypsyDiscoveryPrivate *priv;
	gboolean result;

	g_return_val_if_fail (GYPSY_IS_DISCOVERY (discovery), FALSE);
	priv = discovery->priv;

	result = org_freedesktop_Gypsy_Discovery_start_scanning (priv->proxy,
								 error);
	return result;
}

gboolean
gypsy_discovery_stop_scanning (GypsyDiscovery *discovery,
                               GError        **error)
{
	GypsyDiscoveryPrivate *priv;
	gboolean result;

	g_return_val_if_fail (GYPSY_IS_DISCOVERY (discovery), FALSE);
	priv = discovery->priv;

	result = org_freedesktop_Gypsy_Discovery_stop_scanning (priv->proxy,
								error);
	return result;
}

GypsyDiscoveryDeviceInfo *
gypsy_discovery_device_info_copy (GypsyDiscoveryDeviceInfo *di)
{
	GypsyDiscoveryDeviceInfo *dicopy;

	dicopy = g_slice_new (GypsyDiscoveryDeviceInfo);
	dicopy->device_path = g_strdup (di->device_path);
	dicopy->type = g_strdup (di->type);

	return dicopy;
}

void
gypsy_discovery_device_info_free (GypsyDiscoveryDeviceInfo *device_info)
{
	g_free (device_info->device_path);
	g_free (device_info->type);
	g_slice_free (GypsyDiscoveryDeviceInfo, device_info);
}
